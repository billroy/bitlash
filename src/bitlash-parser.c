/***
	bitlash-parser.c

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

#ifndef UNIX_BUILD
#include "avr/eeprom.h"
#endif

// Interpreter globals
byte fetchtype;		// current script type
numvar fetchptr;	// pointer to current char in script
numvar symval;		// value of current numeric expression

#if !USE_GPIORS
byte sym;			// current input symbol
byte inchar;		// Current parser character
#endif

// Expression result
byte exptype;				// type of expression: s_nval [or s_sval]
numvar expval;				// value of numeric expr or length of string

// Temporary buffer for ids
char idbuf[IDLEN+1];



prog_char strings[] PROGMEM = { 
#ifdef TINY85
	"exp \0unexp \0mssng \0str\0 uflow \0oflow \0\0\0\0exp\0op\0\0eof\0var\0num\0)\0\0eep\0:=\"\0> \0line\0char\0stack\0startup\0id\0prompt\0\r\n\0\0\0"
#else
	"expected \0unexpected \0missing \0string\0 underflow\0 overflow\0^C\0^B\0^U\0exp\0op\0:xby+-*/\0eof\0var\0number\0)\0saved\0eeprom\0:=\"\0> \0line\0char\0stack\0startup\0id\0prompt\0\r\nFunctions:\0oops\0arg\0function\0"
#endif
};

// get the address of the nth message in the table
prog_char *getmsg(byte id) {
	prog_char *msg = strings;
	while (id) { msg += strlen_P(msg) + 1; id--; }
	return msg;
}


#if defined(HARDWARE_SERIAL_TX) || defined(SOFTWARE_SERIAL_TX)
// print the nth string from the message table, e.g., msgp(M_missing);
void msgp(byte id) {
	prog_char *msg = getmsg(id);
	for (;;) {
		char c = pgm_read_byte(msg++);
		if (!c) break;
		spb(c);
	}
}
void msgpl(byte msgid) { msgp(msgid); speol(); }
#endif



// Token type dispatcher: based on initial character in the symbol
#define TOKENTYPES 9
typedef void (*tokenhandler)(void);

void skpwhite(void), parsenum(void), parseid(void), eof(void), badsym(void);
void chrconst(void), litsym(void), parseop(void);

tokenhandler tokenhandlers[TOKENTYPES] = {
	skpwhite,		// 0: whitespace -> skip
	parsenum,		// 1: digit -> number
	parseid,		// 2: letter -> identifier
	eof,			// 3: end of string
	badsym,			// 4: illegal starting char
	chrconst,		// 5: ' -> char const
	0,				// [deprecated/free was 6: " -> string constant]
	litsym,			// 7: sym is inchar itself
	parseop			// 8: single and multi char operators (>, >=, >>, ...) beginning with [&|<>=!+-]
};

//	The chartypes array contains a type code for each ascii character in the range 0-127.
//	The code corresponding to a character specifies which of the token handlers above will
//	be called when the character is seen as the initial character in a symbol.
#define np(a,b) ((a<<4)+b)
prog_char chartypes[] PROGMEM = {    											//    0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
	np(3,4), np(4,4),  np(4,4), np(4,4),  np(4,0), np(0,4),  np(4,0), np(4,4),	//0  NUL SOH STX ETX EOT ENQ ACK BEL BS  HT  LF  VT  FF  CR  SO  SI
	np(4,4), np(4,4),  np(4,4), np(4,4),  np(4,4), np(4,4),  np(4,4), np(4,4),	//1  DLE DC1 DC2 DC3 DC4 NAK SYN ETB CAN EM  SUB ESC FS  GS  RS  US
	np(0,8), np(7,7),  np(4,7), np(8,5),  np(7,7), np(7,8),  np(7,8), np(7,8),	//2   SP  !   "   #   $   %   &   '   (   )   *   +   ,   -   .   slash
	np(1,1), np(1,1),  np(1,1), np(1,1),  np(1,1), np(8,7),  np(8,8), np(8,4),	//3   0   1   2   3   4   5   6   7   8   9   :   ;   <   =   >   ?
	np(4,2), np(2,2),  np(2,2), np(2,2),  np(2,2), np(2,2),  np(2,2), np(2,2),	//4   @   A   B   C   D   E   F   G   H   I   J   K   L   M   N   O
	np(2,2), np(2,2),  np(2,2), np(2,2),  np(2,2), np(2,4),  np(4,4), np(7,2),	//5   P   Q   R   S   T   U   V   W   X   Y   Z   [   \   ]   ^   _
	np(4,2), np(2,2),  np(2,2), np(2,2),  np(2,2), np(2,2),  np(2,2), np(2,2),	//6   `   a   b   c   d   e   f   g   h   i   j   k   l   m   n   o
	np(2,2), np(2,2),  np(2,2), np(2,2),  np(2,2), np(2,7),  np(8,7), np(7,4) 	//7   p   q   r   s   t   u   v   w   x   y   z   {   |   }   ~ DEL
};

