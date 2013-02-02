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

SECURITY NOTE: This implementation is not secure, nor can it be made so.  Any node
on the local network can make your Arduino do _anything_.  If you open a pinhole in
your router, anyone in the world can make your Arduino do _anything_.

I would feel bad if someone cooked your fish because you thought it would be cool
to have your aquarium online.  So please don't use this for anything life-critical.

To get started:
	1. Adjust the IP address and port in the code to be suitable for your network
	2. Set the server_name and server_port variables for the computer where
		you are running Bitlash Commander.  The port is usually 3000.
	2. Upload this sketch to your Arduino
		File -> Examples -> Bitlash -> ethernetcommander
		File -> Upload
	3. For debugging, connect via your favorite Serial Monitor at 57600
		You can watch the web traffic and issue commands
		Turn on echo and debug:
		> debug(1); echo(1);
	4. Start Bitlash Commander using the -a option and the client settings below:		
			$ node index -a http://192.168.0.27:8080
	5.	Navigate to the configured IP/port for Bitlash Commander in your browser
		(Same as server_name:server_port below.)
		Open the Commander panel and test a few buttons.  
		Observe the serial console for occasionally helpful logging information.

## Sending commands to Bitlash

Make an HTTP POST to the IP and port configured below.  

The first line of the POST body is presumed to be Bitlash script.

The script is executed and any printed output is returned as the response body.

The command must be less than 140 characters in length.


## Sending data upstream to Bitlash Commander

The "update" and "updatestr" commands are included with this sketch.

These functions format and print a JSON upstream command to Commander to update
the value of a control:

1. update("id", value)

Sends an update to Commander setting the control whose id is "id" to the numeric value given.
The value must be numeric or a numeric expression.

2. updatestr("id", "value")

Sends an update to Bitlash setting the control whose id is "id" to the string value given.
The value must be a string constant in quotes.


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
#define INPUT_BUFFER_LENGTH 140
byte ibuf[INPUT_BUFFER_LENGTH];


////////////////////////////////////////
//
//	Arduino Ethernet configuration
//	Adjust for local conditions
//
byte mac_addr[] = {'b','i','t','l','s','h'};
byte ip[]  		= {192, 168, 0, 27};
byte gateway[] 	= {192, 168, 0, 1};
byte subnet[] 	= {255, 255, 255, 0};
byte dnsserver[]= {0, 0, 0, 0};


#ifndef PORT
#define PORT 8080		// Arduino Ethernet library supports values other than 80 for PORT
#endif

// Bitlash Commander server configuration
//
char server_name[] = "192.168.0.2";
int server_port = 3000;

#define SERVER_COMMAND "POST /update/"
#define SERVER_COMMAND_TAIL " HTTP/1.0\n\n"

//
////////////////////


#if defined(ARDUINO) && ARDUINO >= 100
EthernetServer server = EthernetServer(PORT);
EthernetClient client;
#else
Server server = Server(PORT);
Client client(MAX_SOCK_NUM);		// declare an inactive client
#endif

int debug = 0;
int echo = 0;
numvar func_debug(void) { debug = getarg(1); }
numvar func_echo(void) { echo = getarg(1); }


////////////////////
//
//	Outbound POST interface (web client)
//
#include "../bitlash/src/bitlash.h"		// for printIntegerInBase


numvar sendUpdate(byte stringarg) {
	if (client.connect(server_name, server_port)) {
		client.print(SERVER_COMMAND);
		client.print((char *) getarg(1));
		client.print('/');
		if (stringarg) client.print((char *) getarg(2));
		else client.print(getarg(2));
		client.print(SERVER_COMMAND_TAIL);
		while (client.connected() && !client.available()) delay(1);
		while (client.connected() || client.available()) {
			char c = client.read();
			if (echo) Serial.print(c);
		}
		client.stop();
	}
	return 0;
}

numvar func_update() { return sendUpdate(0); }
numvar func_updatestr() { return sendUpdate(1); }


////////////////////
//
//	Inbound web server interface
//
#define HTTP_200_OK "HTTP/1.1 200 OK\r\n"
#define CONTENT_TYPE "Content-Type: text/plain\r\n\r\n"

extern void prompt(void);

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
	addBitlashFunction("update", (bitlash_function) func_update);
	addBitlashFunction("updatestr", (bitlash_function) func_updatestr);
	addBitlashFunction("debug", (bitlash_function) func_debug);
	addBitlashFunction("echo", (bitlash_function) func_echo);

#if defined(ARDUINO) && ARDUINO >= 100
	Ethernet.begin(mac_addr, ip, dnsserver, gateway, subnet);
#else
	Ethernet.begin(mac_addr, ip, gateway, subnet);
#endif

	server.begin();
}

void loop(void) {
	runWebserver();
	runBitlash();
}
