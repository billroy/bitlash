/***
	bitlashsd.pde: Bitlash integration with SD file support

	Bitlash is a tiny language interpreter that provides a serial port shell environment
	for bit banging and hardware hacking.

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
/***

### Documentation for SD Card Support:

- Download and install the SDFat SD Card library to your libraries directory
	- tested with http://beta-lib.googlecode.com/files/SdFatBeta20120825.zip
	- on Arduino 1.0.1

- Edit bitlash/src/bitlash.h to turn on the SDFILE define: at or near line 340, make this:
		//#define SDFILE
look like this:
		#define SDFILE

- If you are using a Mega2560, edit SdFat/SdFatConfig.h @ line 85, make this change:
	#define MEGA_SOFT_SPI 1

- Restart Arduino, open examples->bitlash->bitlashsd and upload to your Arduino

- Connect with a serial monitor and play


### Commands supported in the bitlashsd sd card demo

	dir
	exists("filename") 
	del("filename") 
	create("filename", "first line\nsecondline\n")
	append("filename", "another line\n")
	type("filename") 
	cd("dirname")
	md("dirname")
	fprintf("filename", "format string %s%d\n", "foo", millis);

### Running Bitlash scripts from sd card

- put your multi-line function script on an SD card and run it from the command line
	- example: bitlashcode/memdump
	- example: bitlashcode/vars

- //comments work (comment to end of line)

- functions are looked up in this priority / order:
	- internal function table
	- user C function table (addBitlashFunction(...))
	- Bitlash functions in EEPROM
	- Bitlash functions in files on SD card

- beware name conflicts: a script on SD card can't override a function in EEPROM

- BUG: the run command only works with EEPROM functions
	- it does not work with file functions
	- for now, to use run with a file function, use this workaround:
		- make a small EEPROM function to call your file function
		- run the the EEPROM function

- startup and prompt functions on sd card are honored, if they exist

- you can upload files using bitlashcode/bloader.py
	- python bloader.py memdump md
		... uploads memdump as "md"

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
