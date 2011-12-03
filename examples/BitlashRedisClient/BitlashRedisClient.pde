//////////////////////////////////////////////////////////////////
//
//	BitlashRedisClient.pde: Bitlash Redis Slave
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

/*****

This is a Bitlash-powered Ethernet Redis client for Arduino.

It works with the official Arduino Ethernet shield and compatible Arduino boards.

To get started:
	1. Adjust the IP address and port in the code to be suitable for your network
	2. Upload this sketch to your Arduino
		File -> Examples -> Bitlash -> BitlashRedisClient
		File -> Upload
	3. Setup a redis server
	
	4. From a PC command shell, test the redis server by setting a "message of the day"
		redis-cli set motd '{print "Hello from the redis server."}'
		redis-cli get motd

	5. From the Bitlash command line, try:
		print get("motd"):s
		printf("%s", get("motd"))

		or if you are brave:
		eval(get("motd"))
		
	3. For debugging, connect via your favorite Serial Monitor at 57600
		You can watch the web traffic and issue commands
	4. Navigate to the configured IP/port in your browser
		The default settings in the code below:
			http://192.168.1.27:8080
		If you see the "Bitlash web server here!" banner you've connected
		You'll see the hit logged on the serial monitor
	5. For remote maintenance, telnet to the same address/port
		and provide the password
		The default password is "open sesame"; please change it below.
	6. Read the rest and go make pages


*****/

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
	#define serialPrintByte(b) Serial.write(b)
	#define Client EthernetClient
	#define Server EthernetServer
#else
	#include "WProgram.h"
	#define serialPrintByte(b) Serial.print(b,BYTE)
#endif
#include "bitlash.h"
#include <SPI.h>
#include <Ethernet.h>

////////////////////////////////////////
//
//	Ethernet configuration
//	Adjust for local conditions
//
byte mac_addr[] = {'b','i','t','l','s','h'};
byte ip[]  		= {192, 168, 1, 27};
byte gateway[] 	= {192, 168, 1, 1};
byte subnet[] 	= {255, 255, 255, 0};

byte server_ip[]  = {192, 168, 1,  6};		// redis server IP
#define PORT 6379		// default redis port
Client client(server_ip, PORT);



//
////////////////////////////////////////

#define BANNER "Bitlash redis client here! v0.2\r\n"

// Command line buffer for alternate command input stream
byte ilen;
byte *iptr;
#define INPUT_BUFFER_LENGTH 80
byte ibuf[INPUT_BUFFER_LENGTH];

// forward declaration
void serialHandler(byte);

// verbose debugging output
#define DEBUG 0

/////////////////////////////////////////////

void serialHandler(byte b) {
#if DEBUG
	serialPrintByte(b);
#endif
	if (client && client.connected()) client.print((char) b);
}

void sendstring(char *ptr) {
	while (*ptr) serialHandler(*ptr++);
}


// parse positive integer from response, leave iptr at start of next line
numvar parse_response_number(void) {
	numvar retval = 0;
	while (isdigit(*iptr)) retval = (retval * 10) + (*iptr++ - '0');
	iptr += 2;		// skip cr and lf
	return retval;
}

byte iseol(char c) { return ((c == '\r') || (c == '\n') || (!c)); }

void skip_past_eol(void) {
	while (!iseol(*iptr)) iptr++;
	iptr += 2;
}


#define RESPONSE_TIMEOUT 5000L


//////////
//
// Parse a redis response and return:
//
// $	pointer to the string
// $-1	0	(NIL)
// :	the integer value
// -	-1
// +	0
// *	the last primitive value in the multi-bulk
//
numvar parse_response() {
numvar value;

	// parse response body
	switch (*iptr++) {

		case '$':	// string reply: 	$3\r\nfoo\r\n
			value = parse_response_number();	// get string length
			if (value == -1L) return 0L;		// handle $-1 (NIL)
			iptr[value] = 0;			// null-terminate string in place
			return (numvar) iptr;		// return pointer to string
		
		case ':':	// integer reply:	:1\r\n
			value = parse_response_number();
			return value;
			
		case '-':
			skip_past_eol();
			return -1L;

		case '+':
			skip_past_eol();
			return 0L;

		case '*':	// multi bulk:		*3\r\n(then three of below)
			// return the value of the last field in the multi bulk
			int i;
			value = parse_response_number();		// this is the number of fields
			for (i=value; i>0; i--) {
				value = parse_response();
				skip_past_eol();
			}
			return value;
	}
	return 0L;	// shouldn't happen
}



