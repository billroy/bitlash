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

#include "bitlash.h"
#include <SdFat.h>
SdFat sd;
SdFile scriptfile;

byte sd_up;	// true iff SDFat.init() has succeeded
char scriptnamecache[14];	// TODO: proper define here

byte initsd(void) {
	if (!sd_up) {
		// TODO: document the sd.init() options here
		if (!sd.init()) return 0;
		sd_up = 1;
	}
}

// return true iff script exists
byte scriptexists(char *scriptname) {
	if (!(initsd()) return 0;
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

numvar sdls(void) {
	if (initsd()) sd.ls(LS_SIZE, 0);		// LS_SIZE, LS_DATE, LS_R, indent
	return 0;
}

numvar sdexists(void) { return scriptexists(arg(1)); }
numvar sdrm(void) { return sd.rm(arg(1)); }

numvar sdwrite(char *filename, char *contents, byte append) {
	numvar fetchmark = markparsepoint();
	if (scriptfile.isOpen()) {
		if (!scriptfile.close()) return 0;
	}
	int flags = O_CREAT | O_TRUNC;
	if (append) flags |= O_APPEND;
	if (!scriptfile.open(filename, flags)) return 0;
	if (scriptfile.write(contents, strlen(contents)) < 0) return 0;
	if (!scriptfile.close()) return 0;
	restoreParsePoint(fetchmark, 1);
	return 1;
}

numvar sdcreate(void) { return sdwrite((char *) arg(1), (char *) arg(2), 0); }
numvar sdappend(void) { return sdwrite((char *) arg(1), (char *) arg(2), 1); }

numvar sdcat(void) {
	if (!scriptexists(arg(1)) return 0;
	numvar fetchmark = markparsepoint();
	initParsePoint(SCRIPT_FILE, 0L, (char *) arg(1));
	while (inchar) {
		spb(inchar);
		fetchc();
	}
	restoreParsePoint(fetchmark, 1);
	return 1;
}

// test doCommand() re-entrancy
numvar exec(void) {
	return doCommand((char *) getarg(1));
}

void setup(void) {

	// initialize bitlash and set primary serial port baud
	// print startup banner and run the startup macro
	initBitlash(57600);

	addBitlashFunction("exec", (bitlash_function) exec);
	addBitlashFunction("sd.ls", (bitlash_function) sdls);
	addBitlashFunction("sd.exists", (bitlash_function) sdexists);
	addBitlashFunction("sd.rm", (bitlash_function) sdrm);
	addBitlashFunction("sd.create", (bitlash_function) sdcreate);
	addBitlashFunction("sd.append", (bitlash_function) sdappend);
	addBitlashFunction("sd.cat", (bitlash_function) sdcat);

}

void loop(void) {
	runBitlash();
}
