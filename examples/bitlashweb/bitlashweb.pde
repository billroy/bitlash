//////////////////////////////////////////////////////////////////
//
//	bitchi.pde: Bitlash Interactive Telnet Console and HTML Interactor
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

// Command line buffer for alternate command input stream
#define LBUFLEN 140
byte llen;
char linebuf[LBUFLEN];


////////////////////////////////////////
//
//	Ethernet configuration
//	Adjust for local conditions
//
byte mac[] 		= {'b','i','t','c','h','i'};
byte ip[]  		= {192, 168, 1, 27};
byte gateway[] 	= {192, 168, 1, 1};
byte subnet[] 	= {255, 255, 255, 0};
#define PORT 8080
#define PASSPHRASE "open sesame"
//
////////////////////////////////////////

#define INDEX_MACRO "_index"
#define ERROR_MACRO "_error"
#define BAD_PASSWORD_MAX 3

#define CONTENT_TYPE "Content-Type: text/plain\r\n\r\n"		// enable this for plaintext output
//#define CONTENT_TYPE "Content-Type: text/html\r\n\r\n"	// enable this for HTML output

/////////////////////////////////////////////
// Static HTML content and template engine
//
const prog_char *index_url PROGMEM = "index";
const prog_char *default_index_page PROGMEM = "Uptime: [print millis]ms";

const prog_char *bitlash_url PROGMEM = "bitlash";
const prog_char *bitlash_page PROGMEM = "http://bitlash.net";

typedef struct {
	const prog_char *url;
	const prog_char *pagetext;
} static_page;

static_page static_pages[] = {
	{index_url, default_index_page},
	{bitlash_url, bitlash_page},
	{0, 0}
};

int findStaticPage(char *pagename) {
int i=0;
int name;
	for (;;) {
		name = pgm_read_word(&static_pages[i].url);
		if (!name) break;
		if (!strcmp_P((const char *) pagename, (PGM_P) name)) return i;
		i++;
	}
	return(-1);
}


void sendStaticPage(char *pagename) {

	int pageindex = findStaticPage(pagename);
	if (pageindex < 0) return;

	PGM_P addr = (PGM_P) pgm_read_word(&static_pages[pageindex].pagetext);
	for (;;) {
		byte b = pgm_read_byte(addr++);
		if (b == '\0') break;
		else if (b == '[') {
			char *optr = linebuf;
			while ((b != ']') && ((optr-linebuf) < LBUFLEN)) {
				b = pgm_read_byte(addr++);
				if (b == ']') {
					*optr = '\0';
					doCommand(linebuf);
				}
				else *optr++ = b;
			}
		}
		else serialHandler(b);
	}
}


/////////////////////////////////////////////


extern void prompt(void);

Server server = Server(PORT);
Client client(MAX_SOCK_NUM);		// declare an inactive client

void serialHandler(byte b) {
	Serial.print(b, BYTE);
	if (client && client.connected()) client.print((char) b);
}

void sendstring(char *ptr) {
	while (*ptr) serialHandler(*ptr++);
}



byte unlocked;
byte badpasswordcount;

#define IDBUFLEN 13
char pagename[IDBUFLEN];


byte isGET(char *line) {
	return !strncmp(line, "GET /", 5);
}

byte isUnsupportedHTTP(char *line) {
	return !strncmp(line, "PUT /", 5) ||
		!strncmp(line, "POST /", 6) ||
		!strncmp(line, "HEAD /", 6);
}

void handleError(void) {
	sendstring("HTTP/1.1 404 OK \r\n\r\n");
	if (getValue(ERROR_MACRO) >= 0) doCommand(ERROR_MACRO);
	else sendstring("Page not found.\r\n");
}


void handleInputLine(char *line) {

	// check for web GET command: GET /macro HTTP/1.1
	if (isGET(line)) {
		char *iptr = line+5;	// point to first letter of macro name
		char *optr = pagename;
		*optr++ = '_';			// given "index" we search for macro "_index"
		while (*iptr && (*iptr != ' ') && ((optr - pagename) < (IDBUFLEN-1))) {
			*optr++ = *iptr++;
		}
		*optr = '\0';
		if (strlen(pagename) == 1) strcpy(pagename, INDEX_MACRO);	// map / to /index thus _index
		if (getValue(pagename) >= 0) {
			sendstring("HTTP/1.1 200 OK\r\n");
			sendstring(CONTENT_TYPE);			// configure this above
			doCommand(pagename);
		}
		else if (findStaticPage(pagename) >= 0) sendStaticPage(pagename);		
		else handleError();
		delay(1);
		client.stop();
	}
	else if (isUnsupportedHTTP(line)) client.stop();

	// not a web command: if we're locked, check for the passphrase
	else if (!unlocked) {
		if (!strcmp(line, PASSPHRASE)) {
			sendstring("Unlocked.\r\n");
			unlocked = 1;
			prompt();
		}
		else if ((strlen(line) > 0) && (++badpasswordcount > BAD_PASSWORD_MAX)) client.stop();
		else {
			if (badpasswordcount) delay(1000 * badpasswordcount);
			sendstring("Password: ");
		}
	}
	
	// unlocked, it's apparently a telnet command, execute it
	else {
		doCommand(line);
		prompt();
	}
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
		unlocked = 0;
		badpasswordcount = 0;
		while (client.connected()) {
			if (client.available()) {
				char c = client.read();
				if ((c == '\r') || (c == '\n')) {
					if (llen) {
						linebuf[llen] = '\0';
						handleInputLine(linebuf);
						llen = 0;
					}
				}
				else if (llen < LBUFLEN-1) linebuf[llen++] = c;
				else {;}	// drop overtyping on the floor here
			}
			else runBitlash();
		}
	}
	runBitlash();
}
