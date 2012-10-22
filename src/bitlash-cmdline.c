/***
	bitlash-cmdline.c

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
#include "bitlash.h"


// Serial command line buffer
char *lbufptr;
char lbuf[LBUFLEN];


// Help text
//
#if !defined(TINY_BUILD)
const prog_char helptext[] PROGMEM = { "http://bitlash.net\r\nSee LICENSE for license\r\nPins: d0-22,a0-22  Variables: a-z, 32 bit long integers\r\nOperators: + - * / ( ) < <= > >= == != << >> ! ^ & | ++ -- :=\r\nCommands: \0" };
#else
const prog_char helptext[] PROGMEM = { "http://bitlash.net\r\n\0" };
#endif

void showdict(const prog_char *addr) {
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
	showdict(reservedwords);
	msgp(M_functions);
#ifdef LONG_ALIASES
	showdict(aliasdict);
	speol();
#endif
	showdict(functiondict);
	speol();
	show_user_functions();
	speol();
	void cmd_ls(void);
	cmd_ls();
}


void prompt(void) {
char buf[IDLEN+1];

#if defined(TINY_BUILD)
	msgp(M_prompt);
#else
	// Run the script named "prompt" if there is one else print "> "
	strncpy_P(buf, getmsg(M_promptid), IDLEN);	// get the name "prompt" in our cmd buf
	if (findscript(buf)) doCommand(buf);
	else msgp(M_prompt);							// else print default prompt
#endif
}

void initlbuf(void) {
	lbufptr = lbuf;

#if defined(SERIAL_OVERRIDE) && 0
	// don't do the prompt in serialIsOverridden mode
	if (serialIsOverridden()) return;
#endif

	prompt();
	
	// flush any pending serial input
	while (serialAvailable()) serialRead();
}


// Add a character to the input line buffer; overflow if needed
byte putlbuf(char c) {
	if (lbufptr < lbuf + LBUFLEN - 2) {
		*lbufptr++ = c;
		spb(c);
		return 1;
	}
	else {
		spb(7);			// beep
		return 0;
	}
}

void pointToError(void) {
	if (fetchtype == SCRIPT_RAM) {
		int i = (char *) fetchptr - lbuf;
		if ((i < 0) || (i >= LBUFLEN)) return;
		speol();
		while (i-- >= 0) spb('-');
		spb('^'); speol();
	}
}


// run the bitlash command line editor and hand off commands to the interpreter
//
// call frequently, e.g. from loop(), or risk losing serial input
// note that doCommand blocks until it returns, so looping in a while() will delay other tasks
//

///////////////////////
//	handle a character from the input stream
// 	may execute the command, etc.
//
void doCharacter(char c) {

	if ((c == '\r') || (c == '\n')) {
		speol();
		*lbufptr = 0;
		doCommand(lbuf);
		initlbuf();
	}
	else if (c == 3) {		// ^C break/stop
		msgpl(M_ctrlc);
		initTaskList();
		initlbuf();
	}
#if !defined(TINY_BUILD)
	else if (c == 2) {			// ^B suspend Background macros
		suspendBackground = !suspendBackground;
	}
#endif
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
		//spb(7);
	}
#endif
#if !defined(TINY_BUILD)
	else if (c == 21) {		// ^U to get last line
		msgpl(M_ctrlu);
		prompt();
		sp(lbuf);
		lbufptr = lbuf + strlen(lbuf);
	}
#endif
	else putlbuf(c);
}


/////////////////////////////
//
//	runBitlash
//
//	This is the main entry point where the main loop gives Bitlash cycles
//	Call this frequently from loop()
//
void runBitlash(void) {

	// Pipe the serial input into the command handler
	if (serialAvailable()) doCharacter(serialRead());

	// Background macro handler: feed it one call each time through
	runBackgroundTasks();
}

