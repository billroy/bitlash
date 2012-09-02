/***
	pkt.h:	Packet Driver Interface

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
#elif defined(RADIO_RF12)
#define RF_PACKET_SIZE RF12_MAXDATA			// 66 = RF12_MAXDATA
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
