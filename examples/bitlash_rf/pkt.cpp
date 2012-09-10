/***
	pkt.c:	Packet Driver Interface

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
#include "../../libraries/bitlash/src/bitlash.h"
#include "bitlash_rf.h"


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
numvar func_pktstat(void) {
	sp("tx pkts:"); printInteger(tx_packet_count, 0, ' ');
	sp(" rx pkts:"); printInteger(rx_packet_count, 0, ' ');
	sp(" bogons:"); printInteger(rx_bogon_count, 0, ' ');
	speol();
}


//////////
// rf_log_packet: Hex dump the packet to the console
//
byte rf_logbytes;

// a function handler to expose the control
numvar func_rflog(void) { rf_logbytes = getarg(1); }

// log-print-byte
//
// NOTE: these functions must use Serial.print(), not spb(), since they are
// invoked during transmit to log transmitted packets WHILE the spb() output
// redirection mechanism is engaged.
// Using spb() here sends the tx log to the other host.
// Unfortunately this limits us to logging locally.
//
void lpb(byte b) {
	if (b == '\\') {
		Serial.print('\\');
		Serial.print('\\');
	}
	else if ((b >= ' ') && (b <= 0x7f)) Serial.print(b, DEC);
	else {
		Serial.print('\\');
		if (b == 0xd) Serial.print('r');
		else if (b == 0xa) Serial.print('n');
		else {
			Serial.print('x');
			if (b < 0x10) Serial.print('0');
			Serial.print(b, HEX);
		}
	}
}

// log a packet
//
void log_packet(char tag, pkt_t *pkt, byte length) {
	if (rf_logbytes) {
		int i = 0;
		int last = (length < rf_logbytes) ? length : rf_logbytes;
		Serial.print('[');
		Serial.print(tag); 
		Serial.print("X ");
		Serial.print(length, DEC); Serial.print(' ');
		Serial.print(pkt->type, DEC); Serial.print(' ');
		Serial.print(pkt->sequence, DEC); Serial.print(' ');
		while (i < last-RF_PACKET_HEADER_SIZE) {
			lpb(pkt->data[i++]);
		}
		Serial.println(']');
	}
}
