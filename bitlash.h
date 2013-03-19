/***
	bitlash.h

	Bitlash is a tiny language interpreter that provides a serial port shell environment
	for bit banging and hardware hacking.

	See the file README for documentation.

	Bitlash lives at: http://bitlash.net
	The author can be reached at: bill@bitlash.net

	Copyright (C) 2008-2012 Bill Roy

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
#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
	#define prog_char char PROGMEM
	#define prog_uchar unsigned char PROGMEM
#else
	#include "WProgram.h"
#endif


// Un-comment the line below to expose all the internals
// of the Bitlash core to your sketch.
//
//#include "src/bitlash.h"

///////////////////////
//	Start Bitlash, and give it cycles to do stuff
//
void initBitlash(unsigned long baud);	// start up and set baud rate
void runBitlash(void);					// call this in loop(), frequently

// Bitlash variables are of type "numvar"
//
typedef long int numvar;					// bitlash returns things of type numvar
typedef unsigned long int unumvar;			// sometimes unsigned interpretation is best (like millis)


///////////////////////
//	Pass a command to Bitlash for interpretation
//
numvar doCommand(char *);				// execute a command from your sketch
void doCharacter(char);					// pass an input character to the line editor

///////////////////////
//	Access to Numeric Variables
//
//	NOTE: access to variables a..z is via an index 0..25, not the variable names.  Got it?
//
numvar getVar(unsigned char);				// return value of bitlash variable.  id is [0..25] for [a..z]
void assignVar(unsigned char, numvar);		// assign value to variable.  id is [0..25] for [a..z]
numvar incVar(unsigned char);				// increment variable.  id is [0..25] for [a..z]


///////////////////////
//	Access to the Bitlash symbol table
//
// Lookup id and return TRUE if it exists
//
byte findscript(char *);		// returns TRUE if a script exists with this ID
int getValue(char *key);			// return location of macro value in EEPROM or -1

///////////////////////
//	Add a user function to Bitlash
//
typedef numvar (*bitlash_function)(void);
void addBitlashFunction(const char *, bitlash_function);
numvar getarg(numvar);
numvar isstringarg(numvar);
numvar getstringarg(numvar which);

///////////////////////
//	Serial Output Capture
//
typedef void (*serialOutputFunc)(byte);
byte serialIsOverridden(void);
void setOutputHandler(serialOutputFunc);
void setOutput(byte pin);
void resetOutputHandler(void);
numvar func_printf_handler(byte, byte);

///////////////////////
//	File functions
//
numvar sdcat(void);
numvar sdwrite(char *filename, char *contents, byte append);
numvar func_fprintf(void);
