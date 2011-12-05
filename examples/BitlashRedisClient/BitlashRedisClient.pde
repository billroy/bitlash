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

The basic idea is that the redis server is a switchboard for Bitlash commands and script.

Each Arduino, and there can be thousands, can poll the server for instructions, 
or subscribe to live channels over which commands can be published by other nodes, 
from the command line, or via any automated redis client interface.

Redis is based on key-value pairs.  The "message of the day" example below uses
the key "motd", but it could be anything, perhaps "commands:hourly"; use what makes
sense in your application.  You associate a string value with a key using the
"set" command in redis, implemented in Bitlash as set(), so to set "motd":

	set("motd","print \"hello\""")

You can read back a key with get():
	
	get("motd")
	
Mostly, you'll want to execute the text as a command using eval():
	eval(get("motd"))
	
Or more generally:
	function do {eval(get(arg(1)))}
	do("motd")

And, to implement the hourly update check in Bitlash:
	function hourly_check {do("commands:hourly")}
	run hourly_check 60*60*1000


To get started:

	0. Learn about redis:
			http://simonwillison.net/static/2010/redis-tutorial/
			http://redis.io/commands

	1. Setup and start a redis server, perhaps on your laptop; see http://redis.io/download

		wget http://redis.googlecode.com/files/redis-2.4.4.tar.gz
		tar xzf redis-2.4.4.tar.gz
		cd redis-2.4.4
		make;make test; make install
		redis-server
		
	2. From a PC command shell, test the redis server by setting a "message of the day" command

		$ redis-cli set motd '{print "Hello from the redis server."}'
		$ redis-cli get motd

	3. Adjust the server IP address and port in the code below to point to your server

	4. Upload this sketch to your Arduino

		File -> Examples -> Bitlash -> BitlashRedisClient
		File -> Upload

	6. From the Bitlash command line, fetch and print the "motd" command you set in step 2
		print the string value:

			print get("motd"):s
			printf("%s", get("motd"))

		or if you are brave, evaluate it as a command:
			eval(get("motd"))

	7. Define these Bitlash functions to run the "motd" from the redis server automatically:
	
		function do {eval(get(arg(1)))};
		function startup {do("motd")};

	8. Run an hourly command check on the key "commands:hourly":
		function hourly {do("commands:hourly")}
		run hourly, 60*60*1000

		set a command to run hourly for all connected Bitlashes:
			from Bitlash: 
				set("commands:hourly","print \"Ding!\"")
			from a terminal:
				redis-cli set commands:hourly 'print "Ding!"'

	9. Subscribe to a pubsub channel
	
			subscribe("wx-alerts")

		then, perhaps from a PC:
		
			redis-cli publish wx-alerts 'print "Tornado warning in Wichita"'
		
		Note: subscribe puts the connection in a mode in which only 
			subscribe and unsubscribe commands are allowed.
		To restore normal command mode you must unsubscribe from each subscription.

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


////////////////////////////////////////
//
// CONFIGURE YOUR REDIS SERVER ADDRESS HERE
//
byte server_ip[]  = {192, 168, 1,  8};		// redis server IP
#define PORT 6379							// default redis port

Client client(server_ip, PORT);
////////////////////////////////////////

#define BANNER "Bitlash redis client here! v0.5\r\n"

// Command line buffer for alternate command input stream
byte ilen;
byte *iptr;
#define INPUT_BUFFER_LENGTH 80
byte ibuf[INPUT_BUFFER_LENGTH];

// forward declaration
void serialHandler(byte);

// verbose debugging output
#define DEBUG 1

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
signed long sgn = 1L;
	if (*iptr == '-') {
		sgn = -1L;
		iptr++;
	}
	unsigned long retval = 0;
	while (isdigit(*iptr)) retval = (retval * 10) + (*iptr++ - '0');
	iptr += 2;		// skip cr and lf
	return (numvar) (sgn * retval);
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
			if ((signed long) value == -1L) return 0L;	// handle NIL ($-1)
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

#if DEBUG
	Serial.print("Response: ");
	Serial.print((char *) ibuf);
#endif

	// parse response body
	return parse_response();
}


// for asynchronous responses from pub/sub
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

#if DEBUG
	Serial.print("Background Response: ");
	Serial.print((char *) ibuf);
#endif

	// parse and eval response body in the background(!)
	char *cmdptr = (char *) parse_response();
	if (cmdptr) {

#if DEBUG
		Serial.print("PubSub command: ");
		Serial.println(cmdptr);
#endif
		doCommand(cmdptr);
	}
}


numvar do_redis_command(char *cmd) {
	if (!client.connected()) {
		if (!client.connect()) return -5L;
	}
	sendstring(cmd);
	
	int nargs = getarg(0);
	int i = 1;
	while (i <= nargs) {
		sendstring(" ");
		sendstring((char *) getarg(i++));
	}
	sendstring("\r\n");
	return process_response();
}


numvar func_get(void) {
	return redis_command("get", 1);
}

numvar func_set(void) {
	return redis_command("set", 2);
}

numvar func_incr(void) {
	return redis_command("incr", 1);
}

numvar func_subscribe(void) {
	return redis_command("subscribe", 1);
}

numvar func_unsubscribe(void) {
	return redis_command("unsubscribe", 1);
}

