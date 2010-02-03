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
#endif

numvar get_free_memory(void) {
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
unumvar dbrandom(unumvar arg1) {
unumvar ret;
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
unumvar inb(unumvar port) {
	return *(volatile byte *) port;
}

void outb(unumvar port, unumvar value) {
	*(volatile byte *) port = (byte) value;
}

/////////////////////////////
// abs() and sign() replacements
//
// 34 bytes for these
//
numvar myabs(numvar num) { return num < 0 ? -num : num; }
numvar mysign(numvar num) {
	//return num < 0 ? -1 : num > 0 ? 1 : 0;
	if (num < 0) return -1;
	if (num > 0) return 1;
	return 0;
}
numvar mymin(numvar num1, numvar num2) {
	return (num1 < num2) ? num1 : num2;
}
numvar mymax(numvar num1, numvar num2) {
	return (num1 > num2) ? num1 : num2;
}
numvar myconstrain(numvar val, numvar lo, numvar hi) {
	if (val < lo) return lo;
	if (val > hi) return hi;
	return val;
}

// 6 byte signature fix for millis -> unumvar
unumvar millisUnumvar(void) { return (unumvar) millis(); }

numvar myar(numvar pin) { return analogRead(pin); }
void myaw(numvar pin, numvar value) { analogWrite(pin, value); }

numvar mydr(numvar pin) { return digitalRead(pin); }
void mydw(numvar pin, numvar value) { digitalWrite(pin, value); }

numvar myer(numvar pin) { return eeread(pin); }
void myew(numvar pin, numvar value) { eewrite(pin, value); }

void mypinmode(numvar pin, numvar mode) { pinMode(pin, mode); }
unumvar mypulsein(numvar pin, numvar value, numvar timeout) { return pulseIn(pin, value, timeout); }


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
typedef void (*bitlash_function)(void);
typedef struct {
	signed char argsig;				// <0 means no return value
	bitlash_function func_ptr;		// pointer to the implementing function
} functab_entry;

functab_entry function_table[] PROGMEM = {
	{ /* f_abs,		*/		1,		(bitlash_function) myabs },
	{ /* f_ar,		*/		1,		(bitlash_function) myar },
	{ /* f_aw,		*/		-2,		(bitlash_function) myaw },
	{ /* f_baud,	*/		2,		(bitlash_function) setBaud },
	{ /* f_beep,	*/		-3, 	(bitlash_function) beep },
	{ /* f_con,		*/		3,		(bitlash_function) myconstrain },
	{ /* f_delay,	*/		-1,		(bitlash_function) delay },
	{ /* f_dr		*/		 1,		(bitlash_function) mydr },
	{ /* f_dw,		*/		-2, 	(bitlash_function) mydw },
	{ /* f_er,		*/		1,		(bitlash_function) myer },
	{ /* f_ew,		*/		-2,		(bitlash_function) myew },
	{ /* f_free,	*/		0,		(bitlash_function) get_free_memory },
	{ /* f_inb,		*/		1,		(bitlash_function) inb },
	{ /* f_max,		*/		2,  	(bitlash_function) mymax },
	{ /* f_millis,	*/		0,		(bitlash_function) millisUnumvar },
	{ /* f_min,		*/		2,		(bitlash_function) mymin },
	{ /* f_outb,	*/		-2, 	(bitlash_function) outb },
	{ /* f_pinmode,	*/		-2, 	(bitlash_function) mypinmode },
	{ /* f_pulsein, */		3,		(bitlash_function) mypulsein },
	{ /* f_random,	*/		1,		(bitlash_function) dbrandom },
	{ /* f_sign,	*/		1,		(bitlash_function) mysign },
	{ /* f_snooze,	*/		-1,		(bitlash_function) snooze }		// last one no comma!
};

// Increase MAXARGS if you add a function requiring more than 3 arguments
// 	Be cautious, though, since the arg block lives on the stack (since function calls nest, right?)
//	I deprecated map() to avoid going to 5, for example, since
//	each slot you add eats stack in getfunction() to the tune of:
// 		sizeof(numvar) * depth of nesting of function calls
//
#define MAXARGS 3	// NOTE: this is hardcoded for an optimization; read below for more repairs if you change it


// Enable USER_FUNCTIONS to include the add_bitlash_function() extension mechanism
// This costs about 256 bytes
//
#define USER_FUNCTIONS

#ifdef USER_FUNCTIONS
#define MAX_USER_FUNCTIONS 6		// increase this if needed, but keep free() > 200 ish
#define USER_FUNCTION_FLAG 0x80

typedef struct {
	char *name;					// pointer to the name
	signed char argsig;			// argument count; <0 means no return value
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
//	argsig:	argument signature: number of arguments, times -1 if no return value
//		eg: millis is 0		[void millis(void)]
//		eg: delay is -1  	[void delay(howlong)]
//		eg: dr is 1			[numvar dr(numvar pin)]
//		eg: dw is -2		[void dw(numvar pin, numvar value)]  takes 2, returns none is -2
//
//	func_ptr: pointer to the implementing C function
//
//	if it weren't built-in, you could add millis() like this:
//		addBitlashFunction("millis", 0, (bitlash_function) millis);
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
	if (myabs(argsig) > MAXARGS) overflow(M_functions); 
	user_functions[bf_install_count].name = name;
	user_functions[bf_install_count].argsig = argsig;
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
// getfunction(): evaluate a function reference
//
// parse the argument list, marshall the arguments and call the function,
// and push its return value, if any, on the value stack
//
void getfunction(byte entry) {

// Function argument storage
byte argct = 0;
signed char nargs, absnargs;
void (*fp)();
numvar args[MAXARGS];

#ifdef USER_FUNCTIONS
	// Detect and handle a user function: its id has the high bit set
	// we set nargs and fp and fall through to masquerade as a built-in
	if (entry & USER_FUNCTION_FLAG) {
		nargs = user_functions[entry & 0x7f].argsig;
		fp = 	user_functions[entry & 0x7f].func_ptr;
	}
	else
#endif
	{
		// built-in function: get argument count and return type; nargs < 0 means noret
		nargs = pgm_read_byte(&function_table[entry].argsig);
		fp = (void (*)()) pgm_read_word(&function_table[entry].func_ptr);
	}

	// separate the arg count from the return value flag
	absnargs = (nargs < 0) ? -nargs : nargs;

	// Parse argument list
	if (sym == s_lparen) {
		getsym();		// eat arglist '('
		while ((sym != s_rparen) && (argct < absnargs)) {
			args[argct++] = getnum();
			if (sym == s_comma) getsym();	// eat arglist ',' and go around
			else break;
		}
		if (sym != s_rparen) expected(M_rparen);
		if (argct != absnargs) expected(M_number);
		getsym();	// eat arglist ')'
	}
	else if (absnargs) expectedchar('(');	// noargs: may omit ()

	// looks yucky but saves 80 bytes to do the indexing just once
	numvar arg1 = args[0];
	numvar arg2 = args[1];
	numvar arg3 = args[2];
	//numvar arg4 = args[3];
	//numvar arg5 = args[4];

	// there are days...
	typedef void (*func_0args_noretval)(void);
	typedef void (*func_1args_noretval)(unumvar);
	typedef void (*func_2args_noretval)(unumvar, unumvar);
	typedef void (*func_3args_noretval)(unumvar, unumvar, unumvar);
	typedef unumvar (*func_0args_retval)(void);
	typedef unumvar (*func_1args_retval)(unumvar);
	typedef unumvar (*func_2args_retval)(unumvar, unumvar);
	typedef unumvar (*func_3args_retval)(unumvar, unumvar, unumvar);


	// Return value pushed at the bottom of the switch
	numvar ret=0;
	switch(nargs) {
		case -1:	(*(func_1args_noretval)fp)(arg1);				break;
		case -2:	(*(func_2args_noretval)fp)(arg1, arg2);			break;
		case -3:	(*(func_3args_noretval)fp)(arg1, arg2, arg3);	break;
		case 0:		ret = (*(func_0args_retval)fp)();				break;	// for now, 0 means noarg + retval
		case 1:		ret = (*(func_1args_retval)fp)(arg1);			break;
		case 2:		ret = (*(func_2args_retval)fp)(arg1,arg2);		break;
		case 3:		ret = (*(func_3args_retval)fp)(arg1,arg2,arg3);	break;
		default: 	unexpected(M_id);
	}
	vpush(ret);
}







