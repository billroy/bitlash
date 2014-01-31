/***
	bitlash-error.c

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
#include "bitlash.h"


void fatal2(char msg1, char msg2) { 

#ifdef SOFTWARE_SERIAL_TX
	resetOutput();
#endif

#if !defined(TINY_BUILD)
	pointToError(); 
#endif
	msgp(msg1); 
	if (msg2) msgpl(msg2); 
#ifdef PARSER_TRACE
	tb();
#endif

	//traceback();

	// Here we punt back to the setjmp in doCommand (bitlash.c)
	longjmp(env, X_EXIT);
}

void fatal(char msgid) { fatal2(msgid, 0); }
void expected(byte msgid) { fatal2(M_expected, msgid); }
void expectedchar(byte c) { 
#if !defined(TINY_BUILD)
	spb(c); 
#endif
	fatal(M_expected); 
}
void unexpected(byte msgid) { fatal2(M_unexpected, msgid); }
void missing(byte msgid) { fatal2(M_missing, msgid); }
void underflow(byte msgid) { fatal2(msgid, M_underflow); }
void overflow(byte msgid) { fatal2(msgid, M_overflow); }
//void toolong(void) { overflow(M_string); }
#if !defined(TINY_BUILD)
void oops(int errcode) { printInteger(errcode, 0, 0); fatal(M_oops); }
#endif
