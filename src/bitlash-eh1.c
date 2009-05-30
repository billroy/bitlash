/***
	bitlash-eh1.c

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
#include "bitlash.h"

#ifdef TINY85

void initStick(void) {
	DDRB = (unsigned char) (1 << BIT_LED) | (1 << BIT_LED2);   	// make the LED an output, all others inputs

	// example initialization for button on BIT_KEY
	//PORTB = (unsigned char) (1 << BIT_KEY);		// engage the pullup on the button

	// Millisecond timing: Enable Timer0 here
	// TODO: tune with a better constant and output compare interrupt
	TCCR0B = (1 << CS01) | (1 << CS00);	// Clock select: clk/64 from prescaler
	TIMSK = (1 << TOIE0);				// Enable overflow interrupt

	// set a2d prescale factor to 128
	// 16 MHz / 128 = 125 KHz, inside the desired 50-200 KHz range.
	// XXX: this will not work properly for other clock speeds, and
	// this code should use F_CPU to determine the prescale factor.
	//PRR &= ~(_BV(PRADC));									// ensure ADC is powered up
	PRR = 0;
	ADCSRA = (_BV(ADEN) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0));
	ADCSRB = 0;

	// Timer1 setup: We run timer1 at full steam and sample the low order bits
	// as an entropy source
	TCCR1 = 1<<CS10;		// PCK or CK / 1
}

// state of the buttons
byte buttons;

// move the mouse
void usbMouse(int8_t x, int8_t y, uint8_t buttons, int8_t dwheel) {

	//if (!USB_IsConnected) return;
	reportBuffer.dx = x;
	reportBuffer.dy = y;
	reportBuffer.buttonMask = buttons;
	reportBuffer.dWheel = dwheel;

	for (;;) {
		usbPoll();
		if (usbInterruptIsReady()) {
			usbSetInterrupt((void *)&reportBuffer, sizeof(reportBuffer));
			break;
		}
	}

#if 0
	for (;;) {
		usbPoll();
		if (usbInterruptIsReady()) {
			memset(&reportBuffer, 0, sizeof(reportBuffer));
			usbSetInterrupt((void *)&reportBuffer, sizeof(reportBuffer));
			break;
		}
	}
#endif

}


void usbKeystroke(uint8_t key) { ; }


#endif