// Return the chartype for a given char
byte chartype(byte c) {
	if (c > 127) return 4;	// illegal starting char but allowed in strconst
	byte entry = pgm_read_byte(chartypes + (c/2));
	if (c&1) return entry & 0xf;
	else return (entry >> 4);		// & 0xf;
}

#if 0
byte isdigit(byte c) { return (chartype(c) == 1); }
byte isalpha(byte c) { return (chartype(c) == 2); }
byte isalnum(byte c) { return isalpha(c) || isdigit(c); }
byte tolower(byte c) {
	return ((c >= 'A') && (c <= 'Z')) ? (c - 'A' + 'a') : c;
}
byte is_end(void) { return ((sym == s_eof) || (sym == s_semi)); }
#endif

// Tests on the symbol type
byte isrelop(void) {
	return ((sym == s_lt) || (sym == s_le)
			|| (sym == s_logicaleq) || (sym == s_logicalne)
			|| (sym == s_gt) || (sym == s_ge));
}
byte ishex(char c) { 
	return ((c >= '0') && (c <= '9')) || ((c >= 'a') && (c <= 'f')) || ((c >= 'A') && (c <= 'F')); 
}
byte hexval(char c) {
	if ((c >= '0') && (c <= '9')) return c - '0';
	return tolower(c) - 'a' + 10;
}


#ifdef PARSER_TRACE
byte trace;
#endif


//	Parse the next token from the input stream.
void getsym(void) {

	// dispatch to handler for this type of char
	(*tokenhandlers[chartype(inchar)])();

#ifdef PARSER_TRACE
	if (trace) {
		sp(" sym="); printInteger(sym); sp(" v="); printInteger(symval); spb(' ');
	}
#endif
}


#ifdef PARSER_TRACE
void tb(void) {		// print a mini-trace
	if (!trace) return;
	sp("@");printHex((unsigned long)fetchptr); spb(' ');
	sp("s");printHex(sym); spb(' ');
	sp("i");printHex(inchar); speol();
}
#endif



////////////////////
///
///		Expression evaluation stack
///
#if defined(MEGA)
#define VSTACKLEN 256
#else
#define VSTACKLEN 64
#endif
byte vsptr;			  		// value stack pointer
numvar *arg;				// argument frame pointer
numvar vstack[VSTACKLEN];  	// value stack


#define STRING_POOL
////////////////////
///
///	String Pool
///
///		The string pool lives in the far end of the vstack.
///		It holds string constants parsed from arg blocks.
///		The pointers themselves are passed as arg(n) values.
///		The whole string pool for an argblock is deallocated
///		when the argblock is released.
///
#if defined(STRING_POOL)

char *stringPool;

// push a character into the string pool
void spush(char c) {
	if (stringPool >= (char *) &vstack[vsptr]) overflow(M_string);
	*stringPool++ = c;
}

// push a string into the string pool
void strpush(char *ptr) {
	while (*ptr) spush(*ptr++);
	spush(0);
}

#endif	// STRING_POOL


