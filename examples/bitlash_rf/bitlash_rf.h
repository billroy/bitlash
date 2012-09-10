/***
	bitlash_rf.h:	Wireless Bitlash

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
#ifndef _BITLASH_RF_H
#define _BITLASH_RF_H

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif


// RF HARDWARE SETUP
//
// Radio selection: enable ONE of these groups
//
#define RADIO_RFM22

// NanodeRF with JeeLabs jeelib driver
//#define RADIO_RF12

#if defined(RADIO_RF12)
#if defined(ARDUINO) && ARDUINO >= 100
	#include "JeeLib.h"
#else
	//#include "Ports.h"
	//#include "RF12.h"
#endif
#endif

// VirtualWire
//#define RADIO_VIRTUALWIRE
	//#include "VirtualWire.h"

// Arduino detector
//
#define ARDUINO_BUILD 1

#include <avr/io.h>
#include <avr/interrupt.h>
#include "pkt.h"


///////////////////////////////
//
//	packet buffer
//
extern pkt_t tx_buf;		// the tx buffer
extern pkt_t rx_buf;		// the rx buffer

// packet counters
//
extern unsigned tx_packet_count;
extern unsigned rx_packet_count;
extern unsigned rx_bogon_count;


//////////////////////
// pkt.c
//
void send_serial(byte);
void send_command_byte(byte);
void pkt_flush(void);


//////////////////////
//	radio.c
//
///
// Radio API
//
void init_radio(void);
void rf_set_rx_address(char *);
void rf_set_tx_address(char *);
byte rx_pkt_ready(void);
byte rx_fetch_pkt(pkt_t *pkt);
void tx_send_pkt(pkt_t *, uint8_t);

// Radio Layer Functions
numvar func_rfget(void);
numvar func_rfset(void);
numvar func_setfreq(void);
numvar func_degf(void);
numvar func_rflog(void);
numvar func_rfstat(void);


extern uint8_t rf_address[5];


//////////////////////
//	bitlash_rf.cpp
//
extern byte radio_go;
void initBitlashRF(void);
void runBitlashRF(void);


#endif	// _BITLASH_RF_H

