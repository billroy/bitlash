//////////////////////////////////////////////////////////////////
//
//	BitlashWebServer.pde: Bitlash Interactive Telnet Console and HTML Interactor
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

This is a Bitlash-powered Ethernet web server and telnet console for Arduino.

It works with the official Arduino Ethernet shield and compatible Arduino boards.

To get started:
	1. Adjust the IP address and port in the code to be suitable for your network
	2. Upload this sketch to your Arduino
		File -> Examples -> Bitlash -> BitlashWebServer
		File -> Upload
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

There are two ways to define a web server page:

	1. You can compile in pages by modifying the builtin_page_list below
		Built-in pages can contain bitlash code in [ ]
		The server executes the code in [ ] while rendering the page output;
		any printed output the code generates will show up on the page.

		For example: "The uptime is: [print millis]ms" (see the "index" example below)

	2. Any Bitlash function whose name begins with the underscore character '_' 
		is considered a valid URL mount point and its output is sent as a response 
		when that URL is invoked.

		For example: the Bitlash function _uptime is called when the url /uptime is requested
			function _uptime {print "Uptime is ",millis,"ms";}

			From another terminal window we can test what the browser might see:
			$ curl http://192.168.1.27:8080/uptime
			Uptime is 123324 ms
			$

		For example: send the value of analog input 3 when ".../volt3" is requested:
			function _volt3 {print a3;}

Special pages _index and _error

	If you define a function named _index it will be rendered when the root url
		is requested.  Think of it as index.html

	If you define a function named _error it will be rendered when an error occurs,
		in place of the built-in error page

	Both of these uses depend on an additional feature: a page handler defined
		as a Bitlash function takes priority over a built-in page.
		You can use this to mask or override built-in pages.

Telnet access

(Not available on Nanode.)

The password-protected telnet console operating on the same IP/port as the web server
provides full access to Bitlash, so you can log in with telnet or nc and do bitlash stuff,
including of course defining a new _function to expose a new web page.

	...$ telnet 192.168.1.27 8080
	Trying 192.168.1.27...
	Connected to 192.168.1.27.
	Escape character is '^]'.
	open sesame
	Bitlash web server here! v0.4
	> 

The default telnet password is "open sesame"; please change it below or risk
becoming a security statistic.

Please note that the server is not smart enough to process web requests while a 
telnet session is open.  (Perhaps this is a feature.)  In any event, you will find 
it is necessary to quit your telnet connection to test your new page in the browser.

To log out, you may use the "logout" command, or the usual ^] followed by quit if you're
using old-school telnet.  If you're using nc, ^C will quit.


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

// Turn on the NANODE define to build for the Nanode enc28j60 ethernet interface
//
// Requires EtherShield library from https://github.com/thiseldo/EtherShield.git
//
//#define NANODE

#ifdef NANODE
#include <EtherShield.h>		// from https://github.com/thiseldo/EtherShield
#include <NanodeMAC.h>			// from https://github.com/thiseldo/NanodeMAC
EtherShield es=EtherShield();
#define TCP_OVERHEAD (TCP_CHECKSUM_L_P+3)
#define PACKET_BUFFER_LENGTH 400		// dhcp.c::dhcp_send() blindly thumps 400 bytes of buf :(
uint16_t pktlen;
byte pktbuf[PACKET_BUFFER_LENGTH + TCP_OVERHEAD];
#define PORT 80					// EtherShield library web server apparently requires PORT to be 80
#else
#include <SPI.h>
#include <Ethernet.h>
#endif


// Command line buffer for alternate command input stream
byte ilen;
#define INPUT_BUFFER_LENGTH 80
byte ibuf[INPUT_BUFFER_LENGTH];



////////////////////////////////////////
//
//	Ethernet configuration
//	Adjust for local conditions
//
byte mac_addr[] = {'b','i','t','l','s','h'};
byte ip[]  		= {192, 168, 1, 27};
byte gateway[] 	= {192, 168, 1, 1};
byte subnet[] 	= {255, 255, 255, 0};
byte dhcpip[]	= {192, 168, 1, 1};
byte dns_ip[] 	= {192, 168, 1, 1};

#ifndef PORT
#define PORT 8080		// Arduino Ethernet library supports values other than 80 for PORT
#endif

#define PASSPHRASE "open sesame"
#define BAD_PASSWORD_MAX 3

//
////////////////////////////////////////

#define BANNER "Bitlash web server here! v0.4\r\n"
#define INDEX_MACRO "_index"
#define ERROR_MACRO "_error"
#define ERROR_PAGE  "error"

#define CONTENT_TYPE_PLAIN "Content-Type: text/plain\r\n\r\n"	// enable this for plaintext output
#define CONTENT_TYPE_HTML  "Content-Type: text/html\r\n\r\n"	// enable this for HTML output
#define CONTENT_TYPE CONTENT_TYPE_PLAIN