void vinit(void) { 
	vsptr = VSTACKLEN-1;
	arg = &vstack[vsptr];	// point the argblock at the stack base
	vpush(0);				// push a 0 there so arg(0) is 0 at the top
#if defined(STRING_POOL)
	stringPool = (char *) vstack;	// stringPool starts at unused base of vstack
	*stringPool = 0;				// make it look empty
#endif
}

void vpush(numvar x) {

#if defined(STRING_POOL)
	if ((char *) &vstack[vsptr] < stringPool) overflow(M_exp);
#else
	if (vsptr <= 0) overflow(M_exp);
#endif
	vstack[vsptr--] = x;
}

numvar vpop(void) {
	if (vsptr >= VSTACKLEN-1) underflow(M_exp);
	return vstack[++vsptr];
}

void vop(byte op)  {
numvar x,y;
	x = vpop(); y = vpop();
	switch (op) {
		case s_add:			vpush(y + x);	break;
		case s_sub:			vpush(y - x);	break;
		case s_mul:			vpush(y * x);	break;
		case s_div:			vpush(y / x);	break;
		case s_mod:			vpush(y % x);	break;
		case s_lt:			vpush(y < x);	break;
		case s_gt:			vpush(y > x);	break;
		case s_le:	 		vpush(y <= x);	break;
		case s_ge:	 		vpush(y >= x);	break;
		case s_logicalne: 	vpush(y != x);	break;
		case s_logicaland:	vpush(y && x);	break;
		case s_logicalor:	vpush(y || x);	break;
		case s_logicaleq:	vpush(y == x);	break;
		case s_bitor:		vpush(y | x);	break;
		case s_bitand:		vpush(y & x);	break;
		case s_xor:			vpush(y ^ x);	break;
		case s_shiftleft:	vpush(y << x);	break;
		case s_shiftright:	vpush(y >> x);	break;
		default: 			unexpected(M_op);
	}
}



////////////////////
///
/// Argument Block handling
///

numvar getarg(numvar which) {
	if (which > arg[0]) missing(M_arg);
	return arg[-which];
}


#if 0
//	returns arg value from parent context
//
numvar getparentarg(void) {
	if (arg[0] < 1) underflow(M_arg);
	numvar *parentArg = *(arg-2);
	if (arg[1] > parentArg[0]) overflow(M_arg);
	return parentArg[arg[1]];
}
#endif

void parsearglist(void) {
	vpush((numvar) arg);				// save base of current argblock
#if defined(STRING_POOL)
	vpush((numvar) stringPool);			// save stringPool base for later release
	strpush(idbuf);						// save called function's name as arg[-1]
#endif
	numvar *newarg = &vstack[vsptr];	// move global arg pointer to base of new block
	vpush(0);							// initialize new arg(0) (a/k/a argc) to 0

	if (sym == s_lparen) {
		getsym();		// eat arglist '('
		while ((sym != s_rparen) && (sym != s_eof)) {

#if defined(STRING_POOL)
			if (sym == s_quote) {
				vpush((numvar) stringPool);	// push the string pointer
				parsestring(&spush);		// parse it into the pool
				spush(0);					// and terminate it
				getsym();					// eat closing "
			} else 
#endif
			vpush(getnum());				// push the value
			newarg[0]++;					// bump the count
			if (sym == s_comma) getsym();	// eat arglist ',' and go around
			else break;
		}
		if (sym == s_rparen) getsym();		// eat the ')'
		else expected(M_rparen);
	}
	arg = newarg;		// activate new argument frame
}


// release the top argblock once its execution context has expired
//
void releaseargblock(void) {
	vsptr += arg[0] + 1;				// pop all args en masse, and the count

#if defined(STRING_POOL)
	// deallocate the string pool slab used by this function
	// by popping the saved caller's stringPool
	stringPool = (char *) vpop();
#endif

	arg = (numvar *) vpop();			// pop parent arg frame and we're back
}



