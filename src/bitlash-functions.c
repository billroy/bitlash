/***
	bitlash-functions.c

	Bitlash is a tiny language interpreter that provides a serial port shell environment
	for bit banging and hardware hacking.

	See the file README for documentation.  Or just upload this file as a sketch and play.

	Bitlash lives at: http://bitlash.net
	The author can be reached at: bill@bitlash.net

	Copyright (C) 2008, 2009 Bill Roy

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


// Function handlers
//	note: this enumeration must be sorted in alpha order 
//	and must be 1:1 with functionargs and functiondict below
#define f_abs		0
#define f_ar		1
#define f_aw		2
#define f_baud		3
#define f_beep		4
#define f_buttons	5
#define f_con		6
#define f_delay		7
#define f_dr		8
#define f_du		9
#define f_dw		10
#define f_er		11
#define f_ew		12
#define f_flash		13
#define f_free		14
#define f_key		15
#define f_map		16
#define f_max		17
#define f_millis	18
#define f_min		19
#define f_move		20
#define f_pinmode	21
#define f_pulsein	22
#define f_rand2		23
#define f_random	24
#define f_sa		25
#define f_shiftout	26
#define f_sign		27
#define f_snooze	28
#define f_sr		29
#define f_syms		30
#define f_usr		31
#define f_wheel		32
#define NUMFUNCTS	33		// update this when you add a function

// Function arguments
#define MAXARGS 5
prog_char functionargs[NUMFUNCTS] PROGMEM = { 1,1,2,2,3,1,3,1,1,1,2,1,2,2,0,1,5,2,0,2,2,2,3,2,1,0,4,1,1,0,0,1,1 };

// Function name dictionary
prog_char functiondict[] PROGMEM = {
	"abs\0ar\0aw\0baud\0beep\0buttons\0constrain\0delay\0dr\0du\0dw\0er\0ew\0flash\0free\0key\0map\0max\0millis\0min\0move\0pinmode\0pulsein\0rand2\0random\0sa\0shiftout\0sign\0snooze\0sr\0syms\0usr\0wheel\0"
};

#ifdef LONG_ALIASES
// Arduino-compatible aliases for commonly used function names
prog_char aliasdict[] PROGMEM = {
	//".\0analogread\0analogwrite\0.\0constrain\0.\0digitalread\0delaymicroseconds\0digitalwrite\0eeprom_read_byte\0eeprom_write_byte\0.\0.\0.\0.\0.\0.\0.\0pinmode\0.\0.\0.\0Serial.available\0.\0.\0Serial.read\0.\0user\0"
	"`\0analogread\0analogwrite\0`\0`\0`\0`\0digitalread\0delaymicroseconds\0digitalwrite\0"
};
#endif


// f_delay checks the keyboard for break this often:
#define CHKINTERVAL 1000


#if !defined(TINY85)
// Abbreviations used below
#define arg1 args[0]
#define arg2 args[1]
#define arg3 args[2]
#define arg4 args[3]
#define arg5 args[4]
#endif


// Deadbeef random number generator
static uint32_t deadbeef_seed = 0xbeefcafe;
static uint32_t deadbeef_beef = 0xdeadbeef;

void stir(byte trash) {
	deadbeef_seed ^= 1 << (trash & 0x1f);
}

#if 0
void deadbeef_srand(uint32_t x) {
	deadbeef_seed = x;
	deadbeef_beef = 0xdeadbeef;
}
#endif


void getfunction(char functionid) {
// Function argument storage
byte argct = 0;
char nargs = pgm_read_byte(functionargs + functionid);
numvar args[MAXARGS];


// Return value pushed at the bottom of the switch
numvar ret=0;

	// Parse argument list
	if (sym == s_lparen) {
		getsym();		// eat arglist '('
		while ((sym != s_rparen) && (argct < nargs)) {
			args[argct++] = getnum();
			if (sym == s_comma) getsym();	// eat arglist ',' and go around
			else break;
		}
		if (sym != s_rparen) expected(M_rparen);
		if (argct != nargs) expected(M_number);
		getsym();	// eat arglist ')'
	}
	else if (nargs) expectedchar('(');

#ifdef TINY85
	// looks yucky but saves 80 bytes to do the indexing just once
	numvar arg1 = args[0];
	numvar arg2 = args[1];
	//numvar arg3 = args[2];
	//numvar arg4 = args[3];
	//numvar arg5 = args[4];
#endif

	switch (functionid) {
		case f_du:		delayMicroseconds(arg1);					break;		// first please
		case f_millis:	ret = millis();								break;
		case f_pinmode:	pinMode(arg1, arg2);						break;
		case f_dr:		ret = digitalRead(arg1);					break;
		case f_dw:		digitalWrite(arg1, arg2);					break;
		case f_snooze:	snooze(arg1);								break;
		case f_delay:
#if !defined(TINY85)
			while (arg1 >= CHKINTERVAL) {
				delay(CHKINTERVAL);
				chkbreak();
				arg1 -= CHKINTERVAL;
			}
#endif
			delay(arg1);		// final residue
			break;
		case f_er:		ret = eeread(arg1);							break;
		case f_ew:		eewrite(arg1, arg2);						break;

#if !defined(TINY85)
		case f_sa:		ret = serialAvailable();					break;
		case f_sr:		ret = serialRead();							break;

// TODO: these work but are disabled pending resolution of the linker bug
		case f_syms:	ret = symcount;								break;

		case f_flash:	flash(arg1,arg2);							break;
		case f_min:		ret = (arg1 < arg2) ? arg1 : arg2;			break;
		case f_max:		ret = (arg1 > arg2) ? arg1 : arg2;			break;
		case f_beep:	beep(arg1, arg2, arg3);						break;
#endif

		case f_free: {
			// from http://forum.pololu.com/viewtopic.php?f=10&t=989&view=unread#p4218
			extern int __bss_end;
			ret = ((int)&ret) - ((int)&__bss_end);
			break;
		}

		case f_abs: 	ret = (arg1 < 0) ? -arg1 : arg1;			break;

		case f_sign:
			if (arg1 > 0) ret = 1;
			else if (arg1 < 0) ret = -1;
			// zero is already handled by the initializer
			break;

		case f_random:	
			/*
				Dead Beef Random Number Generator
				from http://inglorion.net/software/deadbeef_rand/
				stated license terms: "Feel free to use the code in your own software."
			*/
			deadbeef_seed = (deadbeef_seed << 7) ^ ((deadbeef_seed >> 25) + deadbeef_beef);
			ret = ((numvar) deadbeef_seed & 0x7fff) % arg1;
			deadbeef_beef = (deadbeef_beef << 7) ^ ((deadbeef_beef >> 25) + 0xdeadbeef);
			break;	// 1 arg only

