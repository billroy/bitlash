/***
	bitlash-clock.pde

	Bitlash is a tiny language interpreter that provides a serial port shell environment
	for bit banging and hardware hacking.

	This is the Bitlash2 library for Arduino 0014.

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
/*****

	Bitlash API Integration :: Wall Clock Example
	
	This is a sample Bitlash API integration.  
	
	It implements a wall clock, sometimes called a real-time clock, 
	based on the Arduino millis() timer.
	
	INSTALL
	
	Paste bitlash.cpp and this file time.cpp into an Arduino sketch window.
	Modify loop() to look like this:

		void loop(void) {
			runClock();			// <-- add this line
			runBitlash();
		}

	Upload and play.

	
	CLOCK REGISTERS
	
	The time of the clock is automagically maintained in Bitlash variables:
		h:	the hour
		m:	the minute
		s:	the second
		d:	the day
		e:	the epoch (the value of millis() when d:h:m:s was the time)
	
	Think of these as your time registers.
	
	To set the time, set the variables at the Bitlash prompt:
	
		> h=12;m=30;s=12
	
	A note on 'd': the days variable simply counts the number of days since
	startup.  Implementing better day rollover including end-of-month
	handling with correction for leap years is left as an exercise for the
	reader.  Same with leap seconds.  And time zones.  And daylight savings
	time.
	
	CLOCK UPDATE
	
	The automagic time update happens in the runClock() function, which
	the sketch calls runClock() in loop(), along with runBitlash().  
	
	The runClock() function updates the Bitlash clock variables so that 
	your Bitlash script can use them at any time:
	
		> print "Time: ", h,":",m,":",s
	
	
	CLOCK MACROS: 
	
	The script also demonstrates running commands through the API by automatically
	looking for and running these Bitlash macros, if they are present, on the specified
	time events:
	
		onsecond	runs each second
		onminute	runs each minute at seconds rollover
		onhour		runs each hour at minutes rollover
		onday		runs each day at hour rollover
	
	For example, this will chime the hour on a piezo buzzer at pin 11:
	
		> checknoon := "if i==0: i=12"
		> onhour := "i=h; checknoon; while i--: beep 11,440,200; delay(1000)"
	
	Additional event types like onalarm and onleapsecond are left to the reader. ;)

	LICENSE

	Bitlash lives at: http://bitlash.net
	The author can be reached at: bill@bitlash.net

	Copyright (C) 2008 Bill Roy

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

*****/
#include "bitlash.h"

// variable indexes for epoch, hour, minute, second, and day
#define v_epoch ('e'-'a')
#define v_hour ('h'-'a')
#define v_minute ('m'-'a')
#define v_second ('s'-'a')
#define v_day ('d'-'a')


//	runClock -- update the clock variables
//
//	call this frequently in loop()
//
void runClock() {
unsigned long now = millis();
unsigned long dt = now - getVar(v_epoch);

	if (dt < 1000) return;		// nothing to see

	while (dt >= 1000) {		// tally the full seconds
		dt -= 1000;
		incVar(v_second);
		if (getValue("onsecond") >= 0) doCommand("onsecond");
		while (getVar(v_second) >= 60) {
			assignVar(v_second, getVar(v_second) - 60);
			incVar(v_minute);
			if (getValue("onminute") >= 0) doCommand("onminute");
			while (getVar(v_minute) >= 60) {
				assignVar(v_minute, getVar(v_minute) - 60);
				incVar(v_hour);
				if (getValue("onhour") >= 0) doCommand("onhour");
				while (getVar(v_hour) >= 24) {
					incVar(v_day);
					if (getValue("onday") >= 0) doCommand("onday");
					assignVar(v_hour, getVar(v_hour) - 24);
				}
			}
		}
	}
	assignVar(v_epoch, now - dt);	// put back remainder
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
	runClock();
}