// Statement labels
//
//	MUST BE IN ALPHABETICAL ORDER!
//
#ifdef TINY85
prog_char reservedwords[] PROGMEM = { "boot\0if\0run\0stop\0switch\0while\0" };
prog_uchar reservedwordtypes[] PROGMEM = { s_boot, s_if, s_run, s_stop, s_switch, s_while };
#else
prog_char reservedwords[] PROGMEM = { "arg\0boot\0else\0function\0help\0if\0ls\0peep\0print\0ps\0return\0rm\0run\0stop\0switch\0while\0" };
prog_uchar reservedwordtypes[] PROGMEM = { s_arg, s_boot, s_else, s_function, s_help, s_if, s_ls, s_peep, s_print, s_ps, s_return, s_rm, s_run, s_stop, s_switch, s_while };
#endif

// find id in PROGMEM wordlist.  result in symval, return true if found.
byte findindex(char *id, prog_char *wordlist, byte sorted) {
	symval = 0;
	while (pgm_read_byte(wordlist)) {
		int result = strcmp_P(id, wordlist);
		if (!result) return 1;
		else if (sorted && (result < 0)) break;	// only works if list is sorted!
		else {
			symval++;
			wordlist += strlen_P(wordlist) + 1;
		}
	}
	return 0;
}

// return the pin number from a2 or d13
#ifdef TINY85
#define pinnum(id) (id[1] - '0')
#else
byte pinnum(char id[]) {
	//return atoi(id + 1);		// atoi is 60 bytes. this saves 36 net.
	char n = id[1] - '0';
	if (id[2]) n = (n * 10) + id[2] - '0';
	return n;
}
#endif



//////////
//
// Pin alias variables
//
#define PIN_ALIASES 1

#ifdef PIN_ALIASES

#define PV_ANALOG 0x80	// high bit flag to distinguish s_dpin from s_apin
#define PV_MASK 0x7f

prog_char pinnames[] PROGMEM = { 
	"tx\0rx\0led\0vin\0"
};
prog_uchar pinvalues[] PROGMEM = { 
	0, 1, 13, (PV_ANALOG | 1)
};

byte findpinname(char *alias) {
	if (!findindex(alias, (prog_char *) pinnames, 0)) return 0;		// sets symval
	byte pin = pgm_read_byte(pinvalues + symval);
	sym = (pin & PV_ANALOG) ? s_apin : s_dpin;
	symval = pin & PV_MASK;
	return 1;
}
#endif


// Numeric variables
#ifndef NUMVARS
#define NUMVARS 26
#endif
numvar vars[NUMVARS];		// 'a' through 'z'
void assignVar(byte id, numvar value) { vars[id] = value; }
numvar getVar(byte id) { return vars[id]; }
numvar incVar(byte id) { return ++vars[id]; }


// Token handlers to parse the various token types

// Skip to next nonblank and return the symbol therefrom
void skpwhite(void) {
	while (chartype(inchar) == 0) fetchc();
	getsym();
}

// Comment: Skip from // to end of line, return next symbol
void skipcomment(void) {
	while (sym == s_comment) {
		while (inchar && (inchar != '\n') && (inchar != '\r')) fetchc();
		if (!inchar) {
			sym = s_eof;
			return;
		}
		else {
			fetchc();	// eat \r or \n
			getsym();	// and tee up what's next
		}
	}
}

// Handle unexpected character
void badsym(void) {
	unexpected(M_char);
#if !defined(TINY85)
	printHex(inchar);speol();
#endif
}

// Parse a character constant of the form 'c'
void chrconst(void) {
	fetchc();
	symval = inchar;
	sym = s_nval;
	fetchc();
	if (inchar != '\'') expectedchar('\'');
	fetchc();		// consume "
}


prog_char twochartokens[] PROGMEM = { "&&||==!=++--:=>=>><=<<//" };
prog_uchar twocharsyms[] PROGMEM = {
	s_logicaland, s_logicalor, s_logicaleq, s_logicalne, s_incr, 
	s_decr, s_define, s_ge, s_shiftright, s_le, s_shiftleft, s_comment
};

