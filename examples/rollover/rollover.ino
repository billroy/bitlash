/***

	rollover.ino:	Bitlash Millis Overflow Test

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
	Bitlash millis rollover testing

	This test sketch allows a bitlash program to set millis() to an arbitrary value,
	which makes it easy to see what will happen when the millis counter rolls over
	after 49 days of operation.
	
	Here is a test case showing proper background task scheduling through the rollover:

	> function pm {print millis}
	> ls
	function pm {print millis};
	> setMillis(-500);run pm,50    
	> -450
	-400
	-350
	-300
	-250
	-200
	-150
	-100
	-50
	0
	50
	100
	150
	200
	250
	300
	350
	400
	450
	500
	^C

***/

#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif
#include "bitlash.h"

extern volatile unsigned long timer0_millis;

numvar setMillis(void) {
	uint8_t oldSREG = SREG;
	cli();
	timer0_millis = getarg(1);
	SREG = oldSREG;
	return 0;
}


void setup(void) {
	initBitlash(57600);		// must be first to initialize serial port

	addBitlashFunction("setmillis", (bitlash_function) setMillis);
}

void loop(void) {
	runBitlash();
}
