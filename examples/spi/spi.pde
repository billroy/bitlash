/***
	spi.pde:	Bitlash SPI User Function Code

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

// function handler for "spiput()" bitlash function
//
//	arg 1: byte to send via SPI
//
numvar spi_put(void) {
	if (!(SPCR & (1<<SPE))) {	// if SPI isn't initialized, do so now

		// Set output mode for MOSI and SCK
		// Caller responsible for MISO and SS and device select/enable logic
		pinMode(11, OUTPUT);
		pinMode(13, OUTPUT);

		// Initialize SPI to slowest speed (f_osc/128)
		SPCR = 1<<SPE | 1<<MSTR | 1<<SPR1 | 1<<SPR0;
		int junk = SPSR;
		junk = SPDR;
	}
	SPDR = getarg(1);
	while (!(SPSR & (1<<SPIF))) {;}		// could hang here...
	return SPDR;
}


void setup(void) {
	initBitlash(57600);		// must be first to initialize serial port

	// Register the extension function with Bitlash:
	// 		"spiput" is the name Bitlash will match for the function
	// 		(bitlash_function) spi_put is the C function handler declared above
	//
	addBitlashFunction("spiput", (bitlash_function) spi_put);
}

void loop(void) {
	runBitlash();
}
