//////////////////////////////////////////////////////////////////
//
//	parfait.h:	Parfait - Bitlash Integration Definitions
//
//	Copyright (C) 2010 Palmeta Productions LLC
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
// These pin assignments enable use of hardware SPI on the radio interface
//
#define L01_PORT		PORTB
#define L01_PORT_PIN	PINB
#define L01_PORT_DD		DDRB
#define L01_IRQ_PORT	PIND

#define L01_CSN	2 	// Output B2 is d10
#define L01_SCK	5 	// Output B5 is d13
#define MOSI	3 	// Output B3 is d11
#define MISO	4 	// Input  B4 is d12

#define L01_CE	0 	// Output B0 is d8
#define RF_IRQ	2 	// Input on D2 (PORTD2) which is d2

// radio primitives
//
#define rf_begin()		cbi(L01_PORT, L01_CSN)
#define rf_end()		sbi(L01_PORT, L01_CSN)

#define rf_disable()	sbi(L01_PORT, L01_CE)
#define rf_enable()		cbi(L01_PORT, L01_CE)

#define rf_interrupt()	(!(L01_IRQ_PORT & (1<<RF_IRQ)))


//////////////////////
//	led control: RF activity LED, by default pin 7
//
//	redefine LED_RF to move the activity light
//	undefine LED_RF to omit the activity light feature
// 	(saves 128 bytes!)
//
//#define LED_RF	7

#ifdef LED_RF
#define led_on() digitalWrite(LED_RF,1)
#define led_off() digitalWrite(LED_RF,0)
void init_leds(void) {
	pinMode(LED_RF, OUTPUT);
	led_on();
	delay(200);
	led_off();
}
#else
#define led_on()
#define led_off()
#define init_leds()
#endif


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
void init_radio(void);
byte rx_check_pkt(pkt_t *pkt);
void rx_get_pkt(uint8_t, uint8_t, pkt_t *);
void rx_read_pkt(pkt_t *);
uint8_t send_command(uint8_t cmd, uint8_t data);
void tx_send_payload(uint8_t, uint8_t, uint8_t *);
void tx_send_pkt(pkt_t *, uint8_t);
uint8_t spi_write(uint8_t outgoing);
void rf_get_register(uint8_t, uint8_t, uint8_t *);
byte rf_read_register(uint8_t);
void rf_set_register(uint8_t, uint8_t);
void rf_put_address(byte, byte *);
void rf_init_tx_address(void);
numvar func_rfget(void);
numvar func_rfset(void);
numvar func_degf(void);

#define REG_TX_ADDR 0x3a


extern uint8_t rf_address[5];


//////////////////////
//	parfaitmain.c
//
extern byte radio_go;
void initParfait(void);
void runParfait(void);


#endif	// _PARFAIT_H