// Parse a one- or two-char operator like >, >=, >>, ...	
void parseop(void) {
	sym = inchar;		// think horse not zebra
	fetchc();			// inchar has second char of token or ??

	prog_char *tk = twochartokens;
	byte index = 0;
	for (;;) {
		byte c1 = pgm_read_byte(tk++);
		if (!c1) return;
		byte c2 = pgm_read_byte(tk++); 

		if ((sym == c1) && (inchar == c2)) {
			sym = (byte) pgm_read_byte(twocharsyms + index);
			fetchc();
			if (sym == s_comment) skipcomment();
			return;
		}
		index++;
	}
}

//	One-char literal symbols, like '*' and '+'.
void litsym(void) {
	sym = inchar;
	fetchc();
}

// End of input
void eof(void) {
	sym = s_eof;
}

// Parse a numeric constant from the input stream
void parsenum(void) {
byte radix;
	radix = 10;
	symval = inchar - '0';
	for (;;) {
		fetchc();
		inchar = tolower(inchar);
		if ((radix == 10) && (symval == 0)) {
			if (inchar == 'x') { radix = 16; continue; }
			else if (inchar == 'b') { radix = 2; continue; }
		}
		if (isdigit(inchar)) {
			inchar = inchar - '0';
			if (inchar >= radix) break;
			symval = (symval*radix) + inchar;
		}
		else if (radix == 16) {
			if ((inchar >= 'a') && (inchar <= 'f'))
				symval = (symval*radix) + inchar - 'a' + 10;
			else break;
		}
		else break;
	}
	sym = s_nval;
}


// Parse an identifier from the input stream
void parseid(void) {
	char c = *idbuf = tolower(inchar);
	byte idbuflen = 1;
	fetchc();
	while (isalnum(inchar) || (inchar == '.') || (inchar == '_')) {
		if (idbuflen >= IDLEN) overflow(M_id);
		idbuf[idbuflen++] = tolower(inchar);
		fetchc();
	}
	idbuf[idbuflen] = 0;

	// do we have a one-char alpha nvar identifier?
	if ((idbuflen == 1) && isalpha(c)) {
		sym = s_nvar;
		symval = c - 'a';
	}
	
	// a pin identifier 'a'digit* or 'd'digit*?
	else if ((idbuflen <= 3) &&
		((c == 'a') || (c == 'd')) && 
		isdigit(idbuf[1]) && (
#if !defined(TINY85)
		isdigit(idbuf[2]) || 
#endif
		(idbuf[2] == 0))) {
		sym = (c == 'a') ? s_apin : s_dpin;
		symval = pinnum(idbuf);
	}

	// reserved word?
	else if (findindex(idbuf, (prog_char *) reservedwords, 1)) {
		sym = pgm_read_byte(reservedwordtypes + symval);	// e.g., s_if or s_while
	}

	// function?
	else if (findindex(idbuf, (prog_char *) functiondict, 1)) sym = s_nfunct;
#ifdef LONG_ALIASES
	else if (findindex(idbuf, (prog_char *) aliasdict, 0)) sym = s_nfunct;
#endif

#ifdef PIN_ALIASES
	else if (findpinname(idbuf)) {;}		// sym and symval are set in findpinname
#endif

#ifdef USER_FUNCTIONS
	else if (find_user_function(idbuf)) sym = s_nfunct;
#endif

	else findscript(idbuf);
}


//////////
//
//	findscript: look up a script, with side effects
//
byte findscript(char *idbuf) {

	// script function in eeprom?
	if ((symval=findKey(idbuf)) >= 0) sym = s_script_eeprom;

	// script function in a file?
	else if (scriptfileexists(idbuf)) sym = s_script_file;

	else {
		sym = s_undef;		// huh?
		return 0;
	}
	return sym;
}


#define ASC_QUOTE		0x22
#define ASC_BKSLASH		0x5C


