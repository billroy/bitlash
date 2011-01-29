/***
	bitlash-interpreter.c

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


// Turn HEX_UPLOAD on to enable the hex file EEPROM uploader
// It costs 78 bytes of flash
//
//#define HEX_UPLOAD
#ifdef HEX_UPLOAD
int gethex(byte count) {
int value = 0;
	while (count--) {
		value = (value << 4) + hexval(inchar);
		fetchc();
	}
	return value;
}
#endif


void nukeeeprom(void) {
	initTaskList();		// stop any currently running background tasks
	int addr = STARTDB;
	while (addr <= ENDDB) eewrite(addr++, EMPTY);
}

void reboot(void) {
	// This is recommended but does not work on Arduino
	// Reset_AVR();
	void (*bootvec)(void) = 0; (*bootvec)(); 	// we jump through 0 instead
}

void skipbyte(char) {;}

// Skip a statement without executing it
//
// { stmt; stmt; }
// stmt;
//
void skipstatement(void) {
signed char nestlevel = 0;

#ifdef PARSER_TRACE
	if (trace) sp("SKP[");
#endif

	// Skip a statement list in curly braces: { stmt; stmt; stmt; }
	// Eat until the matching s_rcurly
	if (sym == s_lcurly) {
		getsym();	// eat "{"
		while (sym != s_eof) {
			if (sym == s_lcurly) ++nestlevel;
			else if (sym == s_rcurly) {
				if (nestlevel <= 0) {
					getsym(); 	// eat "}"
					break;
				}
				else --nestlevel;
			}
			else if (sym == s_quote) parsestring(&skipbyte);
			getsym();
		}
	}

	// skipping the if statement is a little tricky
	else if (sym == s_if) {

		// find ';', '{', or end
		while ((sym != s_eof) && (sym != s_semi) && (sym != s_lcurly)) getsym();

		if (sym == s_eof) return;
		else if (sym == s_lcurly) skipstatement();	// eat an if-true {statementlist;}
		else getsym();								// ate the statement; eat the ';'

		// now handle the optional 'else' part
		if (sym == s_else) {
			getsym();			// eat 'else'
			skipstatement();	// skip one statement and we're done
		}
	}

	// Skip a single statement, not a statementlist in braces: 
	// eat until semicolon or ')'
	// ignoring embedded argument lists
	else {
		while (sym != s_eof) {
			if (sym == s_lparen) ++nestlevel;
			else if (sym == s_rparen) {
				if (nestlevel <= 0) {
					getsym();
					break;
				}
				else --nestlevel;
			}
			else if (sym == s_quote) parsestring(&skipbyte);
			else if (nestlevel == 0) {
				//if ((sym == s_semi) || (sym == s_comma)) {
				if (sym == s_semi) {
					getsym();	// eat ";"
					break;
				}
			}
			getsym();
		}
	}

#ifdef PARSER_TRACE
	if (trace) sp("]SKP");
#endif

}


numvar getstatement(void);


// The switch statement: execute one of N statements based on a selector value
// switch <numval> { stmt0; stmt1;...;stmtN }
// numval < 0: treated as numval == 0
// numval > N: treated as numval == N
//
numvar getswitchstatement(void) {
numvar thesymval = symval;
char *fetchmark;
numvar retval = 0;
byte thesym = sym;

	getsym();						// eat "switch"
	getnum();						// evaluate the switch selector
	if (expval < 0) expval = 0;		// map negative values to zero
	byte which = (byte) expval;		// and stash it for reference
	if (sym != s_lcurly) expectedchar('{');
	getsym();		// eat "{"

	// we sit before the first statement
	// scan and discard the <selector>'s worth of statements 
	// that sit before the one we want
	while ((which > 0) && (sym != s_eof) && (sym != s_rcurly)) {
		fetchmark = fetchptr;
		thesym = sym;
		thesymval = symval;
		skipstatement();
		if ((sym != s_eof) && (sym != s_rcurly)) --which;
	}

	// If the selector is greater than the number of statements,
	// back up and execute the last one
	if (which > 0) {					// oops ran out of piddys
		fetchptr = fetchmark;			// restore to last statement
		primec();						// set up for getsym()
		sym = thesym;
		symval = thesymval;
	}
	//unexpected(M_number);

	// execute the statement we're pointing at
	retval = getstatement();

	// eat the rest of the statement block to "}"
	while ((sym != s_eof) && (sym != s_rcurly)) skipstatement();
	if (sym == s_rcurly) getsym();		// eat "}"
	return retval;
}


// Get a statement
numvar getstatement(void) {
numvar retval = 0;
char *fetchmark;

	chkbreak();

//#define LINEMODE
#ifdef LINEMODE
	if (sym == s_while) {
		// at this point sym is pointing at s_while, before the conditional expression
		// save fetchptr so we can restart parsing from here as the while iterates
		char *fetchmark = fetchptr;
		for (;;) {
			fetchptr = fetchmark;			// restore to mark
			primec();						// set up for mr. getsym()
			getsym(); 						// fetch the start of the conditional
			if (!getnum()) {					
				//longjmp(env, X_EXIT);		// get the conditional; exit on false
				sym = s_eof;				// we're finished here.  move along.
				return;
			}
			if (sym != s_colon) expectedchar(':');
			getsym();	// eat :
			getstatementlist();
		}
	}
	else if (sym == s_if) {
		getsym(); 								// fetch the start of the conditional
		if (!getnum()) {
			//longjmp(env, X_EXIT);	// get the conditional; exit on false
			sym = s_eof;
			return;
		}
		if (sym != s_colon) expectedchar(':');
		getsym();	// eat :
		getstatementlist();
	}

	// The switch statement: call one of N macros based on a selector value
	// switch <numval>: macroid1, macroid2,.., macroidN
	// numval < 0: numval = 0
	// numval > N: numval = N

	else if (sym == s_switch) {
		getsym();	// eat "switch"
		numvar selector = getnum();	// evaluate the switch value
		if (selector < 0) selector = 0;
		if (sym != s_colon) expectedchar(':');

		// we sit before the first macroid
		// scan and discard the <selector>'s worth of macro ids 
		// that sit before the one we want
		for (;;) {
			getsym();	// get an id, sets symval to its eeprom addr as a side effect
			if (sym != s_macro) expected (6);		// TODO: define M_macro instead of 6
			getsym();	// eat id, get separator; assume symval is untouched
			if ((sym == s_semi) || (sym == s_eof)) break;	// last case is default so we exit always
			if (sym != s_comma) expectedchar(',');
			if (!selector) break;		// ok, this is the one we want to execute
			selector--;					// one down...
		}

		// call the macro whose addr is squirreled in symval all this time
		// on return, the parser is ready to pick up where we left off
		domacrocall(symval);

		// scan past the rest of the unused switch options, if any
		// TODO: syntax checking for non-chosen options could be made much tighter at the cost of some space
		while ((sym != s_semi) && (sym != s_eof)) getsym();		// scan to end of statement without executing
	}

#else
	// new statement handling
	if (sym == s_while) {
		// at this point sym is pointing at s_while, before the conditional expression
		// save fetchptr so we can restart parsing from here as the while iterates
		fetchmark = fetchptr;
		for (;;) {
			fetchptr = fetchmark;			// restore to mark
			primec();						// set up for mr. getsym()
			getsym(); 						// fetch the start of the conditional
			if (getnum()) {
				retval = getstatement();
				if (sym == s_returning) break;	// exit if we caught a return
			}
			else {
				skipstatement();
				break;
			}
		}
	}
	
	else if (sym == s_if) {
		getsym();			// eat "if"
		if (getnum()) {
			retval = getstatement();
			if (sym == s_else) {
				getsym();	// eat "else"
				skipstatement();
			}
		} else {
			skipstatement();
			if (sym == s_else) {
				getsym();	// eat "else"
				retval = getstatement();
			}
		}
	}
	else if (sym == s_lcurly) {
		getsym(); 	// eat "{"
		while ((sym != s_eof) && (sym != s_returning) && (sym != s_rcurly)) retval = getstatement();
		if (sym == s_rcurly) getsym();	// eat "}"
	}
	else if (sym == s_return) {
		getsym();	// eat "return"
		if ((sym != s_eof) && (sym != s_semi)) retval = getnum();
		sym = s_returning;		// signal we're returning up the line
	}
	else if (sym == s_switch) retval = getswitchstatement();

	else if (sym == s_function) cmd_function();

#endif


	else if (sym == s_run) {	// run macroname
		getsym();
		if (sym != s_macro) unexpected(M_id);

		// address of macroid is in symval via parseid
		// check for [,snoozeintervalms]
		getsym();	// eat macroid to check for comma; symval untouched
		if (sym == s_comma) {
			vpush(symval);
			getsym();			// eat the comma
			getnum();			// get a number or else
			startTask(kludge(vpop()), expval);
		}
		else startTask(kludge(symval), 0);
	}

	else if (sym == s_stop) {
		getsym();
		if (sym == s_mul) {						// stop * stops all tasks
			initTaskList();
			getsym();
		}
		else if ((sym == s_semi) || (sym == s_eof)) {
			if (background) stopTask(curtask);	// stop with no args stops the current task IF we're in back
			else initTaskList();				// in foreground, stop all
		}
		else stopTask(getnum());
	}

	else if (sym == s_boot) reboot();

#if !defined(TINY85)
	else if (sym == s_rm) {		// rm "sym" or rm *
		getsym();
		if (sym == s_macro) {
			eraseentry(idbuf);
		} 
		else if (sym == s_mul) nukeeeprom();
		else if (sym != s_undef) expected(M_id);
		getsym();
	}
	else if (sym == s_ps) 		{ getsym();	showTaskList(); }
	else if (sym == s_peep) 	{ getsym(); cmd_peep(); }
	else if (sym == s_ls) 		{ getsym(); cmd_ls(); }
	else if (sym == s_help) 	{ getsym(); cmd_help(); }
	else if (sym == s_print) 	{ getsym(); cmd_print(); }
	else if (sym == s_semi)		{ ; }	// ;)
#endif

#ifdef HEX_UPLOAD
	// a line beginning with a colon is treated as a hex record
	// containing data to upload to eeprom
	//
	// TODO: verify checksum
	//
	else if (sym == s_colon) {
		// fetchptr points at the byte count
		byte byteCount = gethex(2);		// 2 bytes byte count
		int addr = gethex(4);			// 4 bytes address
		byte recordType = gethex(2);	// 2 bytes record type; now fetchptr -> data
		if (recordType == 1) reboot();	// reboot on EOF record (01)
		if (recordType != 0) return;	// we only handle the data record (00)
		if (addr == 0) nukeeeprom();	// auto-clear eeprom on write to 0000
		while (byteCount--) eewrite(addr++, gethex(2));		// update the eeprom
		gethex(2);						// discard the checksum
		getsym();						// and re-prime the parser
	}
#endif

	else getexpression();

	if (sym == s_semi) getsym();		// eat trailing ';'
	return retval;
}


// Parse and execute a list of statements separated by semicolons
//
//
numvar getstatementlist(void) {
//	getstatement();
//	while (sym == s_semi) {
//		getsym();
//		if (sym != s_eof) getstatement();		// quietly allow trailing semicolon
//	}

#ifdef LINEMODE
	while (sym != s_eof) {
		if (sym == s_return) {
			getsym();
			if ((sym == s_semi) || (sym == s_eof)) return 0;
			return getnum();
		}
		else getstatement();
	}
	return 0;
#else
numvar retval = 0;
	while ((sym != s_eof) && (sym != s_returning)) retval = getstatement();
	return retval;
#endif
}



// call a macro and push its return value on the stack
//
void domacrocall(int macroaddress) {
	if (macroaddress >= 0) {
	
		parsearglist();
		byte thesym = sym;					// save sym for restore
		vpush(symval);						// and symval
		char *fetchmark = fetchptr;			// save the current parse pointer

		// call the macro
		calleeprommacro(findend(macroaddress));	// register the macro into the parser stream
		getsym();								// fetch its first symbol
		
		numvar ret = getstatementlist();		// parse and execute the macro code here
//		if (sym != s_eof) expected(M_eof);

		// restore parsing context so we can resume cleanly
		symval = vpop();		// restore symval
		sym = thesym;			// restore saved sym
		releaseargblock();		// drop the args
		fetchptr = fetchmark;	// restore pointer
		primec();				// and inchar
		vpush(ret);				// send back our return value
	}
}


