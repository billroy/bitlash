/***
	bitlash-unix-file.c: Bitlash integration with POSIX file support

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
#include "bitlash.h"

#if defined(UNIX_BUILD)

FILE *scriptfile;
byte scriptfile_is_open;


// filename buffer for 8.3 + \0
#define FNAMELEN 13
char cachedname[FNAMELEN];
byte cachedflags;

// return true iff script exists
byte scriptfileexists(char *scriptname) {
	FILE *file;
	if ((file = fopen(scriptname, "r")) == NULL) return 0;
	fclose(file);
	return 1;
}

byte scriptclose(void) {
	if (scriptfile_is_open) fclose(scriptfile);
	scriptfile_is_open = 0;
    scriptfile = 0;
    *cachedname = 0;
	return 0;
}

// open and set parse location on input file
byte scriptopen(char *scriptname, numvar position, byte flags) {

	// open the input file if there is no file open, 
	// or the open file does not match what we want
	if (!scriptfile_is_open || strcmp(scriptname, cachedname) || (flags != cachedflags)) {
		if (scriptfile_is_open) scriptclose();
		scriptfile = fopen(scriptname, "r");
		if (!scriptfile) return 0;
		strcpy(cachedname, scriptname);		// cache the name we have open
		cachedflags = flags;				// and the mode
		scriptfile_is_open = 1;				// note it's open
		if (position == 0L) return 1;		// save a seek, when we can
	}
	extern off_t lseek(int fd, off_t offset, int whence);
	off_t seek_status = fseek(scriptfile, (off_t) position, SEEK_SET);
    int err = errno;
    if (seek_status == -1) return 0;
	return 1;
}


numvar scriptgetpos(void) {
	return ftell(scriptfile);
}

byte scriptread(void) {
	byte input;
	if (!fread(&input, 1, 1, scriptfile)) {
		scriptclose();
		return 0;		// eof
	}
	return input;
}

byte scriptwrite(char *filename, char *contents, byte append) {

///	if (scriptfile_is_open) {
///		if (!scriptfile.close()) return 0;
///	}

	FILE *outfile;
	char *flags;
	if (append) flags = "a";
	else flags = "w";

	if (scriptfile_is_open) scriptclose();
	scriptfile = fopen(filename, flags);
	if (!scriptfile) return 0;
	strcpy(cachedname, filename);		// cache the name we have open
	cachedflags = 1;					// and the mode
	scriptfile_is_open = 1;				// note it's open
	
    if (strlen(contents)) {
		if (fwrite(contents, 1, strlen(contents), outfile) != strlen(contents)) {
			//fclose(outfile);
			return 0;
		}
	}
	//fclose(outfile);
	return 1;
}

void scriptwritebyte(byte b) {
	// TODO: error check here
	fwrite(&b, 1, 1, scriptfile);
}


numvar sdls(void) {
	system("ls");
	return 0;
}
numvar sdexists(void) { 
	return scriptfileexists((char *) getarg(1)); 
}
numvar sdrm(void) { 
	return unlink((char *) getarg(1)); 
}
numvar sdcreate(void) { 
	return sdwrite((char *) getarg(1), (char *) getarg(2), 0); 
}
numvar sdappend(void) { 
	return sdwrite((char *) getarg(1), (char *) getarg(2), 1); 
}
numvar sdcd(void) {
	// close any cached open file handle
	if (scriptfile_is_open) scriptclose();
	return chdir((char *) getarg(1));
}
numvar sdmd(void) { 
	return mkdir((char *) getarg(1));
}

numvar exec(void) {
	return doCommand((char *) getarg(1));
}

numvar func_pwd(void) {
	system("pwd");
//	#define PWD_BUF_LEN 256
//	char buf[PWD_BUF_LEN];
//	getcwd(buf, PWD_BUF_LEN);
//	sp(buf); speol();
	return 0;
}


#if 0
void setup(void) {

	// initialize bitlash and set primary serial port baud
	// print startup banner and run the startup macro
	initBitlash(57600);

	addBitlashFunction("exec", (bitlash_function) exec);
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
#endif


#endif	// defined(UNIX_BUILD)