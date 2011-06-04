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

// Enable this for script-in-file support
//#define SDFILE

#if defined(SDFILE)

#define O_READ 0x01		// from SdFile.h

// Trampolines for the SD library
byte scriptfileexists(char *scriptname);
byte scriptopen(char *scriptname, numvar position, byte flags);
numvar scriptgetpos(void);
byte scriptread(void);
byte scriptwrite(char *filename, char *contents, byte append);
void scriptwritebyte(byte b);
#else
byte scriptfileexists(char *scriptname) { return 0; }
#endif


// forward declaration
void initparsepoint(byte scripttype, numvar scriptaddress, char *scriptname);


/////////
//
//	Parse and interpret a stream, and return its value
//
//	This is used in doCommand to execute a passed-in or collected text command,
// in domacrocommand() when a macro/function is called from within a parse stream,
//	and in runBackgroundTasks to kick off the background run.
//
//
numvar execscript(byte scripttype, numvar scriptaddress, char *scriptname) {

	// save parse context
	numvar fetchmark = markparsepoint();
	byte thesym = sym;
	vpush(symval);

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
				// Other cleanups here
				vinit();			// initialize the expression stack
				fetchtype = SCRIPT_NONE;	// reset parse context
				fetchptr = 0L;				// reset parse location
				// sd_up = 0;				// TODO: reset file system
				return (numvar) -1;
			}							// X_EXIT case
		}								// switch
	}
	initparsepoint(scripttype, scriptaddress, scriptname);
	getsym();

	// interpret the function text and collect its result
	numvar ret = getstatementlist();
	returntoparsepoint(fetchmark, 1);		// now where were we?
	sym = thesym;
	symval = vpop();
	return ret;
}


// how to access the calling and called function names
//
#define callername ((char *) ((numvar *) arg[2]) [1])
#define calleename ((char *) arg[1]) 


/////////
//
// Call a Bitlash script function and push its return value on the stack
//
void callscriptfunction(byte scripttype, numvar scriptaddress) {

	// note on function name management
	//
	// we get here with the name of the function we want to call in global idbuf.
	// parsearglist() pushes a copy of idbuf into the string pool, so
	// the function's name is the first data in the string pool slab
	// that will be deallocated when the function returns
	//
	// we can refer to this copy of the function's name via the callername macro
	//
	parsearglist();
	numvar ret = execscript(scripttype, scriptaddress, calleename);
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

#if defined(SDFILE)
	if (fetchtype == SCRIPT_FILE) {
		// the location we wish to return to is the point from which we read inchar, 
		// which is one byte before the current file pointer since it auto-advances
		fetchptr = scriptgetpos() - 1;
	}
#endif

	// stash the fetch context type in the high nibble of fetchptr
	// LIMIT: longest script is 2^29-1 bytes
	numvar ret = ((numvar) fetchtype << 28) | (fetchptr & 0x0fffffffL);

#ifdef PARSER_TRACE
	if (trace) {
		speol();	
		sp("mark:");printHex(fetchtype); spb(' '); printHex(fetchptr); 
		spb('>'); printHex(ret);
		speol();
	}
#endif

	return ret;
}


void initparsepoint(byte scripttype, numvar scriptaddress, char *scriptname) {

#ifdef PARSER_TRACE
	if (trace) {
		speol();
		sp("init:");printHex(scripttype); spb(' '); printHex(scriptaddress); 
		if (scriptname) { spb(' '); sp(scriptname); }
		speol();
	}
#endif

	fetchtype = scripttype;
	fetchptr = scriptaddress;
	
	// if we're restoring to idle, we're done
	if (fetchtype == SCRIPT_NONE) return;

#if defined(SDFILE)
	// handle file transition side effects here, once per transition,
	// rather than once per character below in primec()
	if (fetchtype == SCRIPT_FILE) {

		// ask the file glue to open and position the file for us
		if (!scriptopen(scriptname, scriptaddress, O_READ)) unexpected(M_oops);		// TODO: error message
	}
#endif

	primec();	// re-fetch inchar
}


void returntoparsepoint(numvar fetchmark, byte returntoparent) {

	// restore parse type and location; for script files, pass name from string pool
	initparsepoint(fetchmark >> 28, fetchmark & 0x0fffffffL, 
		returntoparent ? callername : calleename);
			//((char *) ((numvar *) arg[2]) [1]) : ((char *) arg[1]) );


#ifdef PARSER_TRACE
	if (trace) {
		speol();
		sp("rest:");
		printHex(fetchmark); spb('>');
		printHex(fetchtype); spb(' '); printHex(fetchptr); spb(' ');printHex(returntoparent);
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
		printHex(fetchptr);
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

#if defined(SDFILE)
		case SCRIPT_FILE:		inchar = scriptread();				break;
#endif

		default:				unexpected(M_oops);
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
		sp((char *) (a[1])); speol();
		a = (numvar *) (a[2]);
	}
}


#if defined(SDFILE)

/////////
//
//	"cat": copy file to serial out
//
numvar sdcat(void) {
	if (!scriptfileexists((char *) getarg(1))) return 0;
	numvar fetchmark = markparsepoint();
	initparsepoint(SCRIPT_FILE, 0L, (char *) getarg(1));
	while (inchar) {
		if (inchar == '\n') spb('\r');
		spb(inchar);
		fetchc();
	}
	returntoparsepoint(fetchmark, 1);
	return 1;
}


/////////
//
//	sdwrite: write or append a line to a file
//
numvar sdwrite(char *filename, char *contents, byte append) {
	numvar fetchmark = markparsepoint();
	if (!scriptwrite(filename, contents, append)) unexpected(M_oops);
	returntoparsepoint(fetchmark, 1);
	return 1;
}

//////////
//
//	func_fprintf(): implementation of fprintf() function
//
//
numvar func_fprintf(void) {
	numvar fetchmark = markparsepoint();

	scriptwrite((char *) getarg(1), "", 1);		// open the file for append (but append nothing)

	//serialOutputFunc saved_handler = serial_override_handler;	// save previous output handler
	setOutputHandler(scriptwritebyte);			// set file output handler

	func_printf_handler(2,3);	// format=arg(2), optional args start at 3

	//setOutputHandler(saved_handler);// restore output handler
	resetOutputHandler();
	returntoparsepoint(fetchmark, 1);
}

#endif	// SDFILE
