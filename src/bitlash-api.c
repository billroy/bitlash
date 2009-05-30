/***
	bitlash-api.c

	Bitlash is a tiny language interpreter that provides a serial port shell environment
	for bit banging and hardware hacking.

	See the file README for documentation.  Or just upload this file as a sketch and play.

	Bitlash lives at: http://bitlash.net
	The author can be reached at: bill@bitlash.net

	Copyright (C) 2008, 2009 Bill Roy

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


// doCommand: main entry point to execute a bitlash command
//
void doCommand(char *cmd) {

	// initialize error code storage
	errorcode1 = errorcode2 = 0;

	// Exceptions come here via longjmp
	switch(setjmp(env)) {
		case 0: break;
		case X_EXIT: {
			initTaskList();		// stop the infernal machine
#ifdef SOFTWARE_SERIAL_TX
			resetOutput();
			return;
#endif

#ifdef TINY85
			for (;;) {
				// Error: SOS
				flash(1,1000);
				delay(500);
				flash(3,100);
				flash(3,300);
				flash(3,100);
				delay(500);
				flash(errorcode1, 200);
				delay(500);
				flash(errorcode2, 200);			
				delay(200);
			}
#endif

		}
	}

	vinit();			// initialize the expression stack
	fetchptr = cmd;		// point to it; next round pre-increments	
	primec();			// prime inchar
	getsym();
	if (sym == s_eof) return;

	getstatementlist();
}


#ifdef TINY85
void flash(unsigned int count, int ontime) {
	delay(ontime);
	while (count--) {
		digitalWrite(1,1);
		delay(ontime);
		digitalWrite(1,0);
		delay(ontime);
	}
}
#endif


#ifdef TINY85
void initBitlash(void) {
	// force our eeprom program to be included and run
	doCommand(kludge((int)startup));
}
#else

void initBitlash(unsigned long baud) {
	beginSerial(baud);
	displayBanner();

	// Run the script named "startup" if there is one
	strncpy_P(lbuf, getmsg(M_startup), STRVALLEN);	// get the name "startup" in our cmd buf
	if (findKey(lbuf) >= 0) doCommand(lbuf);		// look it up.  exists?  call it.

	initlbuf();
}
#endif



#if 0
//
// Here is sample code a simple bitlash integration.
//
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
#endif


// end bitlash.c
