//////////////////////////////////////////////////////////////////
//
//	parfait.pde:	Bitlash Integration for the Parfait RFM22
//					Wireless Shield
//
//	Copyright 2010-2011 by Bill Roy
//
//	Permission is hereby granted, free of charge, to any person
//	obtaining a copy of this software and associated documentation
//	files (the "Software"), to deal in the Software without
//	restriction, including without limitation the rights to use,
//	copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following
//	conditions:
//	
//	The above copyright notice and this permission notice shall be
//	included in all copies or substantial portions of the Software.
//	
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
//	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
//	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
//	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
//	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
//	OTHER DEALINGS IN THE SOFTWARE.
//
//
//////////////////////////////////////////////////////////////////
//
#include "WProgram.h"
#include "bitlash.h"
#include "../../libraries/bitlash/src/bitlash.h"
#include "parfait.h"

//#include "../pkt.ccc"
//#include "../radio_rfm22.ccc"

//////////
//
// tell 1:<address> 2:<cmd> 3..N:<optional args>
//
#define REG_TX_ADDR 0x3a

numvar cmd_tell(void) {
	rf_put_address(REG_TX_ADDR, (byte *) getarg(1));		// todo: verify this handles broadcast right
	setOutputHandler(&send_command_byte);	// engage the command forwarding logic
	func_printf_handler(2,3);				// format=arg(2), optional args start at 3
	pkt_flush();							// push the caboose
	resetOutputHandler();					// restore the print handler
	rf_init_tx_address();					// and broadcast tx address
	return 0;
}


//////////
//
// rprintf(): broadcast printf over the radio link
//
numvar func_rprintf(void) {
	setOutputHandler(&send_serial);
	func_printf_handler(1,2);	// format=arg(1), optional args start at 2
	pkt_flush();
	resetOutputHandler();
	return 0;
}


//////////
//
// rf controls
//
byte radio_go;



///////////////////////////////
//
//	initParfait(): main initialization
//
//	call this once from setup(), after initBitlash()
//
void initParfait(void) {

	init_leds();
	init_radio();

	// RF shell startup banner
	sp("parfait 0.2 up!\r\n> ");
}


/////////////////////////////////////
// parfait main loop
//
// call this frequently from loop() else receive no packets!
//
void runParfait(void) {

	if (!radio_go) {
		init_radio();
		if (!radio_go) return;
	}

	for (;;) {

		if (!rf_interrupt()) break;		// bail if no action from radio city

		// fetch the packet
		byte payload_length = rx_check_pkt(&rx_buf);
		if (payload_length <= RF_PACKET_HEADER_SIZE) break;	// fetch packet if ready else punt
		payload_length -= RF_PACKET_HEADER_SIZE;	// calculate size of payload

		char c;
		int i = 0;
		switch (rx_buf.type) {

			case PKT_SERIAL:
				while (i < payload_length) spb(rx_buf.data[i++]);
				break;

			case PKT_COMMAND:
				while (i < payload_length) {
					c = rx_buf.data[i++];

					// forward serial output to the network while the command executes;
					// thus tell moe "ls" sends the ls output to the sending node's console
					if ((c == '\r') || (c == '\n') || (c == '`')) {
						setOutputHandler(&send_serial);
						doCharacter(c);
						pkt_flush();
						resetOutputHandler();
					}
					else doCharacter(c);	// push in the commands
				}
				break;

			// TODO: call out to user-registered packet handler
			default: 
				rx_bogon_count++;
		}
	}
}





//////////
//
// Parfait + Bitlash main
//
void setup(void) {
	initBitlash(57600);		// must be first to initialize serial port
	initParfait();			// must be after initBitlash()

	addBitlashFunction("tell", (bitlash_function) cmd_tell);
	addBitlashFunction("rfget", (bitlash_function) func_rfget);
	addBitlashFunction("rfset", (bitlash_function) func_rfset);
	addBitlashFunction("rprintf", (bitlash_function) func_rprintf);
	addBitlashFunction("degf", (bitlash_function) func_degf);

}

void loop(void) {
	runParfait();
	runBitlash();
}
