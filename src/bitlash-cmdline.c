/***
	bitlash-cmdline.c

	Bitlash is a tiny language interpreter that provides a serial port shell environment
	for bit banging and hardware hacking.

	See the file README for documentation.

	Bitlash lives at: http://bitlash.net
	The author can be reached at: bill@bitlash.net

	Copyright (C) 2008, 2009, 2010 Bill Roy

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


// Serial command line buffer
byte remoteOperation;		// set BITLASH_RQ_EXECUTE execute contents of line buffer on next runBitlash
char *lbufptr;
char lbuf[LBUFLEN];


#if !defined(TINY85)

// Help text
//
#ifdef ARDUINO_BUILD
prog_char helptext[] PROGMEM = { "http://bitlash.net\r\nSee LICENSE for license\r\nPins: d0-22,a0-22  Variables: a-z, 32 bit long integers\r\nOperators: + - * / ( ) < <= > >= == != << >> ! ^ & | ++ -- :=\r\nCommands: \0" };
#else
prog_char helptext[] PROGMEM = { "http://bitlash.net\r\n\0" };
#endif

void showdict(prog_char *addr) {
byte c;
	for (;;) {
		c = pgm_read_byte(addr++);
		if (c == 255) return;
		else if (c == 0) {
			if (pgm_read_byte(addr) == 0) return;
			else spb(' ');
		}
		else if (c != '`') spb(c);
	}
}

void displayBanner(void);

void cmd_help(void) {
	displayBanner();
	showdict(helptext);
	// TODO: extern this puppy
	//showdict(reservedwords);
	msgp(M_functions);
#ifdef LONG_ALIASES
	showdict(aliasdict);
	speol();
#endif
	showdict(functiondict);
	speol();
	speol();
	void cmd_ls(void);
	cmd_ls();
}


void prompt(void) {
char buf[IDLEN+1];
	// Run the script named "prompt" if there is one else print "> "
	strncpy_P(buf, getmsg(M_promptid), IDLEN);	// get the name "prompt" in our cmd buf
	int entry = findKey(buf);
	if (entry >= 0) doCommand(kludge(findend(entry)));
	else msgp(M_prompt);							// else print default prompt
}

void initlbuf(void) {
	lbufptr = lbuf;

#ifndef TINY85
	prompt();
	
	// flush any pending serial input
	while (serialAvailable()) serialRead();
#endif
}


// Add a character to the input line buffer; overflow if needed
byte putlbuf(char c) {
	if (lbufptr < lbuf + LBUFLEN - 1) {
		*lbufptr++ = c;
		return 1;
	}
	return 0;		// TODO: overflow(M_line); ?
}

void pointToError(void) {
	if (isram(fetchptr)) {
		int i = fetchptr - lbuf;
		if ((i < 0) && (i >= 80)) return;
		while (i-- >= 0) spb(' ');
		spb('^'); speol();
	}
}
#endif


#ifdef AVROPENDOUS_BUILD
byte bitlashBackgroundEvent;
void connectBitlash(void) { bitlashBackgroundEvent = 'c'; }
#include "stdbool.h"
#endif


// run the bitlash command line editor and hand off commands to the interpreter
//
// call frequently, e.g. from loop(), or risk losing serial input
// note that doCommand blocks until it returns, so looping in a while() will delay other tasks
//

// TODO: change to lastval when linker bug is fixed
numvar lastval;			// last return value

#ifdef TINY85

void runBitlash(void) {

	// Process a command from the buffer, if it's ready
	if (remoteOperation == BITLASH_RQ_EXECUTEBUFFER) {
		//remoteOperation = BITLASH_RQ_BUSY;
		doCommand(lbuf);
		lastval = expval;
		initlbuf();
		remoteOperation = BITLASH_RQ_NULL;
	}
	runBackgroundTasks();
}

#else

void runBitlash(void) {

#ifdef AVROPENDOUS_BUILD
	if (bitlashBackgroundEvent == 'c') {
		if (serialAvailable()) {
			bitlashBackgroundEvent = 0;
			displayBanner();
			initlbuf();
		}
	}
#endif

	if (serialAvailable()) {
		if (lbufptr >= &lbuf[LBUFLEN-1]) overflow(M_line);
		char c = serialRead();

#if 0
		setOutput(9);
		//spb(c);
		spb('['); printHex(unumvar (c & 0xff)); spb(']');
		resetOutput();
#endif

		if ((c == '\r') || (c == '\n') || (c == '`')) {
			speol();
			*lbufptr = 0;
			doCommand(lbuf);
			initlbuf();
		} else if (c == 3) {		// ^C break/stop
			msgpl(M_ctrlc);
			initTaskList();
			initlbuf();
		}
		else if (c == 2) {			// ^B suspend Background macros
			suspendBackground = !suspendBackground;
		}
		else if ((c == 8) || (c == 0x7f)) {
			if (lbufptr == lbuf) spb(7);		// bell
			else {
				spb(8); spb(' '); spb(8);
				*(--lbufptr) = 0;
			}
		} 
#ifdef PARSER_TRACE
		else if (c == 20) {		// ^T toggle trace
			trace = !trace;
			spb(7);
		}
#endif
		else if (c == 21) {		// ^U to get last line
			msgpl(M_ctrlu);
			prompt();
			sp(lbuf);
			lbufptr = lbuf + strlen(lbuf);
		}
#if 0
		else if (c == 22) {
			for (int i=0; i<NUMTASKS; i++) { printHex((unsigned long) tasklist[i]); spb(' '); } speol(); 
		}
#endif
		else {
			spb(c);
			*lbufptr++ = c;
		}
	}

	// Background macro handler: feed it one call each time through
	runBackgroundTasks();
}
#endif



#if !defined(TINY85)
//	Banner and copyright notice
//
prog_char banner[] PROGMEM = { 
// Ruler:     1                   2         3         4         5         6         7         8         9        10
//   12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890
#ifdef ARDUINO_BUILD
	"print \"bitlash here! v1.1 (c) 2010 Bill Roy -type HELP-\",free,\"bytes free\""
#else
	"print \"bitlash here! v1.1 (c) 2010 Bill Roy\""
#endif
};

void displayBanner(void) {
	// print the banner and copyright notice
	// please note the license requires that you maintain this notice
	strncpy_P(lbuf, banner, STRVALLEN);
	doCommand(lbuf);
}
#endif





