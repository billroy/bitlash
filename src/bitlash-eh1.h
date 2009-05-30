/***
	bitlash-eh1.h

	Bitlash is a tiny language interpreter that provides a serial port shell environment
	for bit banging and hardware hacking.

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
#ifndef _BITLASH_EH1_H
#define _BITLASH_EH1_H

// This define enables the bit definitions and interrupt configuration in usbdrv.h
#define EH1

// EH-1 IO port assignments
#define BIT_LED 1
#define BIT_LED2 4
#define BIT_KEY 3

// courtesy lights
#define LEDS_ON()	PORTB |= (1 << BIT_LED) | (1 << BIT_LED2);		// lights on
#define LEDS_OFF() 	PORTB &= ~((1 << BIT_LED) | (1 << BIT_LED2));	// lights off

// report_t for the hid_mouse demo
typedef struct{
    uint8_t   buttonMask;
    char    dx;
    char    dy;
    char    dWheel;
} report_t;
extern report_t reportBuffer;

extern byte buttons;

#endif
