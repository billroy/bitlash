//////////////////////////////////////////////////////////////////
//
//	bitlashtelnet2.pde:	Bitlash Telnet Server for the Ethernet Shield
//
//	Copyright 2011 by Bill Roy
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
//////////////////////////////////////////////////////////////////
//
#include <SPI.h>
#include <Ethernet.h>
#include "bitlash.h"

////////////////////////////////////////
//
//	Ethernet configuration
//	Adjust for local conditions
//
byte mac[] 		= {'b','i','t','l','s','h'};
byte ip[]  		= {192, 168, 1, 27};
byte gateway[] 	= {192, 168, 1, 1};
byte subnet[] 	= {255, 255, 255, 0};
#define PORT 8080
//
////////////////////////////////////////

Server server = Server(PORT);
Client client(MAX_SOCK_NUM);		// declare an inactive client

void serialHandler(byte b) {
	Serial.print(b, BYTE);
	if (client && client.connected()) client.print((char) b);
}


void setup(void) {

	initBitlash(57600);
	Ethernet.begin(mac, ip, gateway, subnet);
	server.begin();

	setOutputHandler(&serialHandler);
}


void loop(void) {
	
	client = server.available();
	if (client) {
		while (client.connected()) {
			if (client.available()) {
				char c = (char) client.read();
				if (c != '\n') doCharacter(c);	// prevent double prompts
			}
			else runBitlash();
		}
	}
	runBitlash();
}
