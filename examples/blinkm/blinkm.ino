/***
	blinkm.ino:	Bitlash BlinkM integration
	
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
#include "Arduino.h"
#include "Wire.h"
#include "BlinkM_funcs.h"
#include "bitlash.h"

byte blinkm_addr = 0;
byte red, green, blue;

numvar func_setaddr(void) {
	blinkm_addr = getarg(1);
}

numvar func_setrgb(void) {
	red = getarg(1);
	green = getarg(2);
	blue = getarg(3);
	BlinkM_setRGB(blinkm_addr, red, green, blue);
}

numvar func_red(void) {
	red = getarg(1);
	BlinkM_fadeToRGB(blinkm_addr, red, green, blue);
}

numvar func_blue(void) {
	blue = getarg(1);
	BlinkM_fadeToRGB(blinkm_addr, red, green, blue);
}

numvar func_green(void) {
	green = getarg(1);
	BlinkM_fadeToRGB(blinkm_addr, red, green, blue);
}

numvar func_fadetorgb(void) {
	red = getarg(1);
	green = getarg(2);
	blue = getarg(3);
	BlinkM_fadeToRGB(blinkm_addr, red, green, blue);
}

numvar func_fadetohsb(void) {
	BlinkM_fadeToHSB(blinkm_addr, getarg(1), getarg(2), getarg(3));
}

numvar func_setfade(void) {
	BlinkM_setFadeSpeed(blinkm_addr, getarg(1));
}

void setup(void) {
	initBitlash(57600);		// must be first to initialize serial port

	BlinkM_beginWithPower();
	BlinkM_stopScript(blinkm_addr);  // turn off startup script

	addBitlashFunction("fadergb", (bitlash_function) func_fadetorgb);
	addBitlashFunction("fadehsb", (bitlash_function) func_fadetohsb);
	addBitlashFunction("setrgb", (bitlash_function) func_setrgb);
	addBitlashFunction("red", (bitlash_function) func_red);
	addBitlashFunction("green", (bitlash_function) func_green);
	addBitlashFunction("blue", (bitlash_function) func_blue);
	addBitlashFunction("setfade", (bitlash_function) func_setfade);
	addBitlashFunction("setaddr", (bitlash_function) func_setaddr);
}

void loop(void) {
	runBitlash();
}
