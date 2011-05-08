#include <SdFat.h>
SdFat sd;
SdFile myFile;

// parse context types
#define FETCH_NONE		0
#define FETCH_RAM 		1
#define FETCH_PROGMEM 	2
#define FETCH_EEPROM 	3
#define FETCH_SDFAT		4

byte fetchtype;
numvar fetchptr;

/*****

TODO: SD Support


- doCommand()
	- published interface is like exec("print 1+1"): executes string in RAM
	- make doCommand() re-entrant
		- only register setjmp if previous fetchtype is FETCH_NONE

- runBackgroundMacro
	- internally we need
		- exec_stream(byte type, numvar fetchmark, how does the sdfile name get through?)
	- does not need re-entrancy; only called at top of task entry		

- move domacrocall here from bitlash-interpreter.c
	- called from factor

- parseargblock()
	- stash/retrieve function name and pointer
	- arg[-1] if push before arg(0)
	- save and deallocate it after the call
	- TODO: stack dump on fatal error :)

- parseid(): 
	- need a function somewhere for SD init
		- may as well be here
		- need a define
		- may as well be BITLASH_SD
	- check for active file system
		*** flag to indicate whether sd system is active
		*** how often to check?
			- boot time?
			- eject() function?
		- SDFat.init()
			- returns true on success

	- check for file on sd card
		- use SDFat.exists(file)
			- if we open it we have to save the context beforehand...

	- how to pass back called-function location context: SDFILE vs. EEPROM?
		- till now it's implicitly been eeprom
		- another symbol type?


- feat: detect card insertion and autorun startup function
- feat: commands on the card: ls, rm, create
- feat: create/write to file

END TODO LIST
*****/


void doCommand_EEPROM(numvar location) {
	initParsePoint(FETCH_EEPROM, location);
}

//*** use for the banner in initBitlash()
//*** make more examples
doCommand_PROGMEM(numvar location);
	initParsePoint(FETCH_PROGMEM, location);
}

doCommand_SDFILE(??) {
}



//////////
///
///	Initialize the parser to interpret a stream
///
///	This is used in doCommand to execute a passed-in or collected text command,
/// in domacrocommand() when a macro/function is called from within a parse stream,
///	and in runBackgroundTasks to kick off the background run.
///
/// Overwrites current parse context; callers must use mark and restore to resume parsing.
///
void initParsePoint(byte type, numvar location) {
	fetchtype = type;
	fetchptr = location;
	primec();
}


//////////
///
/// Parse mark and restore
///
/// Interpreting the while and switch commands requires backing up to a previous point
///	in the input stream.  These routines allow the parser to drop anchor at a point in the
///	stream and restore back to it.
///	
numvar markParsePoint(void) {

	if (fetchtype == FETCH_SDFAT) {
		// the location we wish to return to is the point from which we read inchar, 
		// which is one byte before the current file pointer since it auto-advances
		fetchptr = infile.getPosition() - 1;
	}

	// stash the fetch context type in the high nibble of fetchptr
	// LIMIT: longest script is 2^29-1 bytes
	return (fetchtype << 28) | fetchptr;
}


void returnToParsePoint(numvar fetchmark) {

	// restore parse type and location
	fetchtype = fetchmark >> 28;			// unstash type from top nibble
	fetchptr = fetchmark & 0xfffffff;		// LIMIT: longest script is 2^29-1 bytes

//??does this belong in primec?
	if (fetchtype == FETCH_SDFAT) {
		// open the input file if there is no file open, 
		// or the open file does not match what we want
		if (!SDFile.isOpen() || strcmp(???, SDFile.getFilename(char * name)) {
			inFile = SDFile.open(???, O_READ);
um, what happens here on error?
card pulled?

		}
		SDFile.seekSet(fetchptr);
	}

	primec();	// re-fetch inchar
}


//////////
///
///	fetchc(): 
///		advance input to next character of input stream
///		and set inchar to the character found there
///
void fetchc(void) {
	++fetchptr;
	primec();
}


//////////
///
///	primec(): 
///		fetch the current character from the input stream
///		set inchar to the character or zero on EOF
///
void primec(void) {

	switch (fetchtype) {
		case FETCH_RAM:		inchar = *(char *) fetchptr;		break;
		case FETCH_PROGMEM:	inchar = pgm_read_byte(fetchptr); 	break;
		case FETCH_EEPROM:	inchar = eeread(fetchptr);			break;
	
		case FETCH_SDFAT:
			if (infile.read(&inchar, 1) <= 0) {
				//infile.close();		// leave the file open for re-use
				inchar = 0;		// signal EOF
			}
			break;
	}
}
