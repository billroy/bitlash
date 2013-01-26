/***
	commander.pde:	Bitlash Commander Client
	
	Copyright (C) 2013 Bill Roy

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

/*

This sketch contains a few functions that make it easy to work with Bitlash Commander.

The update functions format and print a JSON upstream command to Commander to update
the value of a control:

1. update("id", value)

Sends an update to Commander setting the control whose id is "id" to the numeric value given.
The value must be numeric or a numeric expression.

2. updatestr("id", "value")

Sends an update to Bitlash setting the control whose id is "id" to the string value given.
The value must be a string constant in quotes.

3. tone(pin, freq, duration) and notone(pin)

Thrown in for demos.

*/

#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif
#include "bitlash.h"
#include "../bitlash/src/bitlash.h"


numvar func_update() {
	if (getarg(0) == 2) {
		sp("{\"id\":\"");
		sp((char *) getarg(1));
		sp("\",\"value\":");
		printIntegerInBase(getarg(2), 10, 0, 0);
		sp("}\n");
	}
	return 0;
}

numvar func_updatestr() {
	if (getarg(0) == 2) {
		sp("{\"id\":\"");
		sp((char *) getarg(1));
		sp("\",\"value\":\"");
		sp((char *) getarg(2));
		sp("\"}\n");
	}
	return 0;
}


// function handler for "tone()" bitlash function
//
//	arg 1: pin
//	arg 2: frequency
//	arg 3: duration (optional)
//
numvar func_tone(void) {
	if (getarg(0) == 2) tone(getarg(1), getarg(2));
	else tone(getarg(1), getarg(2), getarg(3));
	return 0;
}

numvar func_notone(void) {
	noTone(getarg(1));
	return 0;
}


void setup(void) {
	initBitlash(57600);		// must be first to initialize serial port

	addBitlashFunction("update", (bitlash_function) func_update);
	addBitlashFunction("updatestr", (bitlash_function) func_updatestr);

	addBitlashFunction("tone", (bitlash_function) func_tone);
	addBitlashFunction("notone", (bitlash_function) func_notone);
}

void loop(void) {
	runBitlash();
}