// Parse a "quoted string" from the input.
//
// Enter with sym = s_quote therefore inchar = first char in string
// Exit with inchar = first char past closing s_quote
//
// Callers will need to call getsym() to resume parsing
//
void parsestring(void (*charFunc)(char)) {

	for (;;) {

		if (inchar == ASC_QUOTE) {				// found the string terminator
			fetchc();							// consume it so's we move along
			break;								// done with the big loop
		}
		else if (inchar == ASC_BKSLASH) {		// bkslash escape conventions per K&R C
			fetchc();
			switch (inchar) {

				// pass-thrus
				case ASC_QUOTE:				break;	// just a dbl quote, move along
				case ASC_BKSLASH:			break;	// just a backslash, move along

				// minor translations
				case 'n': 	inchar = '\n';	break;
				case 't': 	inchar = '\t';	break;
				case 'r':	inchar = '\r';	break;

				case 'x':			// bkslash x hexdigit hexdigit	
					fetchc();
					if (ishex(inchar)) {
						byte firstnibble = hexval(inchar);
						fetchc();
						if (ishex(inchar)) {
							inchar = hexval(inchar) + (firstnibble << 4);
							break;
						}
					}
					unexpected(M_char);
					inchar = 'x';
					break;
			}
		}
		// Process the character we just extracted
		(*charFunc)(inchar);

		fetchc();
		if (!inchar) unexpected(M_eof);		// get next else end of input before string terminator
	}
}



void getexpression(void);

//
//	Recursive descent parser, old-school style.
//
void getfactor(void) {
numvar thesymval = symval;
byte thesym = sym;
	getsym();		// eat the sym we just saved

	switch (thesym) {
		case s_nval:
			vpush(thesymval);
			break;
			
		case s_nvar:
			if (sym == s_equals) {		// assignment, push is after the break;
				getsym();
				assignVar(thesymval, getnum());
			}
			else if (sym == s_incr) {	// postincrement nvar++
				vpush(getVar(thesymval));
				assignVar(thesymval, getVar(thesymval) + 1);
				getsym();
				break;
			}
			else if (sym == s_decr) {	// postdecrement nvar--
				vpush(getVar(thesymval));
				assignVar(thesymval, getVar(thesymval) - 1);
				getsym();
				break;
			}
			vpush(getVar(thesymval));			// both assignment and reference get pushed here
			break;

		case s_nfunct:
			dofunctioncall(thesymval);			// get its value onto the stack
			break;

		// Script-function-returning-value used as a factor
		case s_script_eeprom:				// macro returning value
			callscriptfunction(SCRIPT_EEPROM, findend(thesymval));
			break;

		case s_script_progmem:
			callscriptfunction(SCRIPT_PROGMEM, thesymval);
			break;

		case s_script_file:
			callscriptfunction(SCRIPT_FILE, (numvar) 0);	// name implicitly in idbuf!
			break;

		case s_apin:					// analog pin reference like a0
			if (sym == s_equals) { 		// digitalWrite or analogWrite
				getsym();
				analogWrite(thesymval, getnum());
				vpush(expval);
			}
			else vpush(analogRead(thesymval));
			break;

		case s_dpin:					// digital pin reference like d1
			if (sym == s_equals) { 		// digitalWrite or analogWrite
				getsym();
				digitalWrite(thesymval, getnum());
				vpush(expval);
			}
			else vpush(digitalRead(thesymval));
			break;

		case s_incr:
			if (sym != s_nvar) expected(M_var);
			assignVar(symval, getVar(symval) + 1);
			vpush(getVar(symval));
			getsym();
			break;

		case s_decr:		// pre decrement
			if (sym != s_nvar) expected(M_var);
			assignVar(symval, getVar(symval) - 1);
			vpush(getVar(symval));
			getsym();
			break;

		case s_arg:			// arg(n) - argument value
			if (sym != s_lparen) expectedchar(s_lparen);
			getsym(); 		// eat '('
			vpush(getarg(getnum()));
			if (sym != s_rparen) expectedchar(s_rparen);
			getsym();		// eat ')'
			break;

		case s_lparen:  // expression in parens
			getexpression();
			if (exptype != s_nval) expected(M_number);
			if (sym != s_rparen) missing(M_rparen);
			vpush(expval);
			getsym();	// eat the )
			break;

		//
		// The Family of Unary Operators, which Bind Most Closely to their Factor
		//
		case s_add:			// unary plus (like +3) is kind of a no-op
			getfactor();	// scan a factor and leave its result on the stack
			break;			// done
	
		case s_sub:			// unary minus (like -3)
			getfactor();
			vpush(-vpop());	// similar to above but we adjust the stack value
			break;
	
		case s_bitnot:
			getfactor();
			vpush(~vpop());
			break;
	
		case s_logicalnot:
			getfactor();
			vpush(!vpop());
			break;

		case s_bitand:		// &var gives address-of-var; &macro gives eeprom address of macro
			if (sym == s_nvar) vpush((numvar) &vars[symval]);
			else if (sym == s_script_eeprom) vpush(symval);
			else expected(M_var);
			getsym();		// eat the var reference
			break;

		case s_mul:			// *foo is contents-of-address-foo; *foo=bar is byte poke assignment

/*****
// what is really acceptable for an lvalue here? ;)
//	*y = 5 is failing now by assigning 5 to y before the * is dereferenced
//	due to calling getfactor
//	everything else works :(
*****/
			getfactor();
#if 0
			if (sym == s_equals) {
				getsym();	// eat '='
				getexpression();
				* (volatile byte *) vpop() = (byte) expval;
				vpush((numvar) (byte) expval);
			} 
			else 
#endif
			vpush((numvar) (* (volatile byte *) vpop()));
			break;

		default: 
			unexpected(M_number);
	}

}


