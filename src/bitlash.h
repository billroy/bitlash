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

// Uncomment this to set Arduino version if < 18:
//#define ARDUINO 15

#if !defined(ARDUINO) && !defined(UNIX_BUILD)
#error "Building is only supported through Arduino and src/Makefile. If you have an Arduino version older than 018 which does not define the ARDUINO variable, manually set your Arduino version in src/bitlash.h"
#endif

#if defined(ARDUINO)
#if ARDUINO < 100
	#include "WProgram.h"
#else
	#include "Arduino.h"
	#define prog_char char PROGMEM
	#define prog_uchar char PROGMEM
#endif
#endif // defined(ARDUINO)

#if !defined(UNIX_BUILD)
#if defined(__SAM3X8E__)
#define ARM_BUILD 1
#elif (defined(__MK20DX128__) || defined(__MK20DX256__)) && defined (CORE_TEENSY)
  // Teensy 3
  #define ARM_BUILD 2
#elif defined(PART_LM4F120H5QR) //support Energia.nu - Stellaris Launchpad / Tiva C Series 
#define ARM_BUILD  4 //support Energia.nu - Stellaris Launchpad / Tiva C Series  
#else
#define AVR_BUILD 1
#endif
#endif // !defined(UNIX_BUILD)


#if defined(AVR_BUILD)
#include "avr/pgmspace.h"
#endif

#include <string.h>
#include <ctype.h>
#include <setjmp.h>

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

// Define this to disable the initBitlash(Stream*) function and have the
// console I/O fixed to DEFAULT_CONSOLE (saves program memory)
//#define DEFAULT_CONSOLE_ONLY


////////////////////////////////////////////////////
//
//	ARDUINO BUILD OPTIONS
//
////////////////////////////////////////////////////
//
#if defined(ARDUINO)		// this detects the Arduino build environment

#ifdef SERIAL_PORT_MONITOR
#define DEFAULT_CONSOLE SERIAL_PORT_MONITOR
#else
// Support 1.0.5 and below and 1.5.4 and below, that don't have
// SERIAL_PORT_MONITOR defined
#define DEFAULT_CONSOLE Serial
#endif

// Assume DEFAULT_CONSOLE lives at pin 0 (Arduino headers don't have
// any way of finding out). Might be undef'd or redefined below.
#define DEFAULT_OUTPIN 0

// Enable Software Serial tx support for Arduino
// this enables "setbaud(4, 4800); print #4:..."
// at a cost of about 400 bytes (for tx only)
//
#define SOFTWARE_SERIAL_TX 1

#define MINIMUM_FREE_RAM 50

#else
#define HIGH 1
#define LOW 0
#endif // defined(ARDUINO)

// Arduino < 019 does not have the Stream class, so don't support
// passing a different Stream object to initBitlash
#if defined(ARDUINO) && ARDUINO < 19
#define DEFAULT_CONSOLE_ONLY
#endif

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

// Sanguino has 24 digital and 8 analog io pins
#define NUMPINS (24+8)

// Sanguino primary serial tx output is on pin 9 (rx on 8)
// Sanguino alternate hardware serial port tx output is on pin 11 (rx on 10)
#define SANGUINO_DEFAULT_SERIAL 9
#define SANGUINO_ALTERNATE_SERIAL 11
#undef DEFAULT_OUTPIN
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

// MEGA has 54 digital and 16 analog pins
#define NUMPINS (54+16)

// Mega primary serial tx output is on pin 1 (rx on 0)
// Mega alternate hardware serial port tx output is on pin 18 (rx on 19)
// TODO: Support for hardware serial uart2 and uart3
//
#define MEGA_DEFAULT_SERIAL 1
#define MEGA_ALTERNATE_SERIAL 18
#undef DEFAULT_OUTPIN
#define DEFAULT_OUTPIN MEGA_DEFAULT_SERIAL
#define ALTERNATE_OUTPIN MEGA_ALTERNATE_SERIAL

#endif	// defined (1280)

#if defined(__AVR_ATmega64__)

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
#undef SOFTWARE_SERIAL_TX
#endif		// TINY_BUILD


// Enable USER_FUNCTIONS to include the add_bitlash_function() extension mechanism
// This costs about 256 bytes
//
#if !defined(TINY_BUILD)
#define USER_FUNCTIONS
#endif

///////////////////////////////////////////////////////
//
//	AVROPENDOUS and TEENSY BUILD OPTIONS
//
#if defined(__AVR_AT90USB162__)

//#define AVROPENDOUS_BUILD
#if defined(AVROPENDOUS_BUILD)
#define MINIMUM_FREE_RAM 20
#define NUMPINS 24
#undef SOFTWARE_SERIAL_TX
// Serial is virtual (USB), so no corresponding pin
#undef DEFAULT_OUTPIN
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
#undef SOFTWARE_SERIAL_TX
// Serial is virtual (USB), so no corresponding pin
#undef DEFAULT_OUTPIN
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
//	See README-UNIX.md for info about how to compile
//
#ifdef UNIX_BUILD
#define MINIMUM_FREE_RAM 200
#define NUMPINS 32
#undef SOFTWARE_SERIAL_TX
#define beginSerial(x)

#define E2END 2047

