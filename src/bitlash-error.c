/***
	bitlash-error.c

	Bitlash is a tiny language interpreter that provides a serial port shell environment
	for bit banging and hardware hacking.

	See the file README for documentation.

	Bitlash lives at: http://bitlash.net
	The author can be reached at: bill@bitlash.net

	Copyright (C) 2008-2011 Bill Roy

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

***/
#include "bitlash.h"


void fatal2(char msg1, char msg2) { 

#ifdef SOFTWARE_SERIAL_TX
	resetOutput();
#endif

#if defined(SOFTWARE_SERIAL_TX) || defined(HARDWARE_SERIAL_TX)
	pointToError(); 
	msgp(msg1); 
	if (msg2) msgpl(msg2); 
#ifdef PARSER_TRACE
	tb();
#endif
#endif

	// Here we punt back to the setjmp in doCommand (bitlash.c)
	longjmp(env, X_EXIT);
}

void fatal(char msgid) { fatal2(msgid, 0); }
void expected(byte msgid) { fatal2(M_expected, msgid); }
void expectedchar(byte c) { 
#if !defined(TINY85)
	spb(c); 
#endif
	fatal(M_expected); 
}
void unexpected(byte msgid) { fatal2(M_unexpected, msgid); }
void missing(byte msgid) { fatal2(M_missing, msgid); }
void underflow(byte msgid) { fatal2(msgid, M_underflow); }
void overflow(byte msgid) { fatal2(msgid, M_overflow); }
//void toolong(void) { overflow(M_string); }
#if !defined(TINY85)
void oops(int errcode) { printInteger(errcode); fatal(M_oops); }
#endif