// parseReduce via a template
// saves 100 bytes but eats stack like crazy
// while we're ram-starved this stays off
//#define USE_PARSEREDUCE

#ifdef USE_PARSEREDUCE
void parseReduce(void (*parsefunc)(void), byte sym1, byte sym2, byte sym3) {
	(*parsefunc)();
	while ((sym == sym1) || (sym == sym2) || (sym == sym3)) {
		byte op = sym;
		getsym();
		(*parsefunc)();
		vop(op);
	}
}
#endif


void getterm(void) {
#ifdef USE_PARSEREDUCE
	parseReduce(&getfactor, s_mul, s_div, s_mod);
#else
	getfactor();
	while ((sym == s_mul) || (sym == s_div) || (sym == s_mod)) {
		byte op = sym;
		getsym();
		getfactor();
		vop(op);
	}
#endif
}

void getsimpexp(void) {
#ifdef USE_PARSEREDUCE
	parseReduce(&getterm, s_add, s_sub, s_sub);
#else
	getterm();
	while ((sym == s_add) || (sym == s_sub)) {
		byte op = sym;
		getsym();
		getterm();
		vop(op);
	}
#endif
}

void getshiftexp(void) {
#ifdef USE_PARSEREDUCE
	parseReduce(&getsimpexp, s_shiftright, s_shiftleft, s_shiftleft);
#else
	getsimpexp();
	while ((sym == s_shiftright) || (sym == s_shiftleft)) {
		byte op = sym;
		getsym();
		getsimpexp();
		vop(op);
	}
#endif
}

void getrelexp(void) {
	getshiftexp();
	while (isrelop()) {
		byte op = sym;
		getsym();
		getshiftexp();
		vop(op);
	}
}

void getbitopexp(void) {
#ifdef USE_PARSEREDUCE
	parseReduce(&getrelexp, s_bitand, s_bitor, s_xor);
#else
	getrelexp();
	while ((sym == s_bitand) || (sym == s_bitor) || (sym == s_xor)) {
		byte op = sym;
		getsym();
		getrelexp();
		vop(op);
	}
#endif
}

// Parse an expression.  Result to expval.
void getexpression(void) {

#ifdef USE_PARSEREDUCE
	parseReduce(&getbitopexp, s_logicaland, s_logicalor, s_logicalor);
#else
	getbitopexp();
	while ((sym == s_logicaland) || (sym == s_logicalor)) {
		byte op = sym;
		getsym();
		getbitopexp();
		vop(op);
	}
#endif
	exptype = s_nval;
	expval = vpop();
}


// Get a number from the input stream.  Result to expval.
numvar getnum(void) {
	getexpression();
	if (exptype != s_nval) expected(M_number);
	return expval;
}

