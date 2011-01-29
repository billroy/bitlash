/*
	bitlash-unix.c: A minimal implementation of certain core Arduino functions	
	
	The author can be reached at: bill@bitlash.net

	Copyright (C) 2011 Bill Roy

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
kludge and dekludge
isram
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

