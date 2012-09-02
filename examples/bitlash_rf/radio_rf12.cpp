/***

	radio_rf12.cpp:	HopeRF RF12 Radio Interface for Bitlash

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

// Dependencies
//
// Per this web article: http://ichilton.github.com/nanode/
//
// If you are using Arduino 0022 or 0023 you need these libraries:
//		RF12 library: 		http://jeelabs.org/pub/snapshots/RF12.zip
//		Ports library:		http://jeelabs.org/pub/snapshots/RF12.zip
//
// If you are using Arduino 1.0
//		JeeLib library		https://github.com/jcw/jeelib
//		You will need to adjust the includes in bitlash_rf.h and bitlash_rf.pde
//

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#include "bitlash.h"
#include "../../libraries/bitlash/src/bitlash.h"
#include "bitlash_rf.h"
#include "pkt.h"

#if defined(RADIO_RF12)		// GUARDS THIS WHOLE FILE

////////////////////////////////////
// Turn this on to enable debug spew
#define RADIO_DEBUG

#define DEFAULT_NETID		30				// 30 for receive-any
#define DEFAULT_NETGROUP	200				// 212 is nanode default
#define DEFAULT_BAND		RF12_915MHZ		// RF12_433MHZ, RF12_868MHZ, RF12_915MHZ

/////////////////////////////////////
// 
// Initialize the radio interface
//
void init_radio(void) {

	// init the radio driver with the parameters defined above
	rf12_initialize(DEFAULT_NETID, DEFAULT_BAND, DEFAULT_NETGROUP);
	radio_go = 1;			// mark the rf system "up"
}


////////////////////////////////////////
//
// Address handling
//
//	TODO: address handling
//	These stubs are required for the API
//
void rf_set_rx_address(char *my_address) {;}
void rf_set_tx_address(char *to_address) {;}

////////////////////////////////////////
//
//	Return TRUE when the radio has a packet for us
//
byte rx_pkt_ready(void) {
	return (rf12_recvDone() && (rf12_crc == 0));
}


// declare the packet logging function
void log_packet(char tag, pkt_t *pkt, byte length);


//////////////////////
// rx_fetch_pkt: read packet into buffer if available
//
//	returns number of bytes transferred to pkt, 0 if no data available
//
byte rx_fetch_pkt(pkt_t *pkt) {

	// get here because rx_pkt_ready is true;
	// the RF12 driver has rf12_len bytes for us at rf12_data

	// If the packet length is bigger than our buffer We Have A Situation.
	// Clip to our max packet size, silently discarding the excess.
	//
	int length = rf12_len;
	if (length > RF_PACKET_SIZE) {
		// todo: log here
		length = RF_PACKET_SIZE;
	}
	rx_packet_count++;						// got one? count it

	// copy the data from the driver's buffer to ours
	if (length) memcpy((void *) pkt, (const void *) rf12_data, length);

	if (RF12_WANTS_ACK) rf12_sendStart(RF12_ACK_REPLY, 0, 0);

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

	while (!rf12_canSend()) rf12_recvDone();
	rf12_sendStart(RF12_HDR_ACK, pkt, length);	// request ack
	rf12_sendWait(0);							// wait tx complete, no sleep (0)

	tx_packet_count++;
}


#endif	// defined(RADIO_RF12)