numvar process_response(void) {

	unsigned long start_time = millis();
	ilen = 0;
	iptr = ibuf;
	*iptr = 0;

	for (;;) {

		if ((millis() - start_time) >= RESPONSE_TIMEOUT) {
			Serial.println("Response timeout.");
			return -3L;			// todo: better server-goes-away handling
		}

		if (client.available()) {

#if DEBUG
			Serial.print("Response rtt: ");
			Serial.print(millis()-start_time);
			Serial.println("ms");
#endif

			while (client.available()) {	// pump in the packet
				if (ilen < INPUT_BUFFER_LENGTH-1) ibuf[ilen++] = client.read();
				else break;		// drop overtyping on the floor here
			}
			ibuf[ilen] = 0;
			break;		// go parse what we got
		}
	}

#if DEBUG || 1
	Serial.print("Response: ");
	Serial.print((char *) ibuf);
#endif

	// parse response body
	return parse_response();
}


// for asynchronous responses like pub/sub
void runServerResponseHandler(void) {

	if (!client.available()) return;

	// have a packet: collect it and parse it for return value
	ilen = 0;
	iptr = ibuf;
	*iptr = 0;

	while (client.available()) {	// pump in the packet
		if (ilen < INPUT_BUFFER_LENGTH-1) ibuf[ilen++] = client.read();
		else break;		// drop overtyping on the floor here
	}
	ibuf[ilen] = 0;

#if DEBUG || 1
	Serial.print("Background Response: ");
	Serial.print((char *) ibuf);
#endif

	// parse and eval response body in the background(!)
	char *cmdptr = (char *) parse_response();
	if (cmdptr) {

#if DEBUG || 1
		Serial.print("PubSub command: ");
		Serial.println(cmdptr);
#endif
		doCommand(cmdptr);
	}
}



numvar func_get(void) {
	if (!client.connected()) {
		if (!client.connect()) return -5L;
	}
	sendstring("get ");
	sendstring((char *) getarg(1));
	sendstring("\r\n");
	return process_response();
}

numvar func_set(void) {
	if (!client.connected()) {
		if (!client.connect()) return -5L;
	}
	sendstring("set ");
	sendstring((char *) getarg(1));
	sendstring(" ");
	sendstring((char *) getarg(2));
	sendstring("\r\n");
	return process_response();
}

numvar func_incr(void) {
	if (!client.connected()) {
		if (!client.connect()) return -5L;
	}
	sendstring("incr ");
	sendstring((char *) getarg(1));
	sendstring("\r\n");
	return process_response();
}

numvar func_subscribe(void) {
	if (!client.connected()) {
		if (!client.connect()) return -5L;
	}
	sendstring("subscribe ");
	sendstring((char *) getarg(1));
	sendstring("\r\n");
	return process_response();
}


numvar func_eval(void) {
	if (getarg(1)) return doCommand((char *) getarg(1));
	return 0L;
}

numvar func_malloc(void) { return (numvar) malloc(getarg(1)); }
numvar func_mfree(void)  { free((void *) getarg(1)); return 0L;}

void setup(void) {

	// Arduino Ethernet library setup
	Ethernet.begin(mac_addr, ip, gateway, subnet);

	addBitlashFunction("get", &func_get);
	addBitlashFunction("set", &func_set);
	addBitlashFunction("incr", &func_incr);
	addBitlashFunction("subscribe", &func_subscribe);
	
	addBitlashFunction("eval", &func_eval);
	addBitlashFunction("malloc", &func_malloc);
	addBitlashFunction("mfree", &func_mfree);

	Serial.print(BANNER);

	initBitlash(57600);	
}



void loop(void) {
	runBitlash();
	runServerResponseHandler();
}
