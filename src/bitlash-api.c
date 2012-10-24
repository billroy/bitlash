/***
	bitlash-api.c

	Bitlash is a tiny language interpreter that provides a serial port shell environment
	for bit banging and hardware hacking.

	See the file README for documentation.  Or just upload this file as a sketch and play.

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
#include "bitlash.h"


// Exception handling state
// Syntax and execution errors are handled via longjmp
jmp_buf env;


/////////
//
// doCommand: main entry point to execute a bitlash command
//
numvar doCommand(char *cmd) {
	return execscript(SCRIPT_RAM, (numvar) cmd, 0);
}


void initBitlash(unsigned long baud) {

#if defined(TINY_BUILD)
	beginSerial(9600);
#else
	beginSerial(baud);
#endif

#if defined(ARM_BUILD)
	eeinit();
#endif

	initTaskList();
	vinit();
	displayBanner();

#if !defined(TINY_BUILD)
	// Run the script named "startup" if there is one
	strncpy_P(lbuf, getmsg(M_startup), STRVALLEN);	// get the name "startup" in our cmd buf
	//if (findKey(lbuf) >= 0) doCommand(lbuf);		// look it up.  exists?  call it.
	if (findscript(lbuf)) doCommand(lbuf);			// look it up.  exists?  call it.
#endif

	initlbuf();
}

