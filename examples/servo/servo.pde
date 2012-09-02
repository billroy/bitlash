/***
	servo.pde:	Bitlash Servo User Function Code

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

#include "Servo.h"
#define NUMSERVOS 8			// bump this for more per http://www.arduino.cc/en/Reference/Servo
byte servo_install_count;	// number of servos that have been initialized
byte servo_pin[NUMSERVOS];	// the pins of those servos
Servo myservos[NUMSERVOS];	// the Servo object for those servos

//////////
// servohandler: function handler for Bitlash servo() function
//
//	arg 1: servopin
//	arg 2: setting
//
numvar servohandler(void) {
byte slot = 0;
	// is this pin already allocated a servo slot?
	while (slot < servo_install_count) {
		if (servo_pin[slot] == getarg(1)) break;
		slot++;
	}
	if (slot >= servo_install_count) {	// not found

		// do we have a free slot to allocate?
		if (servo_install_count < NUMSERVOS) {
			slot = servo_install_count++;
			servo_pin[slot] = getarg(1);
			myservos[slot].attach(getarg(1));
		}
		else {
			Serial.println("servohandler: oops!");	// please increase NUMSERVOS above
			return -1;
		}
	}
	myservos[slot].write(getarg(2));
	return 0;
}

void setup(void) {
	initBitlash(57600);		// must be first to initialize serial port

	// Register the extension function with Bitlash:
	// 		"servo" is the name Bitlash will match for the function
	// 		(bitlash_function) servohandler is the C function handler declared above
	//
	addBitlashFunction("servo", (bitlash_function) servohandler);
}

void loop(void) {
	runBitlash();
}
