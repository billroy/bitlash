/***
	bitlashsd.pde: Bitlash integration with SD file support

	Bitlash is a tiny language interpreter that provides a serial port shell environment
	for bit banging and hardware hacking.

	This is an example demonstrating how to use the Bitlash2 library for Arduino 0015.

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
#include <SdFat.h>
SdFat sd;
SdFile scriptfile;

byte sd_up;	// true iff SDFat.init() has succeeded

char scriptnamecache[14];	// TODO: proper define here


// return true iff script exists
byte scriptexists(char *scriptname) {
	if (!sd_up) {
		if (!sd.init()) return 0;
		sd_up = 1;
	}
	return sd.exists(scriptname);
}

// open and set parse location on input file
byte scriptopen(char *scriptname, numvar position) {
	// open the input file if there is no file open, 
	// or the open file does not match what we want
	if (!scriptfile.isOpen() || strcmp(scriptname, scriptnamecache)) {
		if (scriptfile.isOpen()) scriptfile.close();

		Serial.print("O:"); Serial.println(scriptname);

		if (!scriptfile.open(scriptname, O_READ)) return 0;
		strcpy(scriptnamecache, scriptname);		// cache the name we have open
		if (position == 0L) return 1;				// save a seek, when we can
	}
	return scriptfile.seekSet(position);
}

numvar scriptgetpos(void) {
fpos_t pos;
	pos.position = 0L;		// TODO: remove after debugging
	scriptfile.getpos(&pos);
	return pos.position;
}

byte scriptread(void) {
	int input = scriptfile.read();
	if (input == -1) {
		//scriptfile.close();		// leave the file open for re-use
		return 0;
	}
	return (byte) input;
}



void setup(void) {

	// initialize bitlash and set primary serial port baud
	// print startup banner and run the startup macro
	initBitlash(57600);

	// you can execute commands here to set up initial state
	// bear in mind these execute after the startup macro
	// doCommand("print(1+1)");
}

void loop(void) {
	runBitlash();
}
