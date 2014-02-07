/***
	bitlash-private.h - Private API

	Bitlash is a tiny language interpreter that provides a serial port shell environment
	for bit banging and hardware hacking.

	See the file README for documentation.

	Bitlash lives at: http://bitlash.net
	The author can be reached at: bill@bitlash.net

	Copyright (C) 2008-2013 Bill Roy

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
#ifndef _BITLASH_PRIVATE_H
#define _BITLASH_PRIVATE_H

// bitlash-public.h declares public functions and types
// bitlash-private.h declares private (internal) functions and types
// bitlash-config.h contains (sometimes tweakable) build parameters
#include "bitlash-public.h"

#if defined(AVR_BUILD)
#include "avr/pgmspace.h"
#endif

#include <string.h>
#include <ctype.h>
#include <setjmp.h>

/////////////////////////////////////////////
// bitlash-builtins.c
//
void displayBanner(void);
byte findbuiltin(const char *name);

/////////////////////////////////////////////
// bitlash-cmdline.c
//
void initlbuf(void);
void pointToError(void);
void cmd_help(void);

extern char *lbufptr;
extern char lbuf[LBUFLEN];

/////////////////////////////////////////////
// bitlash-eeprom.c
//
int findKey(const char *key);				// return location of macro keyname in EEPROM or -1

int findoccupied(int);
int findend(int);
void eeputs(int);

#define EMPTY ((uint8_t)255)
#define STARTDB 0
#define FAIL ((int)-1)

/////////////////////////////////////////////
// bitlash-error.c
//
extern jmp_buf env;
#define X_EXIT 1
void fatal2(char, char);
void overflow(byte);
void underflow(byte);
void expected(byte);
void expectedchar(byte);
void unexpected(byte);
void missing(byte);
void oops(int);						// fatal exit


/////////////////////////////////////////////
// bitlash-functions.c
//
void show_user_functions(void);
char find_user_function(const char *id);

void dofunctioncall(byte);
numvar func_free(void);
void make_beep(unumvar, unumvar, unumvar);

extern const prog_char functiondict[] PROGMEM;
extern const prog_char aliasdict[] PROGMEM;

void stir(byte);

/////////////////////////////////////////////
// bitlash-program.c
//
extern char startup[];


/////////////////////////////////////////////
// bitlash-serial.c
//
void printIntegerInBase(unumvar, uint8_t, numvar, byte);
void printInteger(numvar,numvar, byte);
void printHex(unumvar);
void printBinary(unumvar);
void spb(char c);
void sp(const char *);
void speol(void);

numvar func_printf(void);
numvar func_printf_handler(byte,byte);
void cmd_print(void);

#ifdef SOFTWARE_SERIAL_TX
numvar setBaud(numvar, unumvar);
void resetOutput(void);
#endif

#ifdef ARDUINO
void chkbreak(void);
void cmd_print(void);
#endif

// The Stream where input is read from and print writes to when there is
// not output handler set.
#ifndef DEFAULT_CONSOLE_ONLY
extern Stream *blconsole;
#else
// Console is fixed to DEFAULT_CONSOLE, so make blconsole a unchangeable
// pointer to DEFAULT_CONSOLE. By also copying the type of
// DEFAULT_CONSOLE, the compiler can optimize away all vtable operations
// for minimal code overhead
__typeof__(DEFAULT_CONSOLE) * const blconsole = &DEFAULT_CONSOLE;
#endif

// The Print object where the print command normally goes (e.g. when not
// redirected with print #10: "foo")
#ifdef SERIAL_OVERRIDE
extern Print *bloutdefault;
#else
// SERIAL_OVERRIDE is disabled, so print redirection should always just
// reset blout to blconsole. Using this define, we essentially make both
// identifiers refer to the same variable.
#define bloutdefault blconsole
#endif

// The Print object where the print command goes right now
#ifdef SOFTWARE_SERIAL_TX
extern Print *blout;
#else
// SOFTWARE_SERIAL_TX is disabled, so printing should always just go to
// bloutdefault.  Using this define, we essentiallyl make both
// identifiers refer to the same variable.
#define blout bloutdefault
#endif

// Note that if SOFTWARE_SERIAL_TX and SERIAL_OVERRIDE are disabled,
// then blconsole, bloutdefault and blout are all effectively the same
// variable.

/////////////////////////////////////////////
// bitlash-taskmgr.c
//
void initTaskList(void);
void runBackgroundTasks(void);
void stopTask(byte);
void startTask(int, numvar);
void snooze(unumvar);
void showTaskList(void);
unsigned long millisUntilNextTask(void);
extern byte background;
extern byte curtask;
extern byte suspendBackground;


/////////////////////////////////////////////
// eeprom.c
//
void eewrite(int, byte);
byte eeread(int);
void eraseentry(const char *id);
void cmd_function(void);
void cmd_ls(void);
void cmd_peep(void);

#if defined(AVR_BUILD)
// they must live off piste due to aggressive compiler inlining.
void eewrite(int, byte) __attribute__((noinline));
byte eeread(int) __attribute__((noinline));
#endif

#if defined(ARM_BUILD)
extern char virtual_eeprom[];
void eeinit(void);
#endif


/////////////////////////////////////////////
// bitlash-interpreter.c
//
numvar getstatementlist(void);
void domacrocall(int);


/////////////////////////////////////////////
// bitlash-parser.c
//
void vinit(void);							// init the value stack
void vpush(numvar);							// push a numvar on the stack
numvar vpop(void);							// pop a numvar
extern byte vsptr;
#define vsempty() vsptr==0
extern numvar *arg;								// argument frame pointer

// parse context types
#define SCRIPT_NONE		0
#define SCRIPT_RAM 		1
#define SCRIPT_PROGMEM 	2
#define SCRIPT_EEPROM 	3
#define SCRIPT_FILE		4

byte scriptfileexists(const char *);
numvar execscript(byte, numvar, const char *);
void callscriptfunction(byte, numvar);

typedef struct {
	numvar fetchptr;
	byte fetchtype;
} parsepoint;

void markparsepoint(parsepoint *);
void returntoparsepoint(parsepoint *, byte);
void primec(void);
void fetchc(void);
void getsym(void);
void traceback(void);

const prog_char *getmsg(byte);
void parsestring(void (*)(char));
void msgp(byte);
void msgpl(byte);
numvar getnum(void);
void calleeprommacro(int);
void getexpression(void);
byte hexval(char);
byte is_end(void);
numvar isstring(void);
void releaseargblock(void);
void parsearglist(void);
extern const prog_char reservedwords[];



// Interpreter globals
extern byte fetchtype;		// current script type
extern numvar fetchptr;		// pointer to current char in input buffer
extern numvar symval;		// value of current numeric expression

#if defined(AVR_BUILD) && defined(GPIOR0) && defined(GPIOR1)
#define USE_GPIORS
#define sym GPIOR0
#define inchar GPIOR1
#else
extern byte sym;			// current input symbol
extern byte inchar;		// Current parser character
#endif

#ifdef PARSER_TRACE
extern byte trace;
void tb(void);
#endif


// Expression result
extern byte exptype;				// type of expression: s_nval [or s_sval]
extern numvar expval;				// value of numeric expr or length of string

// Temporary buffer for ids
#define IDLEN 12
extern char idbuf[IDLEN+1];

/////////////////////////////////////////////
// bitlash-instream.c
//

#if defined(SDFILE) || defined(UNIX_BUILD)
numvar sdwrite(const char *filename, const char *contents, byte append);
#endif

/////////////////////////////////////////////
// bitlash-unix-file.c
//
#if defined(UNIX_BUILD)
numvar exec(void);
numvar sdls(void);
numvar sdexists(void);
numvar sdrm(void);
numvar sdcreate(void);
numvar sdappend(void);
numvar sdcd(void);
numvar sdmd(void);
numvar func_pwd(void);
#endif

// Strings live in PROGMEM to save ram
//
#define M_expected		0
#define M_unexpected	1
#define M_missing		2
#define M_string		3
#define M_underflow		4
#define M_overflow		5
#define M_ctrlc			6
#define M_ctrlb			7
#define M_ctrlu			8
#define M_exp			9
#define M_op			10
#define M_pfmts			11
#define M_eof			12
#define M_var			13
#define M_number		14
#define M_rparen		15
#define M_saved			16
#define M_eeprom		17
#define M_defmacro		18
#define M_prompt		19
#define M_char			20
#define M_stack			21
#define M_startup		22
#define M_id			23
#define M_promptid		24
#define M_functions		25
#define M_oops			26
#define M_arg			27
#define M_function		28


//	Names for symbols
//
//	Each symbol in the grammar is parsed to a unique symval enumerated here
//
//	One character symbols take their ascii value as their symval
//	Complex symbols have the high bit set so start at 128 (0x80)
//
#define s_eof			0
#define s_undef			(0 | 0x80)
#define s_nval			(1 | 0x80)
#define s_sval			(2 | 0x80)
#define s_nvar			(3 | 0x80)
#define s_le			(4 | 0x80)
#define s_ge			(5 | 0x80)
#define s_logicaland	(6 | 0x80)
#define s_logicalor		(7 | 0x80)
#define s_logicaleq		(8 | 0x80)
#define s_logicalne		(9 | 0x80)
#define s_shiftleft		(10 | 0x80)
#define s_shiftright	(11 | 0x80)
#define s_incr			(12 | 0x80)
#define s_decr			(13 | 0x80)
#define s_nfunct		(14 | 0x80)
#define s_if			(15 | 0x80)
#define s_while			(16 | 0x80)
#define s_apin			(17 | 0x80)
#define s_dpin			(18 | 0x80)
#define s_define		(19 | 0x80)
#define s_function		(20 | 0x80)
#define s_rm			(21 | 0x80)
#define s_run			(22 | 0x80)
#define s_ps			(23 | 0x80)
#define s_stop			(24 | 0x80)
#define s_boot			(25 | 0x80)
#define s_peep			(26 | 0x80)
#define s_help			(27 | 0x80)
#define s_ls			(28 | 0x80)
#define s_print			(29 | 0x80)
#define s_switch		(30 | 0x80)
#define s_return		(31 | 0x80)
#define s_returning		(32 | 0x80)
#define s_arg			(33 | 0x80)
#define s_else			(34 | 0x80)
#define s_script_eeprom	(35 | 0x80)
#define s_script_progmem (36 | 0x80)
#define s_script_file	(37 | 0x80)
#define s_comment		(38 | 0x80)


// Names for literal symbols: these one-character symbols 
// are represented by their 7-bit ascii char code
#define s_semi			';'
#define s_add			'+'
#define s_sub			'-'
#define s_mul			'*'
#define s_div			'/'
#define s_mod			'%'
#define s_lparen		'('
#define s_rparen		')'
#define s_dot			'.'
#define s_lt			'<'
#define s_gt			'>'
#define s_equals		'='
#define s_bitand		'&'
#define s_bitor			'|'
#define s_comma			','
#define s_bitnot		'~'
#define s_logicalnot	'!'
#define s_xor			'^'
#define s_colon			':'
#define s_pound			'#'
#define s_quote			'"'
#define s_dollars		'$'
#define s_lcurly		'{'
#define s_rcurly		'}'


#endif	// defined _BITLASH_H