numvar func_append(void) { return redis_command("append", 2); }


numvar func_redis(void) {
	setOutputHandler(serialHandler);
	numvar func_printf_handler(byte,byte);
	func_printf_handler(1,2);
	sendstring("\r\n");
	resetOutputHandler();
	return process_response();
}



//////////////////////////////
// Buffered serial output
//
#define OUTBUFLEN 80
byte *optr;
byte outbuf[OUTBUFLEN];

void stash_byte(byte b) {
	if (optr < &outbuf[OUTBUFLEN-1]) *optr++ = b;
}


//////////
//
// append(key, valueformatstring, v1,v2,...vN);
//
numvar func_append_prototype(void) {

	sendstring("*3\r\n$6\r\nappend\r\n$");		// 3 parts, command length, command

	// send length of key argument
	extern void printInteger(numvar n, numvar width, byte pad);
	setOutputHandler(serialHandler);
	printInteger((numvar) strlen((const char *) getarg(1)), 0, '0');
	resetOutputHandler();
	sendstring("\r\n");

	// send key argument
	sendstring((char *) getarg(1));
	sendstring("\r\n$");

	optr = outbuf;
	setOutputHandler(stash_byte);
	numvar func_printf_handler(byte,byte);
	func_printf_handler(2,3);				// print the data to our buffer
	resetOutputHandler();
	stash_byte(0);				// null-terminate
	outbuf[OUTBUFLEN-1] = 0;	// truncate for safety

	// send length of value argument
	setOutputHandler(serialHandler);
	printInteger((numvar) strlen((const char *) outbuf), 0, '0');
	resetOutputHandler();
	sendstring("\r\n");

	// send value argument
	sendstring((char *) outbuf);
	sendstring("\r\n");				// final eol to kick off the command

	return process_response();
}



//////////
//
// send_bulk_string
//
//	Sends the string at arg(formatarg), using args starting at optionalargs for printf expansion.
//
//
numvar send_bulk_string(byte formatarg, byte optionalargs) {

	optr = outbuf;
	setOutputHandler(stash_byte);
	numvar func_printf_handler(byte,byte);
	optionalargs = func_printf_handler(formatarg, optionalargs);	// print the data to our buffer
	resetOutputHandler();
	stash_byte(0);				// null-terminate
	outbuf[OUTBUFLEN-1] = 0;	// truncate for safety

	// send length of string: $<len>\r\n
	sendstring("$");
	setOutputHandler(serialHandler);
	extern void printInteger(numvar n, numvar width, byte pad);
	printInteger((numvar) strlen((const char *) outbuf), 0, '0');
	resetOutputHandler();
	sendstring("\r\n");

	// send string payload: <string>\r\n
	sendstring((char *) outbuf);
	sendstring("\r\n");				// eol after data per spec

	return optionalargs;
}


//////////
//
// redis_command(cmd)
//
//	expects:
//		arg(1)		key, 
//		arg(2)		valueformatstring, 
//		arg(3..n)	optional arguments 
//
//
//
numvar redis_command(char *cmd, byte argct) {
	if (!client.connected()) {
		if (!client.connect()) return -5L;
	}

	sendstring("*");		// begin multi-bulk
	
	setOutputHandler(serialHandler);
	extern void printInteger(numvar n, numvar width, byte pad);
	printInteger((numvar) argct+1, 0, '0');		// +1 for the command
	resetOutputHandler();
	sendstring("\r\n$");
	
	setOutputHandler(serialHandler);
	extern void printInteger(numvar n, numvar width, byte pad);
	printInteger((numvar) strlen((const char *) cmd), 0, '0');
	resetOutputHandler();
	sendstring("\r\n");

	sendstring(cmd);
	sendstring("\r\n");

	byte formatarg = 1;
	
	while (argct--) {
		formatarg = send_bulk_string(formatarg, formatarg+1);
	}

	sendstring("\r\n");
	return process_response();
}



numvar func_eval(void) {
	if (getarg(1)) return doCommand((char *) getarg(1));
	return 0L;
}
numvar func_malloc(void) { return (numvar) malloc(getarg(1)); }
numvar func_mfree(void)  { free((void *) getarg(1)); return 0L;}
numvar func_strcpy(void) { return (numvar) strcpy((char *) getarg(1), (char *) getarg(2)); }

void setup(void) {

	// Init Serial so we hear errors before it's re-inited in initBitlash
	Serial.begin(57600);

	// Arduino Ethernet library setup
	Ethernet.begin(mac_addr, ip, gateway, subnet);

	addBitlashFunction("get", &func_get);
	addBitlashFunction("set", &func_set);
	addBitlashFunction("incr", &func_incr);

	addBitlashFunction("subscribe", &func_subscribe);
	addBitlashFunction("unsubscribe", &func_unsubscribe);

	addBitlashFunction("append", &func_append);	
	addBitlashFunction("redis", &func_redis);
	
	addBitlashFunction("eval", &func_eval);
//	addBitlashFunction("malloc", &func_malloc);
//	addBitlashFunction("mfree", &func_mfree);
//	addBitlashFunction("strcpy", &func_strcpy);

	Serial.print(BANNER);

	initBitlash(57600);	
}



void loop(void) {
	runBitlash();
	runServerResponseHandler();
}
