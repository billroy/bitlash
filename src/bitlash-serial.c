/***
	bitlash-serial.c

	Bitlash is a tiny language interpreter that provides a serial port shell environment
	for bit banging and hardware hacking.

	See the file README for documentation.

	Bitlash lives at: http://bitlash.net
	The author can be reached at: bill@bitlash.net

	Copyright (C) 2008-2012 Bill Roy

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:
	
	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.
	
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.

***/
#include "bitlash.h"


// Character io primitives
//#define spb serialWrite

// The default default outpin is, of course, zero
#ifndef DEFAULT_OUTPIN
#define DEFAULT_OUTPIN 0
#endif
byte outpin = DEFAULT_OUTPIN;	// output pin

#ifdef SOFTWARE_SERIAL_TX

#define DEFAULT_SECONDARY_BAUD 9600L

#ifndef NUMPINS
#define NUMPINS 32				// default to Arduino Diecimila/168..328
#endif
int bittime[NUMPINS];			// bit times (1000000/baud) per pin, 0 = uninitialized

// set output back to 'stdout' ;)
void resetOutput(void) { outpin = DEFAULT_OUTPIN; }

void chkpin(char pin) {
	// TODO: fix this warning re: comparison
	if ((pin >= NUMPINS) || (pin < 0)) unexpected(M_number); 
}

numvar setBaud(numvar pin, unumvar baud) {
	chkpin(pin);

//#ifndef SOFTWARE_SERIAL_TX
	if (pin == DEFAULT_OUTPIN) {
		beginSerial(baud);
		return 0;
	}
//#endif

#ifdef ALTERNATE_OUTPIN
	else if (pin == ALTERNATE_OUTPIN) {
		Serial1.begin(baud);
		return 0;
	}
#endif

	bittime[pin] = (1000000/baud) - clockCyclesToMicroseconds(50);
	pinMode(pin, OUTPUT);				// make it an output
	digitalWrite(pin, HIGH);				// set idle
	delayMicroseconds(bittime[pin]);		// let it quiesce
	return bittime[pin];
}

void setOutput(byte pin) {
	chkpin(pin);
	outpin = pin;

#ifdef HARDWARE_SERIAL_TX
	// skip soft baud check for the hardware uart
	if (outpin != DEFAULT_OUTPIN)
#endif
#ifdef ALTERNATE_OUTPIN
	if (outpin != ALTERNATE_OUTPIN)
#endif

	// set the softserial baud if it's not already set
	if (!bittime[outpin]) setBaud(pin, DEFAULT_SECONDARY_BAUD);
}

// bit whack a byte out the port designated by 'outpin'
void whackabyte(unsigned char c) {
	int bt = bittime[outpin];
	char bits = 8;		// 8 data bits
	digitalWrite(outpin, LOW);
	delayMicroseconds(bt);
	while (bits--) {
		//if ((c & 1) == 0) digitalWrite(outpin, LOW);
		//else digitalWrite(outpin, HIGH);
		digitalWrite(outpin, c & 1);
		delayMicroseconds(bt);
		c >>= 1;
	}
	digitalWrite(outpin, HIGH);
	delayMicroseconds(bt<<1);
}
#endif	// SOFTWARE_SERIAL_TX



#ifdef SERIAL_OVERRIDE
///////////////////////////////////////
// serial output override mechanism
// the primary or default serial output can be diverted by plugging in a serialOverrideFunc
//
serialOutputFunc serial_override_handler;

byte serialIsOverridden(void) {
	return serial_override_handler != 0;
}

void setOutputHandler(serialOutputFunc newHandler) {
	serial_override_handler = newHandler;
}

void resetOutputHandler(void) {
	serial_override_handler = 0;
}

#endif



///////////////////////////////////////
// spb: serial print byte
//
// this is a pinchpoint on output.  all output funnels through spb.
//
#if defined(HARDWARE_SERIAL_TX) || defined(SOFTWARE_SERIAL_TX)
void spb(char c) {
#ifdef HARDWARE_SERIAL_TX
	if (outpin == DEFAULT_OUTPIN) { 

#ifdef SERIAL_OVERRIDE
		if (serial_override_handler) (*serial_override_handler)(c);
		else
#endif
		serialWrite(c); 
		return;
	}
#endif
#ifdef ALTERNATE_OUTPIN
	if (outpin == ALTERNATE_OUTPIN) { Serial1.print(c); return; }
#endif
#ifdef SOFTWARE_SERIAL_TX
	whackabyte(c);
#endif
}
void sp(const char *str) { while (*str) spb(*str++); }
void speol(void) { spb(13); spb(10); }
#else
// handle no-serial case
#endif


/*
	bitlash software serial rx implementation adapted from:
	SoftwareSerial.cpp - Software serial library
	Copyright (c) 2006 David A. Mellis.	All right reserved. - hacked by ladyada 
*/
#ifdef SOFTWARE_SERIAL_RX

#define MAX_SOFT_RX_BUFF 32
volatile uint8_t soft_rx_buffer_head;
uint8_t soft_rx_buffer_tail;
char soft_rx_buffer[MAX_SOFT_RX_BUFF];

