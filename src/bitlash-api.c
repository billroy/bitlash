/***
	bitlash-api.c

	Bitlash is a tiny language interpreter that provides a serial port shell environment
	for bit banging and hardware hacking.

	See the file README for documentation.  Or just upload this file as a sketch and play.

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

	beginSerial(baud);

	initTaskList();
	vinit();
	displayBanner();

	// Run the script named "startup" if there is one
	strncpy_P(lbuf, getmsg(M_startup), STRVALLEN);	// get the name "startup" in our cmd buf
	//if (findKey(lbuf) >= 0) doCommand(lbuf);		// look it up.  exists?  call it.
	if (findscript(lbuf)) doCommand(lbuf);			// look it up.  exists?  call it.

	initlbuf();
}

