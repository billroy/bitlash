/***

	bitlash_rf.pde:	Wireless Bitlash
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

#if defined(RADIO_VIRTUALWIRE)
//	#include "VirtualWire.h"	// don't remove the leading tab
#endif

#if defined(RADIO_RF12)
#if defined(ARDUINO) && ARDUINO >= 100
	#include "JeeLib.h"
#else
	//#include "Ports.h"
	//#include "RF12.h"
#endif
#endif

//////////
//
//	tell command:
//		tell("addr" | "*", "command string", optional_arg1, optional_arg2,...);
//
// 		tell("node", "rooftemp(%d)\n",degf())
//		tell("*", "overtemp(%d)\n", degf());
//
numvar func_tell(void) {
	rf_set_tx_address((char *) getarg(1));
	setOutputHandler(&send_command_byte);	// engage the command forwarding logic
	func_printf_handler(2,3);				// format=arg(2), optional args start at 3
	send_command_byte('\n');				// terminate the command
	resetOutputHandler();					// restore the print handler
	pkt_flush();							// push the caboose
	return 0;
}


//////////
//
// rprintf(): broadcast printf over the radio link
//
numvar func_rprintf(void) {
	rf_set_tx_address(0);				// set up for broadcast tx address
	setOutputHandler(&send_serial);
	func_printf_handler(1,2);	// format=arg(1), optional args start at 2
	pkt_flush();
	resetOutputHandler();
	return 0;
}


//////////
//
//	setid("id"): set node id
//
numvar func_setid(void) {
	rf_set_rx_address((char *) getarg(1));
	return 0;
}


//////////
//
// rf controls
//
byte radio_go;



///////////////////////////////
//
//	initBitlashRF(): main initialization
//
//	call this once from setup(), after initBitlash()
//
void initBitlashRF(void) {

	init_radio();

	// RF shell startup banner
	sp("bitlash_rf 0.3 up!\r\n> ");
}


/////////////////////////////////////
// bitlash_rf main loop
//
// call this frequently from loop() else receive no packets!
//
void runBitlashRF(void) {
//spb('0');
	if (!radio_go) {
//spb('1');
		init_radio();
		if (!radio_go) return;
	}
//spb('2');
	while (rx_pkt_ready()) {
//spb('3');
		// fetch the packet
		byte payload_length = rx_fetch_pkt(&rx_buf);
		if (payload_length <= RF_PACKET_HEADER_SIZE) {
			rx_bogon_count++;
			continue;
		}
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
						speol();
						setOutputHandler(&send_serial);
						doCharacter(c);
						pkt_flush();
						resetOutputHandler();
						break;		// discard all after first EOL char (allows CR/LF)
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
// Bitlash_RF main
//
void setup(void) {

	// We need setid() to be available from startup, called in initBitlash()...
	// It's generally bad to add functions before initBitlash since errors can't be reported.
	// Just this one is safe...
	//
	addBitlashFunction("setid", (bitlash_function) func_setid);

	initBitlash(57600);		// must be first to initialize serial port
	initBitlashRF();		// must be after initBitlash()

	addBitlashFunction("tell", (bitlash_function) func_tell);
	addBitlashFunction("rprintf", (bitlash_function) func_rprintf);
	addBitlashFunction("rflog", (bitlash_function) func_rflog);
//	addBitlashFunction("rfstat", (bitlash_function) func_pktstat);
}


void loop(void) {
	runBitlashRF();
	runBitlash();
}
