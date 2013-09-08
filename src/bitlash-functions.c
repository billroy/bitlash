/***
	bitlash-functions.c

	Bitlash is a tiny language interpreter that provides a serial port shell environment
	for bit banging and hardware hacking.

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

// syntactic sugar for func_handlers()
#if 0	// 15022 bytes
#define arg1 getarg(1)
#define arg2 getarg(2)
#define arg3 getarg(3)
#endif

// 14772 vs 15022
extern numvar *arg;
#define arg1 arg[-1]
#define arg2 arg[-2]
#define arg3 arg[-3]
#define arg4 arg[-4]
#define arg5 arg[-5]

void reqargs(byte n) { if (arg[0] < n) missing(M_arg); }

// this costs 500 bytes more!
//#define reqargs(n) { if (arg[0] < n) missing(M_arg); }

///////////////////////
// FUNCTION HANDLERS
//

// The native tone() routine first included in Arduino 0018 is quite large, 
//		about 1k differential: 15196 - 14110
// It is, however, for the moment quite a bit more frequency-accurate
//
//#define USE_NATIVE_TONE
#ifdef USE_NATIVE_TONE
void make_beep(unumvar pin, unumvar frequency, unumvar duration) {
	tone(pin, frequency, duration);
	delay(duration);
}
#else
numvar func_beep(void) { 		// unumvar pin, unumvar frequency, unumvar duration)
	reqargs(3);
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
	return 0;
}
#endif

numvar func_free(void) {
#if defined(UNIX_BUILD)
	return 1000L;
#elif defined(ARM_BUILD)
	return 1000L;
#else
	numvar ret;
	// from http://forum.pololu.com/viewtopic.php?f=10&t=989&view=unread#p4218
	extern int __bss_end;
	return ((int)&ret) - ((int)&__bss_end);
#endif
}

#if 0
void zapheap(void) {
int heaptop;
extern int __bss_end;
	//Serial.println((int) &__bss_end, DEC);
	memset(((char *)&__bss_end)+1, 0xbb, (&heaptop - &__bss_end) - 6);
}
#endif

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
	reqargs(1);
	deadbeef_seed = (deadbeef_seed << 7) ^ ((deadbeef_seed >> 25) + deadbeef_beef);
	ret = ((numvar) deadbeef_seed & 0x7fffffff) % arg1;
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
numvar func_inb(void) { reqargs(1); return *(volatile byte *) arg1; }
numvar func_outb(void) { reqargs(2); *(volatile byte *) arg1 = (byte) arg2; return 0;}
numvar func_abs(void) { reqargs(1); return arg1 < 0 ? -arg1 : arg1; }
numvar func_sign(void) {
	reqargs(1);
	if (arg1 < 0) return -1;
	if (arg1 > 0) return 1;
	return 0;
}
numvar func_min(void) { reqargs(2); return (arg1 < arg2) ? arg1 : arg2; }
numvar func_max() { reqargs(2); return (arg1 > arg2) ? arg1 : arg2; }
numvar func_constrain(void) {
	reqargs(3);
	if (arg1 < arg2) return arg2;
	if (arg1 > arg3) return arg3;
	return arg1;
}
numvar func_ar(void) { reqargs(1); return analogRead(arg1); }
numvar func_aw(void) { reqargs(2); analogWrite(arg1, arg2); return 0; }
numvar func_dr(void) { reqargs(1); return digitalRead(arg1); }
numvar func_dw(void) { reqargs(2); digitalWrite(arg1, arg2); return 0; }
numvar func_er(void) { reqargs(1); return eeread(arg1); }
numvar func_ew(void) { reqargs(2); eewrite(arg1, arg2); return 0; }
numvar func_pinmode(void) { reqargs(2); pinMode(arg1, arg2); return 0; }
numvar func_pulsein(void) { reqargs(3); return pulseIn(arg1, arg2, arg3); }
numvar func_snooze(void) { reqargs(1); snooze(arg1); return 0; }
numvar func_delay(void) { reqargs(1); delay(arg1); return 0; }

#if !defined(TINY_BUILD)
numvar func_setBaud(void) { reqargs(2); setBaud(arg1, arg2); return 0; }
#endif

//numvar func_map(void) { reqargs(5); return map(arg1, arg2, arg3, arg4, arg5); }
//numvar func_shiftout(void) { reqargs(4); shiftOut(arg1, arg2, arg3, arg4); return 0; }

numvar func_bitclear(void) { reqargs(2); return arg1 & ~((numvar)1 << arg2); }
numvar func_bitset(void) { reqargs(2); return arg1 | ((numvar)1 << arg2); }
numvar func_bitread(void) { reqargs(2); return (arg1 & ((numvar)1 << arg2)) != 0; }
numvar func_bitwrite(void) { reqargs(3); return arg3 ? func_bitset() : func_bitclear(); }

numvar func_getkey(void) {
	if (getarg(0) > 0) sp((char *) getarg(1));
	while (!serialAvailable()) {;}		// blocking!
	return (numvar) serialRead();
}

numvar func_getnum(void) {
	numvar num = 0;
	if (getarg(0) > 0) sp((char *) getarg(1));
	for (;;) {
		while (!serialAvailable()) {;}	// blocking!
		int k = serialRead();
		if ((k == '\r') || (k == '\n')) {
			speol();
			return num;
		}
		else if ((k >= '0') && (k <= '9')) {
			num = num * 10L + (long) (k - '0');
		}
		else if (k == '-') num = -num;
		else if ((k == 8) || (k == 0x7f)) {
			if (num != 0) {
				num /= 10L;
				spb(8); spb(' '); spb(8);
			}
		}
		else {
			spb(7);	// beep
			continue;
		}
		spb(k);			// else echo what we ate
	}
}

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
#if defined(TINY_BUILD)
const prog_char functiondict[] PROGMEM = {
//	"abs\0"
//	"ar\0"
//	"aw\0"
//	"baud\0"
//	"bc\0"
//	"beep\0"
//	"br\0"
//	"bs\0"
//	"bw\0"
//	"constrain\0"
	"delay\0"
//	"dr\0"
//	"dw\0"
//	"er\0"
//	"ew\0"
	"free\0"
//	"inb\0"
//	"map\0"
//	"max\0"
	"millis\0"
//	"min\0"
//	"outb\0"
	"pinmode\0"
//	"printf\0"
//	"pulsein\0"
//	"random\0"
//	"shiftout\0"
//	"sign\0"
	"snooze\0"
};

#else		// standard function set

const prog_char functiondict[] PROGMEM = {
	"abs\0"
	"ar\0"
	"aw\0"
	"baud\0"
	"bc\0"
	"beep\0"
	"br\0"
	"bs\0"
	"bw\0"
	"constrain\0"
	"delay\0"
	"dr\0"
	"dw\0"
	"er\0"
	"ew\0"
	"free\0"
	"getkey\0"
	"getnum\0"
	"inb\0"
	"isstr\0"
//	"map\0"
	"max\0"
	"millis\0"
	"min\0"
	"outb\0"
	"pinmode\0"
	"printf\0"
	"pulsein\0"
	"random\0"
//	"shiftout\0"
	"sign\0"
	"snooze\0"
};
#endif



// function_table
//
// this must be 1:1 with the symbols above, which in turn must be in alpha order
//
#if defined(TINY_BUILD)
const bitlash_function function_table[] PROGMEM = {
//	func_abs,
//	func_ar,
//	func_aw,
//	func_setBaud,
//	func_bitclear,
//	func_beep,
//	func_bitread,
//	func_bitset,
//	func_bitwrite,
//	func_constrain,
	func_delay,
//	func_dr,
//	func_dw,
//	func_er,
//	func_ew,
	func_free,
//	func_inb,
//	func_map,
//	func_max,
	(bitlash_function) millis,
//	func_min,
//	func_outb,
	func_pinmode,
//	func_printf,
//	func_pulsein,
//	func_random,
//	func_shiftout,
//	func_sign,
	func_snooze 		// last one no comma!
 };

#else		// standard function set

const bitlash_function function_table[] PROGMEM = {
	func_abs,
	func_ar,
	func_aw,
	func_setBaud,
	func_bitclear,
	func_beep,
	func_bitread,
	func_bitset,
	func_bitwrite,
	func_constrain,
	func_delay,
	func_dr,
	func_dw,
	func_er,
	func_ew,
	func_free,
	func_getkey,
	func_getnum,
	func_inb,
	isstring,
//	func_map,
	func_max,
	(bitlash_function) millis,
	func_min,
	func_outb,
	func_pinmode,
	func_printf,
	func_pulsein,
	func_random,
//	func_shiftout,
	func_sign,
	func_snooze 		// last one no comma!
 };
#endif

// Enable USER_FUNCTIONS to include the add_bitlash_function() extension mechanism
// This costs about 256 bytes
//
#if !defined(TINY_BUILD)
#define USER_FUNCTIONS
#endif

#ifdef USER_FUNCTIONS
#define MAX_USER_FUNCTIONS 20		// increase this if needed, but keep free() > 200 ish
#define USER_FUNCTION_FLAG 0x80

typedef struct {
	const char *name;					// pointer to the name
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
// Example:
//		numvar foo_the_function(void) { return random(arg(1)) * random(arg(2)); }
//		addBitlashFunction("foo", &foo_the_function);
//
//		then in Bitlash you can say:
//		> print foo(22,33)
//		148
//
void addBitlashFunction(const char *name, bitlash_function func_ptr) {
	if (bf_install_count >= MAX_USER_FUNCTIONS) overflow(M_functions);
	user_functions[bf_install_count].name = name;
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

//////////
//
// show_user_functions: display a list of registered user functions
//
void show_user_functions(void) {
byte i;
	for (i=0; i < bf_install_count; i++) {
		sp(user_functions[i].name);
		spb(' ');
	}
	speol();
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
#ifdef UNIX_BUILD
	fp = function_table[entry];
#else
	fp = (bitlash_function) pgm_read_word(&function_table[entry]);
#endif

	parsearglist();			// parse the arguments
	numvar ret = (*fp)();	// call the function 
	releaseargblock();		// peel off the arguments
	vpush(ret);				// and push the return value
}
