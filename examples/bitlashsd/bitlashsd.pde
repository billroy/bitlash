/***
	bitlashsd.pde: Bitlash integration with SD file support

	Bitlash is a tiny language interpreter that provides a serial port shell environment
	for bit banging and hardware hacking.

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

#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#include "bitlash.h"
#include <SdFat.h>
SdFat sd;
SdFile scriptfile;

byte sd_up;	// true iff SDFat.init() has succeeded

// filename buffer for 8.3 + \0
#define FNAMELEN 13
char cachedname[FNAMELEN];
byte cachedflags;

byte initsd(void) {
	if (!sd_up) {
		// TODO: document the sd.init() options here
		if (!sd.init()) return 0;
		sd_up = 1;
	}
}

// return true iff script exists
byte scriptfileexists(char *scriptname) {
	if (!(initsd())) return 0;
	return sd.exists(scriptname);
}

// open and set parse location on input file
byte scriptopen(char *scriptname, numvar position, byte flags) {
	// open the input file if there is no file open, 
	// or the open file does not match what we want
	if (!scriptfile.isOpen() || strcmp(scriptname, cachedname) || (flags != cachedflags)) {
		if (scriptfile.isOpen()) {
			if (!scriptfile.close()) return 0;
		}

		//Serial.print("O:"); Serial.print(flags,DEC);
		//Serial.print(' ',BYTE); Serial.println(scriptname);

		if (!scriptfile.open(scriptname, flags)) return 0;
		strcpy(cachedname, scriptname);		// cache the name we have open
		cachedflags = flags;				// and the mode
		if (position == 0L) return 1;		// save a seek, when we can
	}
	return scriptfile.seekSet(position);
}

numvar scriptgetpos(void) {
fpos_t pos;
	//pos.position = 0L;		// TODO: remove after debugging
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

byte scriptwrite(char *filename, char *contents, byte append) {

///	if (scriptfile.isOpen()) {
///		if (!scriptfile.close()) return 0;
///	}

	byte flags;
	if (append) flags = O_WRITE | O_CREAT | O_APPEND;
	else 		flags = O_WRITE | O_CREAT | O_TRUNC;

	if (!scriptopen(filename, 0L, flags)) return 0;
	if (strlen(contents)) {
		if (scriptfile.write(contents, strlen(contents)) < 0) return 0;
	}
//	if (!scriptfile.close()) return 0;
	return 1;
}

void scriptwritebyte(byte b) {
	// TODO: error check here
	scriptfile.write(&b, 1);
}


numvar sdls(void) {
//	if (initsd()) sd.ls(LS_SIZE, 0);		// LS_SIZE, LS_DATE, LS_R, indent
	if (initsd()) sd.ls(LS_SIZE);		// LS_SIZE, LS_DATE, LS_R, indent
	return 0;
}
numvar sdexists(void) { 
	if (!initsd()) return 0;
	return scriptfileexists((char *) getarg(1)); 
}
numvar sdrm(void) { 
	if (!initsd()) return 0;
	return sd.remove((char *) getarg(1)); 
}
numvar sdcreate(void) { 
	if (!initsd()) return 0;
	return sdwrite((char *) getarg(1), (char *) getarg(2), 0); 
}
numvar sdappend(void) { 
	if (!initsd()) return 0;
	return sdwrite((char *) getarg(1), (char *) getarg(2), 1); 
}
numvar sdcd(void) {

	// close any cached open file handle
	if (scriptfile.isOpen()) {
		if (!scriptfile.close()) return 0;
	}
	else if (!initsd()) return 0;
	return sd.chdir((char *) getarg(1));
}
numvar sdmd(void) { 
	if (!initsd()) return 0;
	return sd.mkdir((char *) getarg(1));
}


// test doCommand() re-entrancy
numvar exec(void) {
	return doCommand((char *) getarg(1));
}



void setup(void) {

	// initialize bitlash and set primary serial port baud
	// print startup banner and run the startup macro
	initBitlash(57600);

//	addBitlashFunction("exec", (bitlash_function) exec);
	addBitlashFunction("dir", (bitlash_function) sdls);
	addBitlashFunction("exists", (bitlash_function) sdexists);
	addBitlashFunction("del", (bitlash_function) sdrm);
//	addBitlashFunction("create", (bitlash_function) sdcreate);
	addBitlashFunction("append", (bitlash_function) sdappend);
	addBitlashFunction("type", (bitlash_function) sdcat);
	addBitlashFunction("cd", (bitlash_function) sdcd);
	addBitlashFunction("md", (bitlash_function) sdmd);
	addBitlashFunction("fprintf", (bitlash_function) func_fprintf);
}

void loop(void) {
	runBitlash();
}