#define HTTP_200_OK "HTTP/1.1 200 OK\r\n"
#define HTTP_404_NOTFOUND "HTTP/1.1 404 Not Found\r\n"

typedef struct {
	const prog_char *url;
	const prog_char *pagetext;
} builtin_page;


/////////////////////////////////////////////
//
// 	Built-in HTML content: Add your new built-in pages here:
//
//	Define a URL mount point and the text of your page like the ones already there
//	Add them to the builtin_pages table just like the ones already there
//
//	Each string needs \0 at the end.  
//	If you leave one off, you are guaranteed an interesting debugging experience
//
//	Lines will run together in text/plain mode without \r\n,
//	so mind your \r\n line endings.
//

// This block generates the built-in index page that is called on /
const prog_char index_url[] PROGMEM = { "index\0" };
const prog_char default_index_page[] PROGMEM = { 
	BANNER
	"Uptime: [print millis,]ms\r\n"
	"Powered by Bitlash.\r\n"
	"\0"	// must be last
};

// This block generates the built-in error page
const prog_char error_url[] PROGMEM = { "error\0" };
const prog_char error_page[] PROGMEM = { "Not found.\r\n\0" };

// This example generates a JSON response with the value read from a0, like this:
//	$ curl 192.168.1.27/analog0
//	{"a0":234}
//
// You could do the same with a function in EEPROM:
// $ function _analog0 {printf("{\"a0\":%d}\r\n",a0);}
//
const prog_char analog0_url[] PROGMEM = { "analog0\0" };
const prog_char analog0_page[] PROGMEM = { "{\"a0\":[print a0,]}\r\n\0" };


// Add new pages here, just like these
builtin_page builtin_page_list[] = {
	{ index_url, default_index_page },
	{ error_url, error_page },
	{ analog0_url, analog0_page },
	{ 0, 0 }	// marks end, must be last
};

//
//	End of Static HTML content
//
/////////////////////////////////////////////


int findStaticPage(char *pagename) {
int i=0;
const prog_char *name;
	for (;;) {
		name = builtin_page_list[i].url;
		if (!name) break;
		if (!strcmp_P((const char *) pagename, name)) return i;
		i++;
	}
	return(-1);
}

// forward declaration
void serialHandler(byte);

void sendStaticPage(char *pagename) {

	int pageindex = findStaticPage(pagename);
	if (pageindex < 0) return;

	const prog_char *addr = builtin_page_list[pageindex].pagetext;
	for (;;) {
		byte b = pgm_read_byte(addr++);
		if (b == '\0') break;
		else if (b == '[') {
			byte *optr = ibuf;
			while ((b != ']') && ((optr-ibuf) < INPUT_BUFFER_LENGTH-1)) {
				b = pgm_read_byte(addr++);
				if (b == ']') {
					*optr = '\0';
					doCommand((char *) ibuf);
				}
				else *optr++ = b;
			}
		}
		else serialHandler(b);
	}
}

//
/////////////////////////////////////////////


extern void prompt(void);
byte unlocked;
byte badpasswordcount;

#ifdef NANODE
void flushoutput(void) {
	if (pktlen) {
		es.ES_www_server_reply(pktbuf, pktlen); // send web page data
		pktlen = 0;
	}
}
#else
Server server = Server(PORT);
Client client(MAX_SOCK_NUM);		// declare an inactive client
#endif

numvar func_logout(void) {
#ifdef NANODE
	flushoutput();
#else
	client.stop();
#endif
	unlocked = 0;
}

void serialHandler(byte b) {
	serialPrintByte(b);

#ifdef NANODE
	// LIMIT CHECK: test this for correct behavior at overflow
	if (pktlen >= PACKET_BUFFER_LENGTH) flushoutput();
	pktlen = es.ES_fill_tcp_data_len(pktbuf, pktlen, (const char *) &b, 1);
	
#else
	if (client && client.connected()) client.print((char) b);
#endif
}

void sendstring(char *ptr) {
	while (*ptr) serialHandler(*ptr++);
}


#define IDBUFLEN 13
char pagename[IDBUFLEN];


byte isGET(byte *line) {
	return !strncmp((char *)line, "GET /", 5);
}

byte isUnsupportedHTTP(byte *line) {
	return !strncmp((char *)line, "PUT /", 5) ||
		!strncmp((char *)line, "POST /", 6) ||
		!strncmp((char *)line, "HEAD /", 6);
}


