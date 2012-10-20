/***
	bitlash.cpp

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
#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#ifdef UNIX_BUILD
#include "src/bitlash-unix.c"
//#else
//#include "src/bitlash-arduino.c"
#endif

#include "src/bitlash-cmdline.c"
#include "src/bitlash-eeprom.c"
#include "src/bitlash-error.c"
#include "src/bitlash-functions.c"
#include "src/bitlash-builtins.c"
#include "src/bitlash-interpreter.c"
#include "src/bitlash-instream.c"
#include "src/bitlash-parser.c"
#include "src/bitlash-serial.c"
#include "src/bitlash-taskmgr.c"
#include "src/bitlash-api.c"
#include "src/eeprom.c"
