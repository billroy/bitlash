//////////////////////////////////////////////////////////////////
//
//	pkt.c:	Packet Driver Interface
//
//	Copyright (C) 2009, 2010 Palmeta Productions LLC
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
//////////////////////////////////////////////////////////////////
//
#include "parfait.h"


///////////////
// Packet buffer management
///////////////
byte tx_index;		// index to next free space in tx_buf.data[]
byte pkt_sequence;	// packet sequence byte
pkt_t tx_buf;		// the tx packet buffer
pkt_t rx_buf;		// rx buffer used in the receive loop

// packet counters
unsigned tx_packet_count;
unsigned rx_packet_count;
unsigned rx_bogon_count;


//////////
// pkt_flush: flush any pending data in the packet buffer to tx
//
void pkt_flush(void) {
	if (tx_index) {
		tx_buf.sequence = pkt_sequence++;
		tx_send_pkt(&tx_buf, tx_index + RF_PACKET_HEADER_SIZE);
		tx_index = 0;		// reset for next byte
	}
}

//////////
// pkt_put: add a byte to the buffer and transmit on buffer full
//
void pkt_put(byte b) {
	tx_buf.data[tx_index++] = b;
	if (tx_index >= RF_PACKET_DATA_SIZE) {	// full?
		pkt_flush();						// transmit
 	}
}

///////////////////////////////
// send serial output
//
void send_serial(byte b) {
	tx_buf.type = PKT_SERIAL;
	pkt_put(b);
}

//////////////////////
//
//	send_command_byte
//
//	Transmit a command according to previously established network parameters.
//
void send_command_byte(byte b) { 
	tx_buf.type = PKT_COMMAND;
	pkt_put(b);
}



//////////
// dump packet stats for debugging
//
void pkt_dump(void) {

	sp("tx pkts:"); printInteger(tx_packet_count);
	sp(" rx pkts:"); printInteger(rx_packet_count);
	sp(" bogons:"); printInteger(rx_bogon_count);
	speol();

#if 0
	sp("TCNT1:"); speol(TCNT1, HEX);
	sp("SPCR:"); speol(SPCR, HEX);
	sp("SPSR:"); speol(SPSR, HEX);
	sp("TCCR1A:"); speol(TCCR1A, HEX);
	sp("TCCR1B:"); speol(TCCR1B, HEX);
	sp("TIMSK1:"); speol(TIMSK1, HEX);
	sp("OCR1A:"); speol(OCR1A, HEX);
	sp("TCNT1:"); speol(TCNT1, HEX);
	sp("DDRB:"); speol(DDRB, HEX);
#endif
}

