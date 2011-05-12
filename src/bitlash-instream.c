/***
	bitlash-instream.c

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


// forward declaration
void initparsepoint(void);

byte scriptexists(char *scriptname);
byte scriptopen(char *scriptname, numvar position);
numvar scriptgetpos(void);
byte scriptread(void);


/////////
//
//	Parse and interpret a stream, and return its value
//
//	This is used in doCommand to execute a passed-in or collected text command,
// in domacrocommand() when a macro/function is called from within a parse stream,
//	and in runBackgroundTasks to kick off the background run.
//
//
numvar execscript(byte scripttype, numvar scriptaddress) {
numvar fetchmark = markparsepoint();

	// if this is the first stream context in this invocation,
	// set up our error recovery point and init the value stack
	// otherwise we skip this to allow nested execution calls 
	// to properly return to top
	//
	if (fetchtype == SCRIPT_NONE) {

		// Exceptions come here via longjmp; see bitlash-error.c
		switch(setjmp(env)) {
			case 0: break;
			case X_EXIT: {

				// POLICY: Stop all background tasks on any error
				//
				// It is not possible to be certain that continuing here will work.
				// Not all errors leave the interpreter in a working state.  Though most do.
				// The conservative/deterministic choice is to stop all background tasks
				// and drop the user back to the command prompt.
				//
				// On the other hand, you may find this inconvenient in your application, 
				// and may be happy taking the risk of continuing.
				//
				// In which case, comment out this line and proceed with caution.
				//	
				// TODO: if the macro "onerror" exists, call it here instead.  Let it "stop *".
				//
				// -br
				//
				initTaskList();		// stop all pending tasks

#ifdef SOFTWARE_SERIAL_TX
				resetOutput();		// clean up print module
#endif

				return (numvar) -1;
			}
		}
		vinit();			// initialize the expression stack
	}
	fetchtype = scripttype;
	fetchptr = scriptaddress;
	initparsepoint();
	getsym();

	// interpret the function text and collect its result
	numvar ret = getstatementlist();
	returntoparsepoint(fetchmark);		// now where were we?
	return ret;
}


/////////
//
// Call a Bitlash script function and push its return value on the stack
//
void callscriptfunction(byte scripttype, numvar scriptaddress) {
	
	parsearglist();
	byte thesym = sym;					// save next sym for restore
	vpush(symval);						// and symval

	numvar ret = execscript(scripttype, scriptaddress);

	symval = vpop();
	sym = thesym;
	releaseargblock();
	vpush(ret);
}


/////////
//
// Parse mark and restore
//
// Interpreting the while and switch commands requires marking and resuming from
// a previous point in the input stream.  So does calling a function in eeprom.
// These routines allow the parser to drop anchor at a point in the stream 
// and restore back to it.
//	
numvar markparsepoint(void) {

	if (fetchtype == SCRIPT_FILE) {
		// the location we wish to return to is the point from which we read inchar, 
		// which is one byte before the current file pointer since it auto-advances
		fetchptr = scriptgetpos() - 1;
	}

	// stash the fetch context type in the high nibble of fetchptr
	// LIMIT: longest script is 2^29-1 bytes
	numvar ret = ((numvar) fetchtype << 28) | (numvar) fetchptr;

#ifdef PARSER_TRACE
	if (trace) {
		sp("mark:");printInteger(fetchtype); spb(' '); printInteger(fetchptr); 
		spb('>'); printInteger(ret);
		speol();
	}
#endif

	return ret;
}


void initparsepoint(void) {

	// handle file transition side effects here, once per transition,
	// rather than once per character below in primec()
	if (fetchtype == SCRIPT_FILE) {

		sp("fopen ");sp((char *) fetchptr); speol();

		// ask the file glue to open and position the file for us
		if (!scriptopen((char *) fetchptr, 0L)) unexpected(M_oops);		// TODO: error message
	}
	primec();	// re-fetch inchar
}


void returntoparsepoint(numvar fetchmark) {
	// restore parse type and location
	fetchtype = fetchmark >> 28;			// unstash type from top nibble
	fetchptr = fetchmark & 0xfffffff;		// LIMIT: longest script is 2^29-1 bytes
	initparsepoint();

#ifdef PARSER_TRACE
	if (trace) {
		sp("return:");
		printInteger(fetchmark); spb('>');
		printInteger(fetchtype); spb(' '); printInteger(fetchptr); 
		speol();
	}
#endif

}


/////////
//
//	fetchc(): 
//		advance input to next character of input stream
//		and set inchar to the character found there
//
void fetchc(void) {
	++fetchptr;

#ifdef PARSER_TRACE
	if (trace) {
		spb('[');
		printInteger(fetchptr);
		spb(']');
	}
#endif

	primec();
}


/////////
//
//	primec(): 
//		fetch the current character from the input stream
//		set inchar to the character or zero on EOF
//
void primec(void) {
	switch (fetchtype) {
		case SCRIPT_RAM:		inchar = *(char *) fetchptr;		break;
		case SCRIPT_PROGMEM:	inchar = pgm_read_byte(fetchptr); 	break;
		case SCRIPT_EEPROM:		inchar = eeread((int) fetchptr);	break;
		case SCRIPT_FILE:		inchar = scriptread();				break;
	}

#ifdef PARSER_TRACE
	if (trace) {
		spb('<'); 
		if (inchar >= 0x20) spb(inchar);
		else { spb('\\'); printInteger(inchar); }
		spb('>');
	}
#endif

}


/////////
//
//	Print traceback
//
void traceback(void) {
numvar *a = arg;
	while (a) {
		sp((char *) (a[-1])); speol();
		a = (numvar *) (a[-2]);
	}
}
