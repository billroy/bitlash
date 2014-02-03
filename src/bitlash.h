/***
	bitlash.h

	Bitlash is a tiny language interpreter that provides a serial port shell environment
	for bit banging and hardware hacking.

	See the file README for documentation.

	Bitlash lives at: http://bitlash.net
	The author can be reached at: bill@bitlash.net

	Copyright (C) 2008-2013 Bill Roy

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
#ifndef _BITLASH_H
#define _BITLASH_H

#if defined(__x86_64__) || defined(__i386__)
#define UNIX_BUILD 1
#elif defined(__SAM3X8E__)
#define ARM_BUILD 1
#elif (defined(__MK20DX128__) || defined(__MK20DX256__)) && defined (CORE_TEENSY)
  // Teensy 3
  #define ARM_BUILD 2
#elif defined(PART_LM4F120H5QR) //support Energia.nu - Stellaris Launchpad / Tiva C Series 
#define ARM_BUILD  4 //support Energia.nu - Stellaris Launchpad / Tiva C Series  
#else
#define AVR_BUILD 1
#endif


#if defined(AVR_BUILD)
#include "avr/io.h"
#include "avr/pgmspace.h"
#include "avr/interrupt.h"
#endif

#if defined(AVR_BUILD) || defined(ARM_BUILD)
#include "string.h"
#include "ctype.h"
#include "setjmp.h"
#endif

// Unix includes
#if defined(UNIX_BUILD)
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "ctype.h"
#include "setjmp.h"
#include <time.h>
#include <sys/types.h>
#include <errno.h>
//#include <unistd.h>
#endif

#ifndef byte
#define byte uint8_t
#endif


////////////////////////////////////////////////////
// GLOBAL BUILD OPTIONS
////////////////////////////////////////////////////
//
// Enable LONG_ALIASES to make the parser recognize analogRead(x) as well as ar(x), and so on
// cost: ~200 bytes flash
//#define LONG_ALIASES 1

//
// Enable PARSER_TRACE to make ^T toggle a parser trace debug print stream
// cost: ~400 bytes flash
//#define PARSER_TRACE 1



////////////////////////////////////////////////////
//
//	ARDUINO BUILD OPTIONS
//
////////////////////////////////////////////////////
//
#if defined(HIGH) || defined(ARDUINO)		// this detects the Arduino build environment

#define ARDUINO_BUILD 1

//#define ARDUINO_VERSION 14	// working
//#define ARDUINO_VERSION 15 	// working
#define ARDUINO_VERSION 16		// working, released

// the serial support, she is changing all the time
#if ARDUINO_VERSION >= 15
#define beginSerial Serial.begin
#define serialAvailable Serial.available
#define serialRead Serial.read

#if defined(ARDUINO) && ARDUINO >= 100
	#define serialWrite Serial.write
#else
	#define serialWrite Serial.print
#endif

#endif

// Arduino version: 11 - enable by hand if needed; see bitlash-serial.h
//#define ARDUINO_VERSION 11


#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
	#define prog_char char PROGMEM
	#define prog_uchar char PROGMEM
#else
	#include "WProgram.h"
	#include "WConstants.h"
#endif


// Enable Software Serial tx support for Arduino
// this enables "setbaud(4, 4800); print #4:..."
// at a cost of about 400 bytes (for tx only)
//
#define SOFTWARE_SERIAL_TX 1
#define HARDWARE_SERIAL_TX 1

#define MINIMUM_FREE_RAM 50

#else
#define HIGH 1
#define LOW 0
#endif		// HIGH: arduino build


///////////////////////////////////////////////////////
//
// ARDUINO ETHERNET BUILD OPTIONS
//
///////////////////////////////////////////////////////
//
// Enable WIZ_ETHERNET true to build for telnet access to the official Arduino
// WIZ-5100 Ethernet shield
//#define WIZ_ETHERNET 1
//
// Enable AF_ETHERNET to build for telnet access to the Adafruit Ethernet shield 
// configured per the pinout below
//
//#define AF_ETHERNET 1
//

///////////////////////////////////////////////////////
//	WIZNET ETHERNET CONFIGURATION
//
#ifdef WIZ_ETHERNET

//
// You'll need these two lines in your sketch, as of Arduino-0022:
//
//	#include <SPI.h>
//	#include <Ethernet.h>
//

byte mac[] 		= { 'b','i','t','l','s','h' };
byte ip[]  		= { 192, 168, 1, 27 };
byte gateway[] 	= { 192, 168, 1, 1 };
byte subnet[] 	= {255,255,255,0};
#define PORT 8080
Server server = Server(PORT);

#define beginSerial beginEthernet
#define serialAvailable server.available
#define serialRead server.available().read
#define serialWrite server.write
void beginEthernet(unsigned long baud) {
	Ethernet.begin(mac, ip, gateway, subnet);
	server.begin();
}
#endif	// WIZ_ETHERNET


///////////////////////////////////////////////////////
// ADAFRUIT XPORT ETHERNET CONFIGURATION
//
#ifdef AF_ETHERNET
#define NET_TX 2
#define NET_RX 3
#define SOFTWARE_SERIAL_RX 1
#define RXPIN NET_RX
#undef HARDWARE_SERIAL_TX		// sorry, no room for pin 0/1 hard uart
#define DEFAULT_OUTPIN NET_TX
#define BAUD_OVERRIDE 9600
#endif	// AF_ETHERNET



///////////////////////////////////////////////////////
//
// SANGUINO BUILD
//
///////////////////////////////////////////////////////
//
// SANGUINO is auto-enabled to build for the Sanguino '644
// if the '644 define is present
//
#if defined(__AVR_ATmega644P__)
#define SANGUINO

//void beginSerial(unsigned long baud) { Serial.begin(baud); }
//char serialAvailable(void) { return Serial.available(); }
//char serialRead(void) { return Serial.read(); }
//void serialWrite(char c) { return Serial.print(c); }

#ifndef beginSerial
#define beginSerial Serial.begin
#define serialAvailable Serial.available
#define serialRead Serial.read
#define serialWrite Serial.print
#endif

// Sanguino has 24 digital and 8 analog io pins
#define NUMPINS (24+8)

// Sanguino primary serial tx output is on pin 9 (rx on 8)
// Sanguino alternate hardware serial port tx output is on pin 11 (rx on 10)
#define SANGUINO_DEFAULT_SERIAL 9
#define SANGUINO_ALTERNATE_SERIAL 11
#define DEFAULT_OUTPIN SANGUINO_DEFAULT_SERIAL
#define ALTERNATE_OUTPIN SANGUINO_ALTERNATE_SERIAL

#endif	// defined (644)


///////////////////////////////////////////////////////
//
// MEGA BUILD
//
//	Note: These are speculative and untested.  Feedback welcome.
//
///////////////////////////////////////////////////////
//
// MEGA is auto-enabled to build for the Arduino Mega or Mega2560
// if the '1280/'2560 define is present
//
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
#define MEGA 1
#endif

#if defined(MEGA)
#define beginSerial Serial.begin
#define serialAvailable Serial.available
#define serialRead Serial.read
#define serialWrite Serial.print

// MEGA has 54 digital and 16 analog pins
#define NUMPINS (54+16)

// Mega primary serial tx output is on pin 1 (rx on 0)
// Mega alternate hardware serial port tx output is on pin 18 (rx on 19)
// TODO: Support for hardware serial uart2 and uart3
//
#define MEGA_DEFAULT_SERIAL 1
#define MEGA_ALTERNATE_SERIAL 18
#define DEFAULT_OUTPIN MEGA_DEFAULT_SERIAL
#define ALTERNATE_OUTPIN MEGA_ALTERNATE_SERIAL

#endif	// defined (1280)

#if defined(__AVR_ATmega64__)

#define beginSerial Serial1.begin
#define serialAvailable Serial1.available
#define serialRead Serial1.read
#define serialWrite Serial1.print

#define NUMPINS (53)
#endif



///////////////////////////////////////////////////////
//
//	TINY BUILD OPTIONS
//
#if defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny84__)
#define TINY_BUILD 1
#undef MINIMUM_FREE_RAM
#define MINIMUM_FREE_RAM 20
#define NUMPINS 6
//#undef HARDWARE_SERIAL_TX
#undef SOFTWARE_SERIAL_TX
//#define SOFTWARE_SERIAL_TX 1

//#include "usbdrv.h"

#endif		// TINY_BUILD



///////////////////////////////////////////////////////
//
//	AVROPENDOUS and TEENSY BUILD OPTIONS
//
#if defined(__AVR_AT90USB162__)

//#define AVROPENDOUS_BUILD
#if defined(AVROPENDOUS_BUILD)
#define MINIMUM_FREE_RAM 20
#define NUMPINS 24
#undef HARDWARE_SERIAL_TX
#undef SOFTWARE_SERIAL_TX
void beginSerial(unsigned long baud) { ; }
#define serialAvailable usbAvailable
#define serialRead usbRead
#define serialWrite usbWrite
#include <util/delay.h>
#endif	// defined AVRO

#define TEENSY
#ifdef TEENSY
#endif	// defined TEENSY

#endif	// defined '162


///////////////////////////////////////////////////////
//
//	ATMega32U4 BUILD OPTIONS
//
#if defined(__AVR_ATmega32U4__)

//#define AVROPENDOUS_BUILD
#if defined(AVROPENDOUS_BUILD)
#define MINIMUM_FREE_RAM 50
#define NUMPINS 40
#undef HARDWARE_SERIAL_TX
#undef SOFTWARE_SERIAL_TX
void beginSerial(unsigned long baud) { ; }
#define serialAvailable usbAvailable
#define serialRead usbRead
#define serialWrite usbWrite
#include <util/delay.h>
#endif	// AVRO

#define TEENSY2
#if defined(TEENSY2)
#endif	// TEENSY2

#endif	// defined '32U4


///////////////////////////////////////////////////////
//
// SD CARD SUPPORT: Enable the SDFILE define for SD card script-in-file support
//
//#define SDFILE


///////////////////////////////////////////////////////
//
//	Unix build options
//	(not working)
//
//	> gcc bitlash.cpp -D UNIX_BUILD
//
#ifdef UNIX_BUILD
#define MINIMUM_FREE_RAM 200
#define NUMPINS 32
#undef HARDWARE_SERIAL_TX
#undef SOFTWARE_SERIAL_TX
#define beginSerial(x)

#define E2END 2047

#define uint8_t unsigned char
#define uint32_t unsigned long int
#define prog_char char
#define prog_uchar unsigned char
#define strncpy_P strncpy
#define strcmp_P strcmp
#define strlen_P strlen

#define PROGMEM
#define OUTPUT 1

#define pgm_read_byte(addr) (*(char*) (addr))
#define pgm_read_word(addr) (*(int *) (addr))

unsigned long millis(void);

#endif	// defined unix_build


////////////////////
//
//	ARM BUILD
#if defined(ARM_BUILD)
 #define prog_char char
#define prog_uchar byte
#define PROGMEM
#define pgm_read_byte(b) (*(char *)(b))
#define pgm_read_word(b) (*(int *)(b))
#define strncpy_P strncpy
#define strcmp_P strcmp
#define strlen_P strlen
#if ARM_BUILD==1
  #define E2END 4096
#else
  // Teensy 3
  #define E2END 2048
#endif

#endif


// numvar is 32 bits on Arduino and 16 bits elsewhere
#if (defined(ARDUINO_BUILD) || defined(UNIX_BUILD)) && !defined(TINY_BUILD)
typedef long int numvar;
typedef unsigned long int unumvar;
#else
typedef int numvar;
typedef unsigned int unumvar;
#endif		// arduino_build


#ifdef AVROPENDOUS_BUILD
// USB integration
uint8_t usbAvailable(void);
int usbRead(void);
void usbWrite(uint8_t);
void usbMouseOn(void);
void usbMouseOff(void);
void connectBitlash(void);
#endif	// avropendous


// Function prototypes


/////////////////////////////////////////////
// bitlash-api.c
//
void initBitlash(unsigned long baud);	// start up and set baud rate
void runBitlash(void);					// call this in loop(), frequently
numvar doCommand(char *);					// execute a command from your sketch
void doCharacter(char);					// pass an input character to the line editor

//void flash(unsigned int, int);


/////////////////////////////////////////////
// bitlash-arduino.c
//
//#ifndef ARDUINO_BUILD
#if 0
void digitalWrite(uint8_t, uint8_t);
int digitalRead(uint8_t);
int analogRead(uint8_t);
void pinMode(uint8_t, uint8_t);
unsigned long millis(void);
void delay(unsigned long);
void delayMicroseconds(unsigned int);
#define clockCyclesPerMicrosecond() ( F_CPU / 1000000L )
#define clockCyclesToMicroseconds(a) ( (a) / clockCyclesPerMicrosecond() )
#define INPUT 0
#define OUTPUT 1
#endif


/////////////////////////////////////////////
// bitlash-cmdline.c
//
#ifdef TINY_BUILD
byte putlbuf(char);
void initlbuf(void);
#endif

// String value buffer size
#ifdef AVR_BUILD
  #define STRVALSIZE 120
#else
  #define STRVALSIZE 512
#endif
#define STRVALLEN (STRVALSIZE-1)
#define LBUFLEN STRVALSIZE

extern char *lbufptr;
extern char lbuf[LBUFLEN];


/////////////////////////////////////////////
// bitlash-eeprom.c
//
int findKey(char *key);				// return location of macro keyname in EEPROM or -1
int getValue(char *key);			// return location of macro value in EEPROM or -1

int findoccupied(int);
int findend(int);
void eeputs(int);

#define EMPTY ((uint8_t)255)
#define STARTDB 0
#define FAIL ((int)-1)

/////////////////////////////////////////////
// External EEPROM (I2C)
//
//#define EEPROM_MICROCHIP_24XX32A	// Uncomment to enable EEPROM via I2C
									// Supports a Microchip 24xx32A EEPROM module attached to the I2C bus
									// http://ww1.microchip.com/downloads/en/DeviceDoc/21713J.pdf
									// Specifically, the DigiX has such a module onboard
									// https://digistump.com/wiki/digix/tutorials/eeprom

#define EEPROM_ADDRESS 0x50			// default EEPROM address for DigiX boards

////////////////////////
//
// EEPROM database begin/end offset
//
// Use the predefined constant from the avr-gcc support file
//
#if defined(EEPROM_MICROCHIP_24XX32A)
	#define ENDDB 4095
	#define ENDEEPROM 4095
#else
	#define ENDDB E2END
	#define ENDEEPROM E2END
#endif

/////////////////////////////////////////////
// bitlash-error.c
//
extern jmp_buf env;
#define X_EXIT 1
void fatal2(char, char);
void overflow(byte);
void underflow(byte);
void expected(byte);
void expectedchar(byte);
void unexpected(byte);
void missing(byte);
//void oops(int);						// fatal exit


/////////////////////////////////////////////
// bitlash-functions.c
//
typedef numvar (*bitlash_function)(void);
void show_user_functions(void);

void dofunctioncall(byte);
numvar func_free(void);
void make_beep(unumvar, unumvar, unumvar);

extern const prog_char functiondict[] PROGMEM;
extern const prog_char aliasdict[] PROGMEM;

void stir(byte);

/////////////////////////////////////////////
// bitlash-program.c
//
extern char startup[];


/////////////////////////////////////////////
// bitlash-serial.c
//
void printIntegerInBase(unumvar, uint8_t, numvar, byte);
void printInteger(numvar,numvar, byte);
void printHex(unumvar);
void printBinary(unumvar);
void spb(char c);
void sp(const char *);
void speol(void);

numvar func_printf(void); 
numvar func_printf_handler(byte,byte);

#ifdef SOFTWARE_SERIAL_TX
numvar setBaud(numvar, unumvar);
void resetOutput(void);
#endif

// serial override handling
#define SERIAL_OVERRIDE
#ifdef SERIAL_OVERRIDE
typedef void (*serialOutputFunc)(byte);
byte serialIsOverridden(void);
void setOutputHandler(serialOutputFunc);
void resetOutputHandler(void);
extern serialOutputFunc serial_override_handler;
#endif

#ifdef ARDUINO_BUILD
void chkbreak(void);
void cmd_print(void);
#endif
numvar func_printf_handler(byte, byte);


/////////////////////////////////////////////
// bitlash-taskmgr.c
//
void initTaskList(void);
void runBackgroundTasks(void);
void stopTask(byte);
void startTask(int, numvar);
void snooze(unumvar);
void showTaskList(void);
extern byte background;
extern byte curtask;
extern byte suspendBackground;


/////////////////////////////////////////////
// eeprom.c
// they must live off piste due to aggressive compiler inlining.
//
#if defined(AVR_BUILD)
void eewrite(int, byte) __attribute__((noinline));
byte eeread(int) __attribute__((noinline));

#elif defined(ARM_BUILD)
void eewrite(int, byte);
byte eeread(int);
extern char virtual_eeprom[];
void eeinit(void);
#endif


/////////////////////////////////////////////
// bitlash-interpreter.c
//
numvar getstatementlist(void);
void domacrocall(int);


/////////////////////////////////////////////
// bitlash-parser.c
//
void vinit(void);							// init the value stack
void vpush(numvar);							// push a numvar on the stack
numvar vpop(void);							// pop a numvar
extern byte vsptr;
#define vsempty() vsptr==0
extern numvar *arg;								// argument frame pointer
numvar getVar(uint8_t id);					// return value of bitlash variable.  id is [0..25] for [a..z]
void assignVar(uint8_t id, numvar value);	// assign value to variable.  id is [0..25] for [a..z]
numvar incVar(uint8_t id);					// increment variable.  id is [0..25] for [a..z]

// parse context types
#define SCRIPT_NONE		0
#define SCRIPT_RAM 		1
#define SCRIPT_PROGMEM 	2
#define SCRIPT_EEPROM 	3
#define SCRIPT_FILE		4

byte findscript(char *);
byte scriptfileexists(char *);
numvar execscript(byte, numvar, char *);
void callscriptfunction(byte, numvar);

typedef struct {
	numvar fetchptr;
	byte fetchtype;
} parsepoint;

void markparsepoint(parsepoint *);
void returntoparsepoint(parsepoint *, byte);
void primec(void);
void fetchc(void);
void getsym(void);
void traceback(void);

numvar func_fprintf(void);

const prog_char *getmsg(byte);
void parsestring(void (*)(char));
void msgp(byte);
void msgpl(byte);
numvar getnum(void);
void calleeprommacro(int);
void getexpression(void);
byte hexval(char);
byte is_end(void);
numvar getarg(numvar);
numvar isstring(void);
numvar getstringarg(numvar);
void releaseargblock(void);
void parsearglist(void);
extern const prog_char reservedwords[];



// Interpreter globals
extern byte fetchtype;		// current script type
extern numvar fetchptr;		// pointer to current char in input buffer
extern numvar symval;		// value of current numeric expression

#define USE_GPIORS defined(AVR_BUILD)

#ifndef GPIOR0 || GPIOR1
	#undef USE_GPIORS
#endif
#if (defined USE_GPIORS)
#define sym GPIOR0
#define inchar GPIOR1
#else
extern byte sym;			// current input symbol
extern byte inchar;		// Current parser character
#endif

#ifdef PARSER_TRACE
extern byte trace;
void tb(void);
#endif


// Expression result
extern byte exptype;				// type of expression: s_nval [or s_sval]
extern numvar expval;				// value of numeric expr or length of string

// Temporary buffer for ids
#define IDLEN 12
extern char idbuf[IDLEN+1];


// Strings live in PROGMEM to save ram
//
#define M_expected		0
#define M_unexpected	1
#define M_missing		2
#define M_string		3
#define M_underflow		4
#define M_overflow		5
#define M_ctrlc			6
#define M_ctrlb			7
#define M_ctrlu			8
#define M_exp			9
#define M_op			10
#define M_pfmts			11
#define M_eof			12
#define M_var			13
#define M_number		14
#define M_rparen		15
#define M_saved			16
#define M_eeprom		17
#define M_defmacro		18
#define M_prompt		19
#define M_char			20
#define M_stack			21
#define M_startup		22
#define M_id			23
#define M_promptid		24
#define M_functions		25
#define M_oops			26
#define M_arg			27
#define M_function		28


//	Names for symbols
//
//	Each symbol in the grammar is parsed to a unique symval enumerated here
//
//	One character symbols take their ascii value as their symval
//	Complex symbols have the high bit set so start at 128 (0x80)
//
#define s_eof			0
#define s_undef			(0 | 0x80)
#define s_nval			(1 | 0x80)
#define s_sval			(2 | 0x80)
#define s_nvar			(3 | 0x80)
#define s_le			(4 | 0x80)
#define s_ge			(5 | 0x80)
#define s_logicaland	(6 | 0x80)
#define s_logicalor		(7 | 0x80)
#define s_logicaleq		(8 | 0x80)
#define s_logicalne		(9 | 0x80)
#define s_shiftleft		(10 | 0x80)
#define s_shiftright	(11 | 0x80)
#define s_incr			(12 | 0x80)
#define s_decr			(13 | 0x80)
#define s_nfunct		(14 | 0x80)
#define s_if			(15 | 0x80)
#define s_while			(16 | 0x80)
#define s_apin			(17 | 0x80)
#define s_dpin			(18 | 0x80)
#define s_define		(19 | 0x80)
#define s_function		(20 | 0x80)
#define s_rm			(21 | 0x80)
#define s_run			(22 | 0x80)
#define s_ps			(23 | 0x80)
#define s_stop			(24 | 0x80)
#define s_boot			(25 | 0x80)
#define s_peep			(26 | 0x80)
#define s_help			(27 | 0x80)
#define s_ls			(28 | 0x80)
#define s_print			(29 | 0x80)
#define s_switch		(30 | 0x80)
#define s_return		(31 | 0x80)
#define s_returning		(32 | 0x80)
#define s_arg			(33 | 0x80)
#define s_else			(34 | 0x80)
#define s_script_eeprom	(35 | 0x80)
#define s_script_progmem (36 | 0x80)
#define s_script_file	(37 | 0x80)
#define s_comment		(38 | 0x80)


// Names for literal symbols: these one-character symbols 
// are represented by their 7-bit ascii char code
#define s_semi			';'
#define s_add			'+'
#define s_sub			'-'
#define s_mul			'*'
#define s_div			'/'
#define s_mod			'%'
#define s_lparen		'('
#define s_rparen		')'
#define s_dot			'.'
#define s_lt			'<'
#define s_gt			'>'
#define s_equals		'='
#define s_bitand		'&'
#define s_bitor			'|'
#define s_comma			','
#define s_bitnot		'~'
#define s_logicalnot	'!'
#define s_xor			'^'
#define s_colon			':'
#define s_pound			'#'
#define s_quote			'"'
#define s_dollars		'$'
#define s_lcurly		'{'
#define s_rcurly		'}'


#endif	// defined _BITLASH_H

