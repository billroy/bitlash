//////////////////////////////////////////////////////////////////
//
//	pkt.h:	Packet Driver Interface
//
//	Copyright (C) 2009-2011 by Bill Roy
//
//	This library is free software; you can redistribute it and/or
//	modify it under the terms of the GNU Lesser General Public
//	License as published by the Free Software Foundation; either
//	version 2.1 of the License, or (at your option) any later version.
//
//	This library is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//	Lesser General Public License for more details.
//
//	You should have received a copy of the GNU Lesser General Public
//	License along with this library; if not, write to the Free Software
//	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
//
//////////////////////////////////////////////////////////////////
//
#ifndef _PKT_H
#define _PKT_H


////////////////////////////////////////////
// packet types
//
// 00-7f are reserved for the system
// 80-ff are available for user allocation
//
#define PKT_BOGON 	0			// reserved for bogon detection
#define PKT_SERIAL	1			// serial output broadcast
#define PKT_COMMAND 2			// bitlash commands, point-to-point or broadcast

#define PKT_USER	0x80		// 0x80-0xff are allocated for user application types


///////////////////////////////////
// packet buffer: the pkt_t
//
#if defined(RADIO_VIRTUALWIRE)
#define RF_PACKET_SIZE VW_MAX_PAYLOAD		// 27 when this was written
#else
#define RF_PACKET_SIZE 60
#endif


#define RF_PACKET_HEADER_SIZE 2
#define RF_PACKET_DATA_SIZE (RF_PACKET_SIZE - RF_PACKET_HEADER_SIZE)
typedef struct {
	byte type;
	byte sequence;
	union {
		byte data[RF_PACKET_DATA_SIZE];	
		// other payload types here
	};
} pkt_t;


// exported packet functions
numvar func_pktstat(void);
numvar func_rflog(void);
void log_packet(char, pkt_t *, byte);


#endif		// _PKT_H
