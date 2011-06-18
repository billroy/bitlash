//////////////////////////////////////////////////////////////////
//
//	radio_virtualwire.cpp:	Virtual Wire Radio Interface for Bitlash
//
//	Copyright (C) 2011 by Bill Roy
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

//
// Requires VirtualWire 1.5 library from: 
//	http://www.open.com.au/mikem/arduino/VirtualWire-1.5.zip
//
// VirtualWire doc at: 
//	http://www.open.com.au/mikem/arduino/VirtualWire.pdf
//

#include "WProgram.h"
#include "bitlash.h"
#include "../../libraries/bitlash/src/bitlash.h"
#include "bitlash_rf.h"
#include "pkt.h"

#if defined(RADIO_VIRTUALWIRE)		// GUARDS THIS WHOLE FILE


////////////////////////////////////
// Turn this on to enable debug spew
#define RADIO_DEBUG


/////////////////////////////////////
// 
// Initialize the radio interface
//
void init_radio(void) {
	vw_set_ptt_pin(10);		// defaults to 10
	vw_set_rx_pin(11);		// defaults to 11
	vw_set_tx_pin(12);		// defaults to 12
	vw_setup(2000);			// set up for 2000 bps
	vw_rx_start();			// start the rx
	radio_go = 1;			// mark the rf system "up"
}


////////////////////////////////////////
//
// Address handling
//
//	No address handling for VirtualWire
//	These stubs are required for the API
//
void rf_set_rx_address(char *my_address) {;}
void rf_set_tx_address(char *to_address) {;}

////////////////////////////////////////
//
//	Return TRUE when the radio has a packet for us
//
byte rx_pkt_ready(void) {
	return vw_have_message();
}


// declare the packet logging function
void log_packet(char tag, pkt_t *pkt, byte length);


//////////////////////
// rx_fetch_pkt: read packet into buffer if available
//
//	returns number of bytes transferred to pkt, 0 if no data available
//
byte rx_fetch_pkt(pkt_t *pkt) {

	// Does the radio have business for us?
	//if (!vw_have_message()) return 0;

	//	"If a message is available (good checksum or not), copies up to *len octets to buf. 
	//	Returns true if there was a message and the checksum was good."
	byte length = RF_PACKET_SIZE;
	if (!vw_get_message((uint8_t *) pkt, &length)) {
		//sp("rx: bad chksum\r\n");
		return 0;
	}
	// If the packet length is bigger than our buffer We Have A Situation.
	// Clip to our max packet size, silently discarding the excess.
	//
	if (length > RF_PACKET_SIZE) {
		// todo: log here
		length = RF_PACKET_SIZE;
	}
	rx_packet_count++;						// got one? count it

#if defined(RADIO_DEBUG)
	log_packet('R', pkt, length);
#endif

	return length;
}


//////////////////////
//
// tx_send_packet: transmit a data packet
//
//	length: length of raw packet including header
//	pkt:	pointer to packet
//
void tx_send_pkt(pkt_t *pkt, uint8_t length) {

#if defined(RADIO_DEBUG)
	log_packet('T', pkt, length);
#endif

	vw_wait_tx();	// wait for tx idle
	vw_send((uint8_t *) pkt, length);		// todo: handle error here
	tx_packet_count++;
}


#endif	// defined(RADIO_VIRTUALWIRE)