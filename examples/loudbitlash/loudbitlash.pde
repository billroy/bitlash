/***

	loudbitlash.pde:	Bitlash Serial Override Handler Test

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
#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif
#include "bitlash.h"

byte makeLoud = 0;

void printUpper(byte b) {
	if (makeLoud && (b >= 'a') && (b <= 'z')) {
#if defined(ARDUINO) && ARDUINO >= 100
		Serial.write(b - 'a' + 'A');
#else
		Serial.print(b - 'a' + 'A', BYTE);
#endif
	}
	else {
#if defined(ARDUINO) && ARDUINO >= 100
		Serial.write(b);
#else
		Serial.print(b, BYTE);
#endif
	}
}

numvar setLoudness(void) {
	makeLoud = getarg(1);
	return 0;
}


void setup(void) {
	initBitlash(57600);		// must be first to initialize serial port

	setOutputHandler(&printUpper);

	// Register an extension function to control this charming behavior
	addBitlashFunction("loud", (bitlash_function) setLoudness);
}

void loop(void) {
	runBitlash();
}
