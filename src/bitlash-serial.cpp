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
#include "bitlash-private.h"

#ifndef DEFAULT_CONSOLE_ONLY
Stream *blconsole = &DEFAULT_CONSOLE;
#endif

#ifdef SERIAL_OVERRIDE
Print *bloutdefault = blconsole;
#endif

#ifdef SOFTWARE_SERIAL_TX
Print *blout = blconsole;
#endif

#ifdef SOFTWARE_SERIAL_TX

#define DEFAULT_SECONDARY_BAUD 9600L

#ifndef NUMPINS
#define NUMPINS 32				// default to Arduino Diecimila/168..328
#endif

// set output back to 'stdout' ;)
void resetOutput(void) { blout = bloutdefault; }

// TX-only minimal software serial implementation
class SoftwareSerialTX : public Print {
public:
	uint8_t pin;
	static uint16_t bittime[NUMPINS]; // bit times (1000000/baud) per pin, 0 = uninitialized

	// bit whack a byte out the port designated by 'outpin'
#if defined(ARDUINO) && ARDUINO < 100
	virtual void write(uint8_t c) {
#else
	virtual size_t write(uint8_t c) {
#endif
		const uint16_t bt = bittime[this->pin];
		char bits = 8; // 8 data bits
		digitalWrite(this->pin, LOW);
		delayMicroseconds(bt);
		while (bits--) {
			//if ((c & 1) == 0) digitalWrite(outpin, LOW);
			//else digitalWrite(outpin, HIGH);
			digitalWrite(this->pin, c & 1);
			delayMicroseconds(bt);
			c >>= 1;
		}
		digitalWrite(this->pin, HIGH);
		delayMicroseconds(bt<<1);
#if !(defined(ARDUINO) && ARDUINO < 100)
		return 1;
#endif
	}


};

uint16_t SoftwareSerialTX::bittime[];

static SoftwareSerialTX sstx;

void chkpin(char pin) {
	// TODO: fix this warning re: comparison
	if ((pin >= NUMPINS) || (pin < 0)) unexpected(M_number); 
}

numvar setBaud(numvar pin, unumvar baud) {
	chkpin(pin);

#ifdef DEFAULT_OUTPIN
	if (pin == DEFAULT_OUTPIN) {
		DEFAULT_CONSOLE.begin(baud);
		return 0;
	}
#endif

#ifdef ALTERNATE_OUTPIN
	else if (pin == ALTERNATE_OUTPIN) {
		Serial1.begin(baud);
		return 0;
	}
#endif

	sstx.bittime[pin] = (1000000/baud) - clockCyclesToMicroseconds(50);
	pinMode(pin, OUTPUT);				// make it an output
	digitalWrite(pin, HIGH);				// set idle
	delayMicroseconds(sstx.bittime[pin]);		// let it quiesce
	return sstx.bittime[pin];
}

void setOutput(byte pin) {
	chkpin(pin);

#ifdef DEFAULT_OUTPIN
	if (pin == DEFAULT_OUTPIN) {
		blout = &DEFAULT_CONSOLE;
		return;
	}
#endif

#ifdef ALTERNATE_OUTPIN
	if (pin == ALTERNATE_OUTPIN) {
		blout = &Serial1;
		return;
	}
#endif

	// set the softserial baud if it's not already set
	if (!sstx.bittime[pin]) setBaud(pin, DEFAULT_SECONDARY_BAUD);
	sstx.pin = pin;
	blout = &sstx;
}

#endif	// SOFTWARE_SERIAL_TX



#ifdef SERIAL_OVERRIDE
///////////////////////////////////////
// serial output override mechanism
// the primary or default serial output can be diverted by plugging in a serialOverrideFunc
//
serialOutputFunc serial_override_handler;

class PrintToFunction : public Print {
public:
	serialOutputFunc func;

#if defined(ARDUINO) && ARDUINO < 100
	virtual void write(uint8_t c)
#else
	virtual size_t write(uint8_t c)
#endif
	{
		func (c);
#if !(defined(ARDUINO) && ARDUINO < 100)
		return 1;
#endif
	}
};

static PrintToFunction outputHandlerPrint;

byte serialIsOverridden(void) {
	return bloutdefault != blconsole;
}

void setOutputHandler(serialOutputFunc newHandler) {
	outputHandlerPrint.func = newHandler;
	blout = bloutdefault = &outputHandlerPrint;
}

void setOutputHandler(Print& print) {
	blout = bloutdefault = &print;
}

void resetOutputHandler(void) {
	blout = bloutdefault = blconsole;
}

#endif



///////////////////////////////////////
// spb: serial print byte
//
// this is a pinchpoint on output.  all output funnels through spb.
//
void spb(char c) {
	blout->write((uint8_t)c);
}

void sp(const char *str) { while (*str) spb(*str++); }
void speol(void) { spb(13); spb(10); }


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
	// allow ^C to break out
#if defined(ARDUINO) && ARDUINO < 100
	// Arduino before 1.0 didn't have peek(), so just read a
	// byte (this discards a byte when it wasn't ^C, though...
	if (blconsole->read() == 3) {
#else
	if (blconsole->peek() == 3) {
		blconsole->read();
#endif
		msgpl(M_ctrlc);
		longjmp(env, X_EXIT);
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
					else if (symval == 's'-'a') sp((const char *)expval);				// :s print string
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
	const char *fptr = (const char *) getarg(formatarg);		// format string pointer

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
					const char *sptr = (const char *) getarg(optionalargs);
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