void servePage(char *pagename) {
	Serial.print("\r\nWeb request: ");
	Serial.print(&pagename[1]);
	Serial.print(" ");
	Serial.println(millis());

	if (getValue(pagename) >= 0) {		// _macro
		sendstring(HTTP_200_OK);
		sendstring(CONTENT_TYPE);			// configure this above
		doCommand(pagename);
	}
	else if (findStaticPage(&pagename[1]) >= 0) {	// static page
		sendstring(HTTP_200_OK);
		sendstring(CONTENT_TYPE);			// configure this above
		sendStaticPage(&pagename[1]);
	}
	else {
		sendstring(HTTP_404_NOTFOUND);
		sendstring(CONTENT_TYPE);
		if (getValue(ERROR_MACRO) >= 0) doCommand(ERROR_MACRO);
		else if (findStaticPage(ERROR_PAGE) >= 0) sendStaticPage(ERROR_PAGE);
		else sendstring("Error: not found.\r\n");	// shouldn't happen!
	}
	delay(1);
	func_logout();
}


void handleInputLine(byte *line) {

	Serial.print("Input line: [");
	Serial.print((char *) line);
	Serial.println("]");

	// check for web GET command: GET /macro HTTP/1.1
	if (isGET(line)) {
		byte *iptr = line+5;	// point to first letter of macro name
		byte *optr = (byte *) pagename;
		*optr++ = '_';			// given "index" we search for macro "_index"
		while (*iptr && (*iptr != ' ') && ((optr - (byte *) pagename) < (IDBUFLEN-1))) {
			*optr++ = *iptr++;
		}
		*optr = '\0';
		if (strlen(pagename) == 1) strcpy(pagename, INDEX_MACRO);	// map / to /index thus _index
		servePage(pagename);
	}


#ifdef NANODE
	// Unfortunately, the Telnet feature is not available on Nanode.
	// 
	// The Ethershield IP stack is currently (Sept 2011) only able to produce single-packet responses
	// This breaks the Telnet functionality, which depends on a persistent connection across
	// several request-response pairs.
	//
#else
	else if (isUnsupportedHTTP(line)) func_logout();

	// not a web command: if we're locked, check for the passphrase
	else if (!unlocked) {
		if (!strcmp((char *) line, PASSPHRASE)) {
			sendstring(BANNER);
			unlocked = 1;
			prompt();
		}
		else if ((strlen((char *) line) > 0) && (++badpasswordcount > BAD_PASSWORD_MAX)) func_logout();
		else {
			if (badpasswordcount) delay(1000);
			sendstring("Password: ");
		}
	}
#endif
	
	// unlocked, it's apparently a telnet command, execute it
	else {
		doCommand((char *)line);
		prompt();
	}
}


void setup(void) {

	initBitlash(57600);

#ifdef NANODE
	NanodeMAC mac(mac_addr);				// Fetch mac address from ID chip
	es.ES_enc28j60SpiInit();				// Initialise SPI interface
	es.ES_enc28j60Init(mac_addr, 8);		// initialize enc28j60 for NANODE pin 8 CS

//#define USE_DHCP
#ifdef USE_DHCP
	while (!es.allocateIPAddress(pktbuf, PACKET_BUFFER_LENGTH, 
								mac_addr, PORT, ip, subnet, gateway, dhcpip, dns_ip ) > 0 ) {

		sendstring("NO IP\r\n");	 // Can't get IP address
	}
#else
	es.ES_client_set_gwip(gateway);					// set the gateway
#endif
	es.ES_init_ip_arp_udp_tcp(mac_addr, ip, PORT);	// init the ethernet/ip layer

#else
	// Arduino Ethernet library setup
	Ethernet.begin(mac_addr, ip, gateway, subnet);
	server.begin();
#endif

	addBitlashFunction("logout", &func_logout);
	setOutputHandler(&serialHandler);
}

void loop(void) {
	
#ifdef NANODE
    // read packet into pktbuf, handle ping and wait for a tcp packet
    // (we also use pktbuf for output during the interpreter phase)
    ilen = es.ES_packetloop_icmp_tcp(pktbuf, es.ES_enc28j60PacketReceive(PACKET_BUFFER_LENGTH, pktbuf));
	if (ilen) {
		// data is at pktbuf[ilen], newline terminated
		// copy it to ibuf for the interpreter
		byte *iptr = pktbuf + ilen;
		byte *optr = ibuf;			// we use ibuf for our input line buffer
		while ((*iptr != '\r') && (*iptr != '\n')) *optr++ = *iptr++;
		*optr = '\0';

		handleInputLine(ibuf);
		flushoutput();
	}
	runBitlash();
	flushoutput();

#else
	client = server.available();
	if (client) {
		unlocked = 0;
		badpasswordcount = 0;
		while (client.connected()) {
			if (client.available()) {
				char c = client.read();
				if ((c == '\r') || (c == '\n')) {
					if (ilen) {
						ibuf[ilen] = '\0';
						handleInputLine(ibuf);
						ilen = 0;
					}
				}
				else if (ilen < INPUT_BUFFER_LENGTH-1) ibuf[ilen++] = c;
				else {;}	// drop overtyping on the floor here
			}
			else runBitlash();
		}
	}
	runBitlash();
#endif
}
