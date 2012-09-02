/*
	bitlash-unix.c: A minimal implementation of certain core Arduino functions	
	
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
*/
#include "bitlash.h"

/*
issues

auto detect gcc for build, set unix_build flag
serialAvailable
serialRead
serialWrite
millis and delay
eeprom functions
	virtual, from a disk file
pgm_read_byte and pgm_read_word
- needs parser context mods

setbaud
EEMEM
*/

int serialAvailable(void) { return 1; }
int serialRead(void) { return getch(); }
void serialWrite(int c) { putch(c); }

numvar setBaud(numvar, unumvar) { return 0; }

// stubs for the hardware IO functions
//
unsigned long pins;
void pinMode(byte pin, byte mode) { ; }
int digitalRead(byte pin) { return ((pins & (1<<pin)) != 0); }
void digitalWrite(byte pin, byte value) {
	if (value) pins |= 1<<pin;
	else pins &= ~(1<<pin);
}
int analogRead(byte pin) { return 0; }
void analogWrite(byte pin, int value) { ; }
int pulseIn(int pin, int mode, int duration) { return 0; }

// stubs for the time functions
//
unsigned long millis(void) { return 0; }
void delay(unsigned long ms) {
	unsigned long start = millis();
	while (millis() - start < ms) { ; }
}
void delayMicroseconds(unsigned int us) {;}

