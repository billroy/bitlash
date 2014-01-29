/***
	bitlash-taskmgr.c

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
			snoozetime[slot] = snoozems;

			// eligible to run at the end of 1 tick
			waketime[slot] = millis() + snoozems;
//			waketime[slot] = millis();		// eligible to run now
			return;
		}
	}
	overflow(M_id);
}


void snooze(unumvar duration) {
	if (background) snoozetime[curtask] = duration;
	else delay(duration);
}


//////////
//
//	runBackgroundTasks
//
//	Runs one eligible background task per invocation
//	Returns true if a task was run
//
void runBackgroundTasks(void) {
byte i;	

#ifdef suspendBackground
	if (suspendBackground) return;
#endif

	for (i=0; i<NUMTASKS; i++) {
		// run one task per call on a round robin basis
		if (++curtask >= NUMTASKS) curtask = 0;
		if ((tasklist[curtask] != SLOT_FREE) && 
			(((signed long) millis() - (signed long) waketime[curtask])) >= 0) {

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

unsigned long millisUntilNextTask(void) {
byte slot;
	long next_wake_time = millis() + 500L;
	for (slot=0; slot<NUMTASKS; slot++) {
		if (tasklist[slot] != SLOT_FREE) {
			if (waketime[slot] < next_wake_time) next_wake_time = waketime[slot];
		}
	}
	long millis_to_wait = next_wake_time - millis();
	if (millis_to_wait < 0) millis_to_wait = 0;
	return millis_to_wait;			// millis until next task runs
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


