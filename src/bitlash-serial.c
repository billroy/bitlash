/***
	bitlash-serial.c

	Bitlash is a tiny language interpreter that provides a serial port shell environment
	for bit banging and hardware hacking.

	See the file README for documentation.

	Bitlash lives at: http://bitlash.net
	The author can be reached at: bill@bitlash.net

	Copyright (C) 2008-2011 Bill Roy

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

***/
#include "bitlash.h"


// Character io primitives
//#define spb serialWrite


#ifdef SOFTWARE_SERIAL_TX

// The default default outpin is, of course, zero
#ifndef DEFAULT_OUTPIN
#define DEFAULT_OUTPIN 0
#endif

#define DEFAULT_SECONDARY_BAUD 9600L
byte outpin = DEFAULT_OUTPIN;	// output pin

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
// we have no serial io, so we don't define spb, sp, speol
#define spb(x)
#define sp(x)
#define speol(x)
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


#if (defined(ARDUINO_BUILD) && (ARDUINO_VERSION >= 12)) || defined(AVROPENDOUS_BUILD)
// From Arduino 0011/wiring_serial.c
// These apparently were removed from wiring_serial.c in 0012

void printIntegerInBase(unumvar n, uint8_t base) {
	char buf[8 * sizeof(numvar)];		// stack for the digits
	char *ptr = buf;
	if (n == 0) {
		spb('0');
		return;
	} 
	while (n > 0) {
		*ptr++ = n % base;
		n /= base;
	}
	while (--ptr >= buf) spb((*ptr < 10) ? (*ptr + '0') : (*ptr - 10 + 'A'));
}
void printInteger(numvar n){
	if (n < 0) {
		spb('-');
		n = -n;
	}
	printIntegerInBase(n, 10);
}
void printHex(unumvar n) { printIntegerInBase(n, 16); }
void printBinary(unumvar n) { printIntegerInBase(n, 2); }
#endif




#if !defined(TINY85)
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




#if !defined(TINY85)

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
				if ((sym == s_nvar) && (symval == 'x'-'a')) printHex((unumvar) expval);
				else if ((sym == s_nvar) && (symval == 'b'-'a')) printBinary((unumvar) expval);
				else if ((sym == s_nvar) && (symval == 'y'-'a')) spb(expval);
				else if (sym > ' ') while (expval-- > 0) spb(sym);	// any litsym
				else expected(M_pfmts);
				getsym();
			}
			else printInteger(expval);
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