#if 0
		case f_seed:
			deadbeef_seed = (deadbeef_seed << arg2) | (uint32_t) arg1;
			break;
#endif

		case f_ar:		ret = analogRead(arg1);						break;

#ifdef ARDUINO_BUILD
		case f_aw:		analogWrite(arg1, arg2);					break;
		case f_con:		ret = constrain(arg1, arg2, arg3);			break;
		case f_map:		ret = map(arg1, arg2, arg3, arg4, arg5);	break;
		case f_pulsein:	ret = pulseIn(arg1, arg2, arg3);			break;	// 3 args only
		case f_rand2:	ret = random(arg1, arg2);					break;
		case f_shiftout: 
			pinMode(arg1, OUTPUT);
			pinMode(arg2, OUTPUT);
			shiftOut(arg1, arg2, arg3, arg4);
			break;
#endif


#if defined(AVROPENDOUS_BUILD) || defined(TINY85)
	case f_move:	usbMouse(arg1, arg2, buttons, 0);				break;
	case f_buttons:	ret = buttons; buttons = arg1;					break;
	case f_wheel:	usbMouse(0, 0, buttons, arg1);					break;	
	case f_key:		usbKeystroke(arg1);								break;
#endif

#ifdef SOFTWARE_SERIAL_TX
		case f_baud:	ret=setBaud(arg1, arg2);					break;
#endif

//#ifndef TINY85
		//
		// User Modifiable Function
		// Add your code below and call it via usr(value)
		//
		case f_usr:
			
			// Dear Lazy Coder: Here is an easy way to add your code to bitlash: the usr() function.
			//
			// It's pretty easy to add a new function on your own, but it's
			// even easier to just drop some code in here and call it via usr().
			//
			// Simple example below: convert argument from DEGF to DEGC and return the value.
			//
			// Replace this with your code.  Return a value.  Upload to Arduino.
			// Then you can reference usr(x) in an expression to run your code and use the result.
			// 
			//ret = (arg1-32) * 5 / 9;	// convert fahrenheit to celsius: print usr(72) => 22

			// Second example: return Tiny85 temperature sensor
			//ret = analogRead(_BV(REFS1) | 0xf);		// read Tiny85 built-in temperature sensor

			// testing
			ret = vsptr;

			break;
		//
		// End user function section
		//
//#endif

		default: unexpected(M_id);
	}
	vpush(ret);
}


///////////////////////
// FUNCTION HANDLERS
//
void beep(unumvar pin, unumvar frequency, unumvar duration) {
	unsigned long cycles = ((unsigned long)frequency * (unsigned long)duration) / 1000UL;
	unsigned long halfperiod = (500000UL / (unsigned long) frequency) - 7UL;	// 7 fudge

	// todo: check for break in here?  This could go 32 seconds...
	pinMode(pin, OUTPUT);
	while (cycles--) {
		digitalWrite(pin, HIGH);
		delayMicroseconds(halfperiod);
		digitalWrite(pin, LOW);
		delayMicroseconds(halfperiod-1);
	}
}


#if 0
// from http://www.embeddedgurus.net/stack-overflow/2007/04/crest-factor-square-roots-neat.html
static inline uint8_t friden_sqrt16(uint16_t val) {
uint16_t rem = 0;
uint16_t root = 0;
uint8_t i; 
	for (i=0; i < 8; i++) {
		root <<= 1;
		rem = (rem << 2) + (val >> 14);
		val <<= 2;
		if (++root <= rem) {
			rem -=root;
			root++;
		} else root--;
	}
	return (uint8_t)(root >> 1);
}
#endif
