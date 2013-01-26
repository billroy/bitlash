/***

	EthernetComander.ino: Bitlash Commander client for Arduino Ethernet shield

	Copyright (C) 2013 Bill Roy

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

/*****

This is a client for Bitlash Commander that uses HTTP over ethernet for command and control.

It works with the official Arduino Ethernet shield and compatible Arduino boards.

To get started:
	1. Adjust the IP address and port in the code to be suitable for your network
	2. Upload this sketch to your Arduino
		File -> Examples -> Bitlash -> ethernetcommander
		File -> Upload
	3. For debugging, connect via your favorite Serial Monitor at 57600
		You can watch the web traffic and issue commands
	4. Navigate to the configured IP/port in your browser
		The default settings in the code below:
			http://192.168.0.27:8080
		You'll see the hit logged on the serial monitor

*****/
#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
	#define serialPrintByte(b) Serial.write(b)
	#include <Ethernet.h>
	#include <EthernetClient.h>
	#include <EthernetServer.h>
	#include <util.h>
#else
	#include "WProgram.h"
	#define serialPrintByte(b) Serial.print(b,BYTE)
	#include <Ethernet.h>
#endif
#include "bitlash.h"
#include <SPI.h>

// Command line buffer for web parsing and alternate command input stream
byte ilen;
#define INPUT_BUFFER_LENGTH 80
byte ibuf[INPUT_BUFFER_LENGTH];


////////////////////////////////////////
//
//	Ethernet configuration
//	Adjust for local conditions
//
byte mac_addr[] = {'b','i','t','l','s','h'};
byte ip[]  		= {192, 168, 0, 27};
byte gateway[] 	= {192, 168, 0, 1};
byte subnet[] 	= {255, 255, 255, 0};

#ifndef PORT
#define PORT 8080		// Arduino Ethernet library supports values other than 80 for PORT
#endif

#define debug 0
#define echo  0

//
////////////////////////////////////////

#define HTTP_200_OK "HTTP/1.1 200 OK\r\n"
#define CONTENT_TYPE "Content-Type: text/plain\r\n\r\n"

extern void prompt(void);

#if defined(ARDUINO) && ARDUINO >= 100
EthernetServer server = EthernetServer(PORT);
EthernetClient client;
#else
Server server = Server(PORT);
Client client(MAX_SOCK_NUM);		// declare an inactive client
#endif

void serialHandler(byte b) {
	if (echo) serialPrintByte(b);
	if (client && client.connected()) client.print((char) b);
}

void sendstring(char *ptr) {
	while (*ptr) serialHandler(*ptr++);
}

byte isPOST(byte *line) {
	return !strncmp((char *)line, "POST /", 6);
}

//////////
//
// Get a line of input from the network connection
//
void getLine(void) {
	ilen = 0;
	while (client.connected()) {
		while (client.available()) {
			char c = client.read();
			if (c == '\r') {;}
			else if (c == '\n') {
				ibuf[ilen] = '\0';
				if (debug) {
					Serial.print("NET: "); 
					Serial.print(ilen); 
					Serial.print(" "); 
					Serial.println((char *)ibuf);
				}
				return;
			}
			else if (ilen < INPUT_BUFFER_LENGTH-1) ibuf[ilen++] = c;
			else {;}	// drop overtyping on the floor here
		}
		//runBitlash();
	}
	if (debug) {
		Serial.print("NET END: "); 
		Serial.println((char *)ibuf);
	}
	ibuf[ilen] = '\0';
	return;
}


//////////
//
// Check for a web request
//
void runWebserver(void) {
	client = server.available();
	if (client) {
		getLine();							// get the POST command
		while (ilen > 0) getLine();			// blow off headers until we get a blank line			
		getLine();							// get the first line of the body (our command)

		sendstring(HTTP_200_OK);			// send reply OK
		sendstring(CONTENT_TYPE);			// and headers
		setOutputHandler(&serialHandler);	// redirect Bitlash output to ethernet
		doCommand((char *) ibuf);			// ask Bitlash to execute the command
		resetOutputHandler();				// restore the print handler
		client.stop();						// and end the connection
	}
}

void setup(void) {
	initBitlash(57600);
	Ethernet.begin(mac_addr, ip, gateway, subnet);
	server.begin();
}

void loop(void) {
	runWebserver();
	runBitlash();
}
