/***
	bitlash-interpreter.c

	Bitlash is a tiny language interpreter that provides a serial port shell environment
	for bit banging and hardware hacking.

	See the file README for documentation.

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


// Turn HEX_UPLOAD on to enable the hex file EEPROM uploader
// It costs 78 bytes of flash
//
#define HEX_UPLOAD
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


#define SKETCH 1
#if SKETCH
void doMacroCall(int);
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


// Get a statement
void getstatement(void) {

#if !defined(TINY85)
	chkbreak();
#endif

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


#if SKETCH
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
		doMacroCall(symval);

		// scan past the rest of the unused switch options, if any
		// TODO: syntax checking for non-chosen options could be made much tighter at the cost of some space
		while ((sym != s_semi) && (sym != s_eof)) getsym();		// scan to end of statement without executing
	}
#endif


	else if ((sym == s_macro) || (sym == s_undef)) {		// macro def or ref
		getsym();						// scan past macro name to next symbol: ; or :=
		if (sym == s_define) {			// macro definition: macroid := strvalue
			// to define the macro, we need to copy the id somewhere on the stack
			// to avoid having this local buffer in every getstatement stack frame,
			// we break out defineMacro here to a separate function that only eats that
			// stack in the case that a macro is being defined
#ifdef TINY85
			unexpected(M_defmacro);
#else
			defineMacro();
#endif
		}
		else if ((sym == s_semi) || (sym == s_eof)) {	// valid macro reference: let's call it
#if SKETCH
			doMacroCall(symval);			// parseid stashes the macro address in symval
#else
			char op = sym;					// save sym for restore
			expval = findKey(idbuf);		// assumes id in idbuf isn't clobbered since getsym() above
			if (expval >= 0) {
				char *fetchmark = fetchptr;			// save the current parse pointer

				// call the macro
				calleeprommacro(findend(expval));	// register the macro into the parser stream
				getsym();
				getstatementlist();		// parse and execute the macro code here
				if (sym != s_eof) expected(M_eof);

				// restore parsing context so we can resume cleanly
				fetchptr = fetchmark;	// restore pointer
				primec();				// and inchar
				sym = op;				// restore saved sym: s_semi or s_eof
			} else unexpected(M_id);
#endif
		}
		else expectedchar(';');
		//else getexpression();		// assume it was macro1+32+macro2...
	}
	
	else if (sym == s_run) {	// run macroname
		getsym();
		if (sym != s_macro) unexpected(M_id);

#if 0
		// address of macroid is in symval via parseid
		startTask(kludge(symval));
		getsym();
#else
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
#endif
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
		else expected(M_id);
		getsym();
	}
	else if (sym == s_ps) showTaskList();
	else if (sym == s_peep) 	{ getsym(); cmd_peep(); }
	else if (sym == s_ls) 		{ getsym(); cmd_ls(); }
	else if (sym == s_help) 	{ getsym(); cmd_help(); }
	else if (sym == s_print) 	{ getsym(); cmd_print(); }
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

	else  {
		getexpression();
	}
}


// Parse and execute a list of statements separated by semicolons
void getstatementlist(void) {
	getstatement();
	while (sym == s_semi) {
		getsym();
		if (sym != s_eof) getstatement();		// quietly allow trailing semicolon
	}


#if 0
	// TODO: try when the linker bug is fixed
	// poll the USB subsystem once per statement
	// this won't prevent starvation but might help
	usbPoll();		
#endif

}


#if SKETCH
void doMacroCall(int macroaddress) {
char op = sym;					// save sym for restore
	if (macroaddress >= 0) {
	
		char *fetchmark = fetchptr;			// save the current parse pointer
	
		// call the macro
		calleeprommacro(findend(macroaddress));	// register the macro into the parser stream
		getsym();
		getstatementlist();		// parse and execute the macro code here
		if (sym != s_eof) expected(M_eof);
	
		// restore parsing context so we can resume cleanly
		fetchptr = fetchmark;	// restore pointer
		primec();				// and inchar
		sym = op;				// restore saved sym
	}
}
#endif


