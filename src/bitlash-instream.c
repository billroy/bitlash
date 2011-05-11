#include <SdFat.h>
SdFat sd;
SdFile myFile;

// parse context types
#define FETCH_NONE		0
#define FETCH_RAM 		1
#define FETCH_PROGMEM 	2
#define FETCH_EEPROM 	3
#define FETCH_FILE		4

byte fetchtype;
numvar fetchptr;

void exec_script(byte, numvar);

/*****

TODO: SD Support

- get rid of kludge, dekludge
- get rid of s_macro


- doCommand fixes per below
	- doc:
		- returns numvar
		- is re-entrant

- background macros

	- runBackgroundMacro
		- doCommand(kludge(findend(dekludge(tasklist[curtask]))));
		+ exec_stream(FETCH_EEPROM, tasklist[curtask]);

	- redefine tasklist to hold numvar in the slot
		- char *tasklist[NUMTASKS];			// macro text of the task
		+ numvar tasklist[NUMTASKS];

- getfactor
	- s_macro -> s_funct_eeprom or what?
	
- run command
	- 

- parseid(): 
***	- resolve s_funct passback strategy

	- s_macro -> s_funct_eeprom
		- add s_funct_file

	// macro ref or def?
	else if ((symval=findKey(idbuf)) >= 0) sym = s_funct_eeprom;
	else if ((scriptFileExists(idbuf)) sym = s_funct_file;

- need a define
	- may as well be BITLASH_FILE

==========

- feat: detect card insertion and autorun startup function
- feat: commands on the card: ls, rm, create
- feat: create/write to file
- feat: tb(): traceback on fatal error

	not working
	function tb {i=arg(-1);while i {print i:s;i=inb(i-4)-4;}}

- feat: anonymous scripts in progmem -> progmem block device

END TODO LIST
*****/




byte sd_up;	// true iff SDFat.init() has succeeded

byte scriptFileExists(char *scriptname) {
	if (!sd_up) {
		if (!(SDFat.init()) return 0;
		sd_up = 1;
	}
	return SDFat.exists(scriptname);
}


/////////
//
// doCommand: main entry point to execute a bitlash command
//
numvar doCommand(char *cmd) {
	return exec_script(FETCH_RAM, cmd);
}




/////////
//
//	Parse and interpret a stream, and return its value
//
//	This is used in doCommand to execute a passed-in or collected text command,
// in domacrocommand() when a macro/function is called from within a parse stream,
//	and in runBackgroundTasks to kick off the background run.
//
//
numvar exec_script(byte type, numvar location) {
numvar fetchmark = markParsePoint();

	// if this is the first stream context in this invocation,
	// set up our error recovery point and init the value stack
	// otherwise we skip this to allow nested execution calls 
	// to properly return to top
	//
	if (fetchtype == FETCH_NONE) {

		// Exceptions come here via longjmp; see bitlash-error.c
		switch(setjmp(env)) {
			case 0: break;
			case X_EXIT: {
				initTaskList();		// stop all pending tasks
#ifdef SOFTWARE_SERIAL_TX
				resetOutput();
#endif
				return;
			}
		}
		vinit();			// initialize the expression stack
	}
	fetchtype = type;
	fetchptr = location;
	initParsePoint();
	getsym();

	// interpret the function text and collect its result
	numvar ret = getstatementlist();
	returnToParsePoint(fetchmark);		// now where were we?
	return ret;
}


/////////
//
// Call a Bitlash script function and push its return value on the stack
//
void callscriptfunction(byte scripttype; numvar scriptaddress) {
	
	parsearglist();
	byte thesym = sym;					// save next sym for restore
	vpush(symval);						// and symval

	numvar ret = exec_script(scripttype, scriptaddress);

	symval = vpop();
	sym = thesym;
	releaseargblock();
	vpush(ret);
}


/////////
//
// Parse mark and restore
//
// Interpreting the while and switch commands requires backing up to and resuming from
// a previous point in the input stream.  So does calling a function in eeprom.
// These routines allow the parser to drop anchor at a point in the stream 
// and restore back to it.
//	
numvar markParsePoint(void) {

	if (fetchtype == FETCH_FILE) {
		// the location we wish to return to is the point from which we read inchar, 
		// which is one byte before the current file pointer since it auto-advances
		fetchptr = infile.getPosition() - 1;
	}

	// stash the fetch context type in the high nibble of fetchptr
	// LIMIT: longest script is 2^29-1 bytes
	return (fetchtype << 28) | fetchptr;
}


void initParsePoint(void) {

	// handle file transition side effects here, once per transition,
	// rather than once per character below in primec()
	if (fetchtype == FETCH_FILE) {

		// open the input file if there is no file open, 
		// or the open file does not match what we want
		if (!SDFile.isOpen() || strcmp((char *) getarg(-1), SDFile.getFilename(char * name)) {
			// Q: need to close an open file here before new open?
			if (!SDFile.open((char *) getarg(-1), O_READ)) expected(M_function);	// TODO: err msg
		}
		if (!SDFile.seekSet(fetchptr)) expected(M_function);	// TODO: file error msg
	}
	primec();	// re-fetch inchar
}


void returnToParsePoint(numvar fetchmark) {
	// restore parse type and location
	fetchtype = fetchmark >> 28;			// unstash type from top nibble
	fetchptr = fetchmark & 0xfffffff;		// LIMIT: longest script is 2^29-1 bytes
	initParsePoint();
}


/////////
//
//	fetchc(): 
//		advance input to next character of input stream
//		and set inchar to the character found there
//
void fetchc(void) {
	++fetchptr;
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
		case FETCH_RAM:		inchar = *(char *) fetchptr;		break;
		case FETCH_PROGMEM:	inchar = pgm_read_byte(fetchptr); 	break;
		case FETCH_EEPROM:	inchar = eeread(fetchptr);			break;
	
		case FETCH_FILE:
			if (SDFile.read(&inchar, 1) == -1) {
				//infile.close();		// leave the file open for re-use
				inchar = 0;		// signal EOF
			}
			break;
	}
}
