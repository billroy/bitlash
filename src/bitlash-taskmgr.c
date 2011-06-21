/***
	bitlash-taskmgr.c

	Bitlash is a tiny language interpreter that provides a serial port shell environment
	for bit banging and hardware hacking.

	See the file README for documentation.

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


// Background task manager
#define NUMTASKS 10
byte background;
byte suspendBackground;
byte curtask;

// BUG: this fails for eeproms > 64k in size
int tasklist[NUMTASKS];				// EEPROM address of text of the function
numvar snoozetime[NUMTASKS];		// time between task invocations
unsigned long waketime[NUMTASKS];	// millis() time this task is eligible to run

#define SLOT_FREE -1

void initTaskList(void) { 
	memset(tasklist, 0xff, NUMTASKS * sizeof(tasklist[0]));

	//+60 bytes
	//for (byte slot = 0; (slot < NUMTASKS); slot++) tasklist[slot] = SLOT_FREE;
}

void stopTask(byte slot) { if (slot < NUMTASKS) tasklist[slot] = SLOT_FREE; }

// add task to run list
void startTask(int macroid, numvar snoozems) {
byte slot;
	for (slot = 0; (slot < NUMTASKS); slot++) {
		if (tasklist[slot] == SLOT_FREE) {
			tasklist[slot] = macroid;
			waketime[slot] = millis();		// eligible to run now
			snoozetime[slot] = snoozems;
			return;
		}
	}
	overflow(M_id);
}


void snooze(unumvar duration) {
	if (background) snoozetime[curtask] = duration;
	else delay(duration);
}


void runBackgroundTasks(void) {
byte i;	

#ifdef suspendBackground
	if (suspendBackground) return;
#endif

	for (i=0; i<NUMTASKS; i++) {
		// run one task per call on a round robin basis
		if (++curtask >= NUMTASKS) curtask = 0;
		if ((tasklist[curtask] != SLOT_FREE) && (millis() >= waketime[curtask])) {

			// run it with the background flag set
			background = 1;
			execscript(SCRIPT_EEPROM, findend(tasklist[curtask]), 0);

			// schedule the next time quantum for this task
			waketime[curtask] = millis() + snoozetime[curtask];
			background = 0;
			break;
		}
	}
}


void showTaskList(void) {
byte slot;
	for (slot = 0; slot < NUMTASKS; slot++) {
		if (tasklist[slot] != SLOT_FREE) {
			printInteger(slot, 0, ' '); spb(':'); spb(' ');
			eeputs(tasklist[slot]); speol();
		}
	}
}


