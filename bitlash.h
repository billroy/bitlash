/***
	bitlash.h

	Bitlash is a tiny language interpreter that provides a serial port shell environment
	for bit banging and hardware hacking.

	This is the Bitlash library for Arduino 0014.

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
#include "WProgram.h"

///////////////////////
//	Start Bitlash, and give it cycles to do stuff
//
void initBitlash(unsigned long baud);	// start up and set baud rate
void runBitlash(void);					// call this in loop(), frequently


///////////////////////
//	Pass a command to Bitlash for interpretation
//
void doCommand(char *);					// execute a command from your sketch
void doCharacter(char);					// pass an input character to the line editor

///////////////////////
//	Access to Numeric Variables
//
//	NOTE: access to variables a..z is via an index 0..25, not the variable names.  Got it?
//
typedef long int numvar;					// bitlash returns things of type numvar
typedef unsigned long int unumvar;			// sometimes unsigned interpretation is best (like millis)

numvar getVar(unsigned char);				// return value of bitlash variable.  id is [0..25] for [a..z]
void assignVar(unsigned char, numvar);		// assign value to variable.  id is [0..25] for [a..z]
numvar incVar(unsigned char);				// increment variable.  id is [0..25] for [a..z]


///////////////////////
//	Access to the Bitlash EEPROM key:value store
//
// Lookup key in EEPROM and return the location of its value
int getValue(char *key);			// return location of macro value in EEPROM or -1

///////////////////////
//	Add a user function to Bitlash
//
typedef numvar (*bitlash_function)(void);
void addBitlashFunction(char *, bitlash_function);
numvar getarg(numvar);

///////////////////////
//	Serial Output Capture
//
typedef void (*serialOutputFunc)(byte);
byte serialIsOverridden(void);
void setOutputHandler(serialOutputFunc);
void resetOutputHandler(void);