#define byte uint8_t
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
#if (defined(ARDUINO) || defined(UNIX_BUILD)) && !defined(TINY_BUILD)
typedef long int numvar;
typedef unsigned long int unumvar;
#else
typedef int numvar;
typedef unsigned int unumvar;
#endif		// arduino_build


// Function prototypes


/////////////////////////////////////////////
// bitlash-api.c
//
void initBitlash(unsigned long baud);	// start up and set baud rate
#ifndef DEFAULT_CONSOLE_ONLY
void initBitlash(Stream& stream);
#endif
void runBitlash(void);					// call this in loop(), frequently
numvar doCommand(const char *);					// execute a command from your sketch
void doCharacter(char);					// pass an input character to the line editor

/////////////////////////////////////////////
// bitlash-builtins.c
//
void displayBanner(void);
byte findbuiltin(const char *name);

/////////////////////////////////////////////
// bitlash-cmdline.c
//
void initlbuf(void);
void pointToError(void);
void cmd_help(void);

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
int findKey(const char *key);				// return location of macro keyname in EEPROM or -1
int getValue(const char *key);			// return location of macro value in EEPROM or -1

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
void oops(int);						// fatal exit


/////////////////////////////////////////////
// bitlash-functions.c
//
typedef numvar (*bitlash_function)(void);
void show_user_functions(void);
char find_user_function(const char *id);
void addBitlashFunction(const char *name, bitlash_function func_ptr);

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
void cmd_print(void);

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
void setOutputHandler(Print&);
void resetOutputHandler(void);
#endif

#ifdef ARDUINO
void chkbreak(void);
void cmd_print(void);
#endif
numvar func_printf_handler(byte, byte);


// The Stream where input is read from and print writes to when there is
// not output handler set.
#ifndef DEFAULT_CONSOLE_ONLY
extern Stream *blconsole;
#else
// Console is fixed to DEFAULT_CONSOLE, so make blconsole a unchangeable
// pointer to DEFAULT_CONSOLE. By also copying the type of
// DEFAULT_CONSOLE, the compiler can optimize away all vtable operations
// for minimal code overhead
__typeof__(DEFAULT_CONSOLE) * const blconsole = &DEFAULT_CONSOLE;
#endif

// The Print object where the print command normally goes (e.g. when not
// redirected with print #10: "foo")
#ifdef SERIAL_OVERRIDE
extern Print *bloutdefault;
#else
// SERIAL_OVERRIDE is disabled, so print redirection should always just
// reset blout to blconsole. Using this define, we essentially make both
// identifiers refer to the same variable.
#define bloutdefault blconsole
#endif

// The Print object where the print command goes right now
#ifdef SOFTWARE_SERIAL_TX
extern Print *blout;
#else
// SOFTWARE_SERIAL_TX is disabled, so printing should always just go to
// bloutdefault.  Using this define, we essentiallyl make both
// identifiers refer to the same variable.
#define blout bloutdefault
#endif

// Note that if SOFTWARE_SERIAL_TX and SERIAL_OVERRIDE are disabled,
// then blconsole, bloutdefault and blout are all effectively the same
// variable.

/////////////////////////////////////////////
// bitlash-taskmgr.c
//
void initTaskList(void);
void runBackgroundTasks(void);
void stopTask(byte);
void startTask(int, numvar);
void snooze(unumvar);
void showTaskList(void);
unsigned long millisUntilNextTask(void);
extern byte background;
extern byte curtask;
extern byte suspendBackground;


/////////////////////////////////////////////
// eeprom.c
//
void eewrite(int, byte);
byte eeread(int);
void eraseentry(const char *id);
void cmd_function(void);
void cmd_ls(void);
void cmd_peep(void);

#if defined(AVR_BUILD)
// they must live off piste due to aggressive compiler inlining.
void eewrite(int, byte) __attribute__((noinline));
byte eeread(int) __attribute__((noinline));
#endif

#if defined(ARM_BUILD)
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

byte findscript(const char *);
byte scriptfileexists(const char *);
numvar execscript(byte, numvar, const char *);
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

#if defined(AVR_BUILD) && defined(GPIOR0) && defined(GPIOR1)
#define USE_GPIORS
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

/////////////////////////////////////////////
// bitlash-instream.c
//

#if defined(SDFILE) || defined(UNIX_BUILD)
numvar sdwrite(const char *filename, const char *contents, byte append);
#endif

/////////////////////////////////////////////
// bitlash-unix-file.c
//
#if defined(UNIX_BUILD)
numvar exec(void);
numvar sdls(void);
numvar sdexists(void);
numvar sdrm(void);
numvar sdcreate(void);
numvar sdappend(void);
numvar sdcat(void);
numvar sdcd(void);
numvar sdmd(void);
numvar func_pwd(void);
#endif

/////////////////////////////////////////////
// bitlash-unix.c
//
#if defined(UNIX_BUILD)
int serialAvailable(void);
int serialRead(void);
void digitalWrite(uint8_t, uint8_t);
int digitalRead(uint8_t);
int analogRead(uint8_t);
void analogWrite(byte, int);
void pinMode(uint8_t, uint8_t);
int pulseIn(int, int, int);
unsigned long millis(void);
void delay(unsigned long);
void delayMicroseconds(unsigned int);
#endif


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

