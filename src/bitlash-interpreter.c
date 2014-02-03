/***
	bitlash-interpreter.c

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
	while (addr <= ENDDB) {
		if (eeread(addr) != EMPTY) eewrite(addr, EMPTY);
		addr++;
	}
}


#if defined(AVR_BUILD)
void cmd_boot(void) {
	// This is recommended but does not work on Arduino
	// Reset_AVR();
	void (*bootvec)(void) = 0; (*bootvec)(); 	// we jump through 0 instead
}
#elif defined(ARM_BUILD)

#if ARM_BUILD==1
// SAM3XA software restart
void cmd_boot(void) {
	// See SAM3X data sheet for reference information.  This is a software
	// reset of processor, peripherals, and raises the NRST pin.  Pretty
	// much everything that can be reset is reset.
	//
	REG_RSTC_CR = (RSTC_CR_PROCRST | RSTC_CR_PERRST | RSTC_CR_EXTRST | RSTC_CR_KEY(0xA5));
	while(1);
}
#elif ARM_BUILD==4
void cmd_boot(void){
  oops('boot');
}
#else
void cmd_boot(void) {
  #ifndef SCB_AIRCR_SYSRESETREQ_MASK
    #define SCB_AIRCR_SYSRESETREQ_MASK ((unsigned int) 0x00000004)
  #endif

  cli();
  delay(100);
  SCB_AIRCR = 0x05FA0000 | SCB_AIRCR_SYSRESETREQ_MASK;
  while(1);
}
#endif
#else
void cmd_boot(void) {oops('boot');}
#endif

void skipbyte(char c) {;}

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

	// skipping the if statement is a little tricky; same for switch
	else if ((sym == s_if) || (sym == s_switch)) {

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
numvar retval = 0;
byte thesym = sym;
parsepoint fetchmark;

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
		markparsepoint(&fetchmark);
		thesym = sym;
		thesymval = symval;
		skipstatement();
		if ((sym != s_eof) && (sym != s_rcurly)) --which;
	}

	// If the selector is greater than the number of statements,
	// back up and execute the last one
	if (which > 0) {					// oops ran out of piddys
		returntoparsepoint(&fetchmark, 0);
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

#if !defined(TINY_BUILD) && !defined(UNIX_BUILD)
	chkbreak();
#endif

	if (sym == s_while) {
		// at this point sym is pointing at s_while, before the conditional expression
		// save fetchptr so we can restart parsing from here as the while iterates
		parsepoint fetchmark;
		markparsepoint(&fetchmark);
		for (;;) {
			returntoparsepoint(&fetchmark, 0);
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

#if !defined(TINY_BUILD)
	else if (sym == s_switch) retval = getswitchstatement();
#endif

	else if (sym == s_function) cmd_function();

	else if (sym == s_run) {	// run macroname
		getsym();
		if ((sym != s_script_eeprom) && (sym != s_script_progmem) &&
			(sym != s_script_file)) unexpected(M_id);

		// address of macroid is in symval via parseid
		// check for [,snoozeintervalms]
		getsym();	// eat macroid to check for comma; symval untouched
		if (sym == s_comma) {
			vpush(symval);
			getsym();			// eat the comma
			getnum();			// get a number or else
			startTask(vpop(), expval);
		}
		else startTask(symval, 0);
	}

	else if (sym == s_stop) {
		getsym();
#if !defined(TINY_BUILD)
		if (sym == s_mul) {						// stop * stops all tasks
			initTaskList();
			getsym();
		}
		else if ((sym == s_semi) || (sym == s_eof)) {
			if (background) stopTask(curtask);	// stop with no args stops the current task IF we're in back
			else initTaskList();				// in foreground, stop all
		}
		else 
#endif
			stopTask(getnum());
	}

	else if (sym == s_rm) {		// rm "sym" or rm *
		getsym();
		if (sym == s_script_eeprom) {
			eraseentry(idbuf);
		} 
#if !defined(TINY_BUILD)
		else if (sym == s_mul) nukeeeprom();
#endif
		else if (sym != s_undef) expected(M_id);
		getsym();
	}
	else if (sym == s_ls) 	{ getsym(); cmd_ls(); }
#if !defined(TINY_BUILD)
	else if (sym == s_boot) cmd_boot();
	else if (sym == s_ps) 	{ getsym();	showTaskList(); }
	else if (sym == s_peep) { getsym(); cmd_peep(); }
	else if (sym == s_help) { getsym(); cmd_help(); }
#endif
	else if (sym == s_print) { getsym(); cmd_print(); }
	else if (sym == s_semi)	{ ; }	// ;)

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

	else {
	    getexpression();
	    retval = expval;
	}

	if (sym == s_semi) getsym();		// eat trailing ';'
	return retval;
}


// Parse and execute a list of statements separated by semicolons
//
//
numvar getstatementlist(void) {
numvar retval = 0;
	while ((sym != s_eof) && (sym != s_returning)) retval = getstatement();
	return retval;
}


