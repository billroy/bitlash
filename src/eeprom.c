/***
	eeprom.c:	minimal eeprom interface

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

#if (defined(AVR_BUILD)) || ( (defined(ARM_BUILD)) && (ARM_BUILD==2))
	// AVR or Teensy 3
	#include "avr/eeprom.h"
	void eewrite(int addr, uint8_t value) { eeprom_write_byte((unsigned char *) addr, value); }
	uint8_t eeread(int addr) { return eeprom_read_byte((unsigned char *) addr); }
	#if defined(ARM_BUILD)
		// Initialize Teensy 3 eeprom
		void eeinit(void) { eeprom_initialize(); }
	#endif
#elif defined(ARM_BUILD)
	#if ARM_BUILD!=2
		// A little fake eeprom for ARM testing
		char virtual_eeprom[E2END];

		void eeinit(void) {
			for (int i=0; i<E2END; i++) virtual_eeprom[i] = 255;
		}

		void eewrite(int addr, uint8_t value) { virtual_eeprom[addr] = value; }
		uint8_t eeread(int addr) { return virtual_eeprom[addr]; }
	#endif
#endif
