/***
	bitlash.cpp

	Bitlash is a tiny language interpreter that provides a serial port shell environment
	for bit banging and hardware hacking.

	This is the Bitlash library for Arduino 0014.

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
#include "WProgram.h"

#ifdef UNIX_BUILD
#include "src/bitlash-unix.c"
#else
#include "src/bitlash-arduino.c"
#endif

#include "src/bitlash-cmdline.c"
#include "src/bitlash-eeprom.c"
#include "src/bitlash-eh1.c"
#include "src/bitlash-error.c"
#include "src/bitlash-functions.c"
#include "src/bitlash-interpreter.c"
#include "src/bitlash-parser.c"
#include "src/bitlash-serial.c"
#include "src/bitlash-taskmgr.c"
#include "src/bitlash-api.c"
#include "src/eeprom.c"
