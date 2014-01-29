/***
	bitlash-instream.c

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

#if defined(SDFILE)

#define O_READ 0x01		// from SdFile.h

// Trampolines for the SD library
byte scriptfileexists(char *scriptname);
byte scriptopen(char *scriptname, numvar position, byte flags);
numvar scriptgetpos(void);
byte scriptread(void);
byte scriptwrite(char *filename, char *contents, byte append);
void scriptwritebyte(byte b);
#elif !defined(UNIX_BUILD)
byte scriptfileexists(char *scriptname) { return 0; }
#endif

// masks for stashing the pointer type in the high nibble
#if defined(UNIX_BUILD) && defined(__x86_64__)
#define MARK_SHIFT 60
#define ADDR_MASK 0xfffffffffffffffL
#else
#define MARK_SHIFT 28
#define ADDR_MASK 0xfffffffL
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
	parsepoint fetchmark;
	markparsepoint(&fetchmark);
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
	returntoparsepoint(&fetchmark, 1);		// now where were we?
	sym = thesym;
	symval = vpop();
	return ret;
}


// how to access the calling and called function names
//
//#define callername ((char *) ((numvar *) arg[2]) [1])
#define callername (arg[2] ? (char* ) (((numvar *) arg[2]) [1]) : NULL )
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
void markparsepoint(parsepoint *p) {

#if defined(SDFILE) || defined(UNIX_BUILD)
	if (fetchtype == SCRIPT_FILE) {
		// the location we wish to return to is the point from which we read inchar, 
		// which is one byte before the current file pointer since it auto-advances
		fetchptr = scriptgetpos() - 1;
	}
#endif

	p->fetchptr = fetchptr;
	p->fetchtype = fetchtype;

#ifdef PARSER_TRACE
	if (trace) {
		speol();	
		sp("mark:");printHex(fetchtype); spb(' '); printHex(fetchptr); 
		speol();
	}
#endif
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

#if defined(SDFILE) || defined(UNIX_BUILD)
	// handle file transition side effects here, once per transition,
	// rather than once per character below in primec()
	if (fetchtype == SCRIPT_FILE) {

#if defined(UNIX_BUILD)
		// ask the file glue to open and position the file for us
		if (!scriptopen(scriptname, scriptaddress, 0)) unexpected(M_oops);		// TODO: error message
#else
		// ask the file glue to open and position the file for us
		if (!scriptopen(scriptname, scriptaddress, O_READ)) unexpected(M_oops);		// TODO: error message
#endif
	}
#endif

	primec();	// re-fetch inchar
}



#ifdef UNIX_BUILD

char *topname = ".top.";

void returntoparsepoint(parsepoint *p, byte returntoparent) {
	// restore parse type and location; for script files, pass name from string pool
	byte ftype = p->fetchtype;
	char *scriptname = calleename;
	if (returntoparent) {
		if ((ftype == SCRIPT_NONE) || (ftype == SCRIPT_RAM))
			scriptname = topname;
		else if (arg[2]) scriptname = callername;
	}
	initparsepoint(p->fetchtype, p->fetchptr, scriptname);

#ifdef PARSER_TRACE
	if (trace) {
		speol();
		sp("rest:");
		printHex(fetchtype); spb(' '); printHex(fetchptr); spb(' ');printHex(returntoparent);
		speol();
	}
#endif

}

#else

void returntoparsepoint(parsepoint *p, byte returntoparent) {
	// restore parse type and location; for script files, pass name from string pool
	initparsepoint(p->fetchtype, p->fetchptr, returntoparent ? callername : calleename);
}
#endif



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

#if defined(SDFILE) || defined(UNIX_BUILD)
		case SCRIPT_FILE:		inchar = scriptread();				break;
#endif

		default:				unexpected(M_oops);
	}

#ifdef PARSER_TRACE
	if (trace) {
		spb('<'); 
		if (inchar >= 0x20) spb(inchar);
		else { spb('\\'); printInteger(inchar, 0, ' '); }
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


#if defined(SDFILE) || defined(UNIX_BUILD)

/////////
//
//	"cat": copy file to serial out
//
numvar sdcat(void) {
	if (!scriptfileexists((char *) getarg(1))) return 0;
	parsepoint fetchmark;
	markparsepoint(&fetchmark);
	initparsepoint(SCRIPT_FILE, 0L, (char *) getarg(1));
	while (inchar) {
		if (inchar == '\n') spb('\r');
		spb(inchar);
		fetchc();
	}
	returntoparsepoint(&fetchmark, 1);
	return 1;
}


/////////
//
//	sdwrite: write or append a line to a file
//
numvar sdwrite(char *filename, char *contents, byte append) {
#if !defined(UNIX_BUILD)
	parsepoint fetchmark;
	markparsepoint(&fetchmark);
#endif

	if (!scriptwrite(filename, contents, append)) unexpected(M_oops);

#if !defined(UNIX_BUILD)
	returntoparsepoint(&fetchmark, 1);
#endif

	return 1;
}

//////////
//
//	func_fprintf(): implementation of fprintf() function
//
//
numvar func_fprintf(void) {
	parsepoint fetchmark;
	markparsepoint(&fetchmark);

	scriptwrite((char *) getarg(1), "", 1);		// open the file for append (but append nothing)

	//serialOutputFunc saved_handler = serial_override_handler;	// save previous output handler
	void scriptwritebyte(byte);
	setOutputHandler(scriptwritebyte);			// set file output handler

	func_printf_handler(2,3);	// format=arg(2), optional args start at 3

	//setOutputHandler(saved_handler);// restore output handler
	resetOutputHandler();
#ifdef scriptclose
    scriptclose();          // close and flush the output
#endif
	returntoparsepoint(&fetchmark, 1);
}

#endif	// SDFILE