//	Pin-change interrupt handlers
//
//	NOTE: Currently, software rx is supported only on pins d0..d7.
//
//SIGNAL(SIG_PIN_CHANGE0) {}

#ifdef ARDUINO_BUILD
#define RXPINCHANGEREG PCMSK2
SIGNAL(SIG_PIN_CHANGE2) {
#else
#define RXPINCHANGEREG PCMSK0
SIGNAL(PCINT0_vect) {
#endif
	if ((RXPIN < 8) && !digitalRead(RXPIN)) {
		char c = 0;
		int bitdelay = bittime[RXPIN];
		// would be nice to know latency to here
		delayMicroseconds(bitdelay >> 1);			// wait half a bit to get centered
		for (uint8_t i=0; i<8; i++) { 
			delayMicroseconds(bitdelay);
			if (digitalRead(RXPIN)) c |= _BV(i); 
		}
		delayMicroseconds(bitdelay);	// fingers in ears for one stop bit

		uint8_t newhead = soft_rx_buffer_head + 1;
		if (newhead >= MAX_SOFT_RX_BUFF) newhead = 0;
		if (newhead != soft_rx_buffer_tail) {
			soft_rx_buffer[soft_rx_buffer_head] = c;
			soft_rx_buffer_head = newhead;
		}
	}
}
	
int softSerialRead(void) {
	uint8_t c;
	if (soft_rx_buffer_tail == soft_rx_buffer_head) return -1;
 	c = soft_rx_buffer[soft_rx_buffer_tail];
	cli();			// clean increment means less excrement
	if (++soft_rx_buffer_tail >= MAX_SOFT_RX_BUFF) soft_rx_buffer_tail = 0;
	sei();
	return c;
}

int softSerialAvailable(void) {
	cli();
	uint8_t avail = (soft_rx_buffer_tail != soft_rx_buffer_head);
	sei();
	return avail;
}

void beginSoftSerial(unsigned long baud) {

#ifdef BAUD_OVERRIDE
	baud = BAUD_OVERRIDE;
#endif
	// set up the output pin and copy its bit rate
	bittime[RXPIN] = setBaud(DEFAULT_OUTPIN, baud);
	pinMode(RXPIN, INPUT); 						// make it an input
	digitalWrite(RXPIN, HIGH);					// and engage the pullup
	delayMicroseconds(bittime[RXPIN]);			// gratuitous stop bits

	// Enable the pin-change interrupt for RXPIN
	if (RXPIN < 8) {			// a PIND pin, PCINT16-23
		RXPINCHANGEREG |= _BV(RXPIN);
		PCICR |= _BV(2);
	} 
#if 0
	// for pins above 7: enable these, set up a signal handler above,
	// and off you go.
	else if (RXPIN <= 13) {	// a PINB pin, PCINT0-5
		PCMSK0 |= _BV(RXPIN-8);
		PCICR |= _BV(0);		
	} 
	else if (RXPIN < 23) {	// a PINC pin, PCINT8-14
		PCMSK1 |= _BV(RXPIN-14);
		PCICR != _BV(1)
	}
#endif
	else unexpected(M_number);
	// todo: handle a0..7; pin out of range; extend for Sanguino, Mega, ...
}

// aliases for the rest of the interpreter
#define serialAvailable softSerialAvailable
#define serialRead softSerialRead
#define beginSerial beginSoftSerial

#endif // SOFTWARE_SERIAL_RX


#if (defined(ARDUINO_BUILD) && (ARDUINO_VERSION >= 12)) || defined(AVROPENDOUS_BUILD) || defined(UNIX_BUILD)
// From Arduino 0011/wiring_serial.c
// These apparently were removed from wiring_serial.c in 0012

void printIntegerInBase(unumvar n, uint8_t base, numvar width, byte pad) {
	char buf[8 * sizeof(numvar)];		// stack for the digits
	char *ptr = buf;
	if (n == 0) {
		*ptr++ = 0;
	} 
	else while (n > 0) {
		*ptr++ = n % base;
		n /= base;
	}

	// pad to width with leading zeroes
	if (width) {
		width -= (ptr-buf);
		while (width-- > 0) spb(pad);
	}

#ifdef UNIX_BUILD
	while (--ptr >= buf) {
		if (*ptr < 10) spb(*ptr + '0');
		else spb(*ptr - 10 + 'A');
	}
#else
	while (--ptr >= buf) spb((*ptr < 10) ? (*ptr + '0') : (*ptr - 10 + 'A'));
#endif
}
void printInteger(numvar n, numvar width, byte pad) {
	if (n < 0) {
		spb('-');
		n = -n;
		--width;
	}
	printIntegerInBase(n, 10, width, pad);
}
void printHex(unumvar n) { printIntegerInBase(n, 16, 0, '0'); }
void printBinary(unumvar n) { printIntegerInBase(n, 2, 0, '0'); }
#endif



#if defined(UNIX_BUILD)

void chkbreak(void) {
	extern byte break_received;
	if (break_received) {
		break_received = 0;
		msgpl(M_ctrlc);
		longjmp(env, X_EXIT);
	}
}

#elif !defined(TINY_BUILD)

// check serial input stream for ^C break
void chkbreak(void) {
	if (serialAvailable()) {		// allow ^C to break out
		if (serialRead() == 3) {	// BUG: this gobblesnarfs input characters! - need serialPeek()
			msgpl(M_ctrlc);
			longjmp(env, X_EXIT);
		}
	}
	if (func_free() < MINIMUM_FREE_RAM) overflow(M_stack);
}
#endif




#if 1	// !defined(TINY_BUILD)

// Print command handler
// 	print exprlist
void cmd_print(void) {
	for (;;) {

#ifdef SOFTWARE_SERIAL_TX
		// print #2: expr,expr,...
		if (sym == s_pound) {
			getsym();
			byte pin = getnum();		// pin to print to
			if (sym != s_colon) expectedchar(':');
			getsym();					// eat :
			setOutput(pin);
		}
#endif

		// Special handling for quoted strings
		if (sym == s_quote) {	// parse it and push it out the output hole (spb)
			parsestring(&spb);	// munch through the string (incl. closing quote) spewing it via spb
			getsym();			// and prime up the next symbol after for the comma check
		} 
		else if ((sym != s_semi) && (sym != s_eof))  {
			getexpression();

			// format specifier: :x :b
			if (sym == s_colon) {
				getsym();		// cheat and look for var ref to x or b
				if (sym == s_nvar) {
					if 		(symval == 'x'-'a') printHex((unumvar) expval);		// :x print hex
#if !defined(TINY_BUILD)
					else if (symval == 'b'-'a') printBinary((unumvar) expval);	// :b print binary
#endif
					else if (symval == 'y'-'a') spb(expval);					// :y print byte
					else if (symval == 's'-'a') sp((char *)expval);				// :s print string
				}
				else if (sym > ' ') while (expval-- > 0) spb(sym);	// any litsym
				else expected(M_pfmts);
				getsym();
			}
			else printInteger(expval, 0, 0);
		}
		if ((sym == s_semi) || (sym == s_eof)) {
			speol();
			break;
		}
		if (sym == s_comma) {
			//if (inchar ==' ') 	// significant whitespace?! ha ha ha ha ha!
			getsym();
			if ((sym == s_semi) || (sym == s_eof)) break;	// trailing comma: no crlf
			spb(' ');
		}
	}
#ifdef SOFTWARE_SERIAL_TX
	resetOutput();
#endif
}
#endif

//////////
//
//	printf("format string", item, item, item)
//
//
numvar func_printf_handler(byte formatarg, byte optionalargs) {

// todo: get rid of m_pfmts
// todo: get rid of s_pound

	if (getarg(0) < formatarg) { speol(); return 0; }
	char *fptr = (char *) getarg(formatarg);		// format string pointer

	while (*fptr) {
		if (*fptr == '%') {
			++fptr;
			numvar width = 0;
			char pad = ' ';
			if (*fptr == '0') {
				pad = '0';
				fptr++;
			}
			if (*fptr == '*') {
				width = getarg(optionalargs++);
				fptr++;
			}
			else while (isdigit(*fptr)) {
				width = (width * 10) + (*fptr++ - '0');
			}
			switch (*fptr) {
				case 'd':	printInteger(getarg(optionalargs), width, pad);		 break;	// decimal
				case 'x':	printIntegerInBase(getarg(optionalargs), 16, width, pad); break;	// hex
				case 'u':	printIntegerInBase(getarg(optionalargs), 10, width, pad); break;	// unsigned decimal (TODO: primitive)
				case 'b':	printIntegerInBase(getarg(optionalargs),  2, width, pad); break;	// binary

				case 's': {			// string
					char *sptr = (char *) getarg(optionalargs);
// BUG: width is the max not the prepad
					width -= strlen(sptr);
					while (width-- > 0) spb(' ');	// pre-pad with blanks
					sp(sptr);
					break;	// string
				}

				case 'c':			// byte ("char")
					do {
						spb(getarg(optionalargs));
					} while (width-- > 0);
					break;

				case '%':			// escaped '%'
					spb('%');
					continue;		// break; would increment optionalargs

#ifdef SOFTWARE_SERIAL_TX
				// print-to-digital-output-pin
				// "%#" - print to pin designated by next argument
				// 	printf("%#Hello, world!",3);		// prints to pin 3
				// "%4800#" - set baud and print to pin
				//	printf("%4800#Hello, world!", 3);
				//
				case '#':		// printf("%115200# %s %d", 3, "time:",millis)
					{
						byte pin = getarg(optionalargs);
						if (width) setBaud(pin, width);
						setOutput(pin);
					}
					break;
#endif

				default: 			// unknown format specifier
					spb('%');
					spb(*fptr);
					continue;		// break; would increment optionargs
			}
			optionalargs++;			// consume the arg printed for this %spec
		}
		else if (*fptr == '\n') speol();
		else spb(*fptr);
		++fptr;
	}

#ifdef SOFTWARE_SERIAL_TX
	resetOutput();
#endif

	return optionalargs;
}

numvar func_printf(void) {
	return func_printf_handler(1,2);	// format=arg(1), optional args start at 2
}

