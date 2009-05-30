/***
	bitlash-request.h

	Bitlash is a tiny language interpreter that provides a serial port shell environment
	for bit banging and hardware hacking.

	See the file README for documentation.

	Bitlash lives at: http://bitlash.net
	The author can be reached at: bill@bitlash.net

	Copyright (C) 2008, 2009 Bill Roy

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

// Bitlash Custom USB Request codes
//
#define BITLASH_RQ_NULL		 0	// reserved
#define BITLASH_RQ_DOCOMMAND 1
#define BITLASH_RQ_GETSTATUS 2
//#define BITLASH_RQ_EEWRITE   3	// deprecated
#define BITLASH_RQ_EXECUTEBUFFER 4
#define BITLASH_RQ_BUSY 5

#define BUSY_SIGNAL -999
