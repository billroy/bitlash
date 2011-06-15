//////////////////////////////////////////////////////////////////
//
//	parfait.h:	Parfait - Bitlash Integration Definitions
//
//	Copyright (C) 2010-2011 by Bill Roy
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
#ifndef _PARFAIT_H
#define _PARFAIT_H

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif


// RF HARDWARE SETUP
//
// Radio selection: enable ONE of these
//
#define RADIO_RFM22
//#define RADIO_VIRTUALWIRE


// Arduino detector
//
#define ARDUINO_BUILD 1

#include <avr/io.h>
#include <avr/interrupt.h>
#include "pkt.h"

// Pain, you want?  Change below here you will, then.
//

// sigh
//typedef uint8_t byte;
#define sbi(a,b) (a|=(1<<b))
#define cbi(a,b) (a&=~(1<<b))


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
//	parfaitmain.c
//
extern byte radio_go;
void initParfait(void);
void runParfait(void);


#endif	// _PARFAIT_H

