/***
	bitlash-functions.c

	Bitlash is a tiny language interpreter that provides a serial port shell environment
	for bit banging and hardware hacking.

	Bitlash lives at: http://bitlash.net
	The author can be reached at: bill@bitlash.net

	Copyright (C) 2008, 2009, 2010 Bill Roy

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

// syntactic sugar for func_handlers()
extern numvar getarg(numvar);
#if 0	// 15022 bytes
#define arg1 getarg(1)
#define arg2 getarg(2)
#define arg3 getarg(3)
#endif

// 14772 vs 15022
extern numvar *arg;
#define arg1 arg[1]
#define arg2 arg[2]
#define arg3 arg[3]
void req1arg(void)  { if (arg[0] != 1) missing(M_arg); }
void req2args(void) { if (arg[0] != 2) missing(M_arg); }
void req3args(void) { if (arg[0] != 3) missing(M_arg); }


///////////////////////
// FUNCTION HANDLERS
//

// The native tone() routine first included in Arduino 0018 is quite large, 
//		about 1k differential: 15196 - 14110
// It is, however, for the moment quite a bit more frequency-accurate
//
//#define USE_NATIVE_TONE
#ifdef USE_NATIVE_TONE
void beep(unumvar pin, unumvar frequency, unumvar duration) {
	tone(pin, frequency, duration);
	delay(duration);
}
#else
numvar func_beep(void) { 		// unumvar pin, unumvar frequency, unumvar duration)
	req3args();
	unsigned long cycles = ((unsigned long) arg2 * (unsigned long) arg3) / 1000UL;
	unsigned long halfperiod = (500000UL / (unsigned long) arg2) - 7UL;	// 7 fudge

	// todo: check for break in here?  This could go 32 seconds...
	pinMode(arg1, OUTPUT);
	while (cycles--) {
		digitalWrite(arg1, HIGH);
		delayMicroseconds(halfperiod);
		digitalWrite(arg1, LOW);
		delayMicroseconds(halfperiod-1);
	}
}
#endif

numvar func_free(void) {
numvar ret;
	// from http://forum.pololu.com/viewtopic.php?f=10&t=989&view=unread#p4218
	extern int __bss_end;
	return ((int)&ret) - ((int)&__bss_end);
}


///////////////////////////////////////////////
//	Dead Beef Random Number Generator
//	from http://inglorion.net/software/deadbeef_rand/
//	stated license terms: "Feel free to use the code in your own software."
///////////////////////////////////////////////
//
static uint32_t deadbeef_seed = 0xbeefcafe;
static uint32_t deadbeef_beef = 0xdeadbeef;
numvar func_random(void) {
unumvar ret;
	req1arg();
	deadbeef_seed = (deadbeef_seed << 7) ^ ((deadbeef_seed >> 25) + deadbeef_beef);
	ret = ((numvar) deadbeef_seed & 0x7fff) % arg1;
	deadbeef_beef = (deadbeef_beef << 7) ^ ((deadbeef_beef >> 25) + 0xdeadbeef);
	return ret;
}

// add entropy to the pool
void stir(byte trash) {
	deadbeef_seed ^= 1 << (trash & 0x1f);
}

#if 0
// seed the random number generator
void dbseed(uint32_t x) {
	deadbeef_seed = x;
	deadbeef_beef = 0xdeadbeef;
}
#endif


/////////////////////////////////////////
// memory mapped port io: inb() and outb()
//
//	see the table of register addresses on page 203 of the Tiny85 spec
//	it is necessary to add 0x20 to the address in the "register" column
//	to get the proper mapped address for the register
//		thus: TCCR0B assigned 0x33 is accesssible at 0x53
//		>print inb(0x53)
//		2
//
numvar func_inb(void) { req1arg(); return *(volatile byte *) arg1; }
numvar func_outb(void) { req2args(); *(volatile byte *) arg1 = (byte) arg2; }
numvar func_abs(void) { req1arg(); return arg1 < 0 ? -arg1 : arg1; }
numvar func_sign(void) {
	req1arg();
	if (arg1 < 0) return -1;
	if (arg1 > 0) return 1;
	return 0;
}
numvar func_min(void) { req2args(); return (arg1 < arg2) ? arg1 : arg2; }
numvar func_max() { req2args(); return (arg1 > arg2) ? arg1 : arg2; }
numvar func_constrain(void) {
	req3args();
	if (arg1 < arg2) return arg2;
	if (arg1 > arg3) return arg3;
	return arg1;
}
numvar func_ar(void) { req1arg(); return analogRead(arg1); }
numvar func_aw(void) { req2args(); analogWrite(arg1, arg2); return 0; }
numvar func_dr(void) { req1arg(); return digitalRead(arg1); }
numvar func_dw(void) { req2args(); digitalWrite(arg1, arg2); return 0; }
numvar func_er(void) { req1arg(); return eeread(arg1); }
numvar func_ew(void) { req2args(); eewrite(arg1, arg2); return 0; }
numvar func_pinmode(void) { req2args(); pinMode(arg1, arg2); return 0; }
numvar func_pulsein(void) { req3args(); return pulseIn(arg1, arg2, arg3); }
numvar func_snooze(void) { req1arg(); snooze(arg1); return 0; }
numvar func_delay(void) { req1arg(); delay(arg1); return 0; }
numvar func_setBaud(void) { req2args(); setBaud(arg1, arg2); return 0; }

//////////
// Function name dictionary
//
// Function dispatch is accomplished by looking up the proposed function name in this structure, 
// which must be in ascending alphabetical order.
//
// The index at which a matching function name is found is used to reference 
// the function_table defined below to get the argument signature and C function 
// address for the handler.
//
//	MAINTENANCE NOTE: 	This dictionary must be sorted in alpha order 
//						and must be 1:1 with function_table below.
//
prog_char functiondict[] PROGMEM = {
	"abs\0"
	"ar\0"
	"aw\0"
	"baud\0"
	"beep\0"
	"constrain\0"
	"delay\0"
	"dr\0"
	"dw\0"
	"er\0"
	"ew\0"
	"free\0"
	"inb\0"
	"max\0"
	"millis\0"
	"min\0"
	"outb\0"
	"pinmode\0"
	"pulsein\0"
	"random\0"
	"sign\0"
	"snooze\0"
};


// function_table
//
// this must be 1:1 with the symbols above, which in turn must be in alpha order
//
typedef numvar (*bitlash_function)(void);

bitlash_function function_table[] PROGMEM = {
	func_abs ,
	func_ar ,
	func_aw ,
	func_setBaud ,
	func_beep ,
	func_constrain ,
	func_delay ,
	func_dr ,
	func_dw ,
	func_er ,
	func_ew ,
	func_free ,
	func_inb ,
	func_max ,
	(bitlash_function) millis ,
	func_min ,
	func_outb ,
	func_pinmode ,
	func_pulsein ,
	func_random ,
	func_sign ,
	func_snooze 		// last one no comma!
 	};


// Enable USER_FUNCTIONS to include the add_bitlash_function() extension mechanism
// This costs about 256 bytes
//
#define USER_FUNCTIONS

#ifdef USER_FUNCTIONS
#define MAX_USER_FUNCTIONS 6		// increase this if needed, but keep free() > 200 ish
#define USER_FUNCTION_FLAG 0x80

typedef struct {
	char *name;					// pointer to the name
	bitlash_function func_ptr;	// pointer to the implementing function
} user_functab_entry;

byte bf_install_count;			// number of installed functions
user_functab_entry user_functions[MAX_USER_FUNCTIONS];		// the table


//////////
// addBitlashFunction: add a function to the user function table
//
//
//	name: Pointer to a string containing the name for the function, like "myfunc".
//
//		Note: Since the user table is searched last, attempting to redefine an 
//		existing function will fail silently, leading to interesting debugging experience.
//
//	func_ptr: pointer to the implementing C function
//
//	if it weren't built-in, you could add millis() like this:
//		addBitlashFunction("millis", (bitlash_function) millis);
//
// Bitlash uses the typedefs "numvar" and "unumvar" for signed and unsigned values within the
// bitlash calculation engine.  The engine can be configured for 16- or 32-bit calculations
// by changing these definitions.
//
//
// You can easily see the calc engine value size by typing "print -1:x" in Bitlash.
//
// Example:
//		numvar foo_the_function(numvar arg) { return random(arg) * random(arg); }
//		addBitlashFunction("foo", 1, &foo_the_function);
//
//		then in Bitlash you can say:
//		> print foo(22)
//		148
//
void addBitlashFunction(char *name, signed char argsig, bitlash_function func_ptr) {
	if (bf_install_count >= MAX_USER_FUNCTIONS-1) overflow(M_functions);
//	if (myabs(argsig) > MAXARGS) overflow(M_functions); 
	user_functions[bf_install_count].name = name;
//	user_functions[bf_install_count].argsig = argsig;
	user_functions[bf_install_count].func_ptr = func_ptr;	
	bf_install_count++;
}

//////////
// find_user_function: find id in the user function table.  
// return true if found, with the user function token in symval (with USER_FUNCTION_FLAG set)
//
char find_user_function(char *id) {
	symval = 0;
	while (symval < bf_install_count) {
		if (!strcmp(id, user_functions[symval].name)) {
			symval |= USER_FUNCTION_FLAG;
			return 1;
		}
		symval++;
	}
	return 0;
}
#endif		// USER_FUNCTIONS



//////////
// dofunctioncall(): evaluate a function reference
//
// parse the argument list, marshall the arguments and call the function,
// and push its return value, if any, on the value stack
//
void dofunctioncall(byte entry) {
bitlash_function fp;

#ifdef USER_FUNCTIONS
	// Detect and handle a user function: its id has the high bit set
	// we set nargs and fp and fall through to masquerade as a built-in
	if (entry & USER_FUNCTION_FLAG) {
		fp = (bitlash_function) user_functions[entry & 0x7f].func_ptr;
	}
	else
#endif
	// built-in function
	fp = (bitlash_function) pgm_read_word(&function_table[entry]);

	parsearglist();			// parse the arguments
	numvar ret = (*fp)();	// call the function 
	releaseargblock();		// peel off the arguments
	vpush(ret);				// and push the return value
}







