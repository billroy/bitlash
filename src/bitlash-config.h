/***
	bitlash-config.h - Build options

	Bitlash is a tiny language interpreter that provides a serial port shell environment
	for bit banging and hardware hacking.

	See the file README for documentation.

	Bitlash lives at: http://bitlash.net
	The author can be reached at: bill@bitlash.net

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

#ifndef _BITLASH_CONFIG_H
#define _BITLASH_CONFIG_H

// Uncomment this to set Arduino version if < 18:
//#define ARDUINO 15

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

// Enable SD card script-in-file support
//#define SDFILE


#if !defined(ARDUINO) && !defined(UNIX_BUILD)
#error "Building is only supported through Arduino and src/Makefile. If you have an Arduino version older than 018 which does not define the ARDUINO variable, manually set your Arduino version in src/bitlash-config.h"
#endif

#if defined(ARDUINO) && ARDUINO < 100
	#include "WProgram.h"
#else
	#include "Arduino.h"
	#define prog_char char PROGMEM
	#define prog_uchar char PROGMEM
#endif

///////////////////////////////////////////////////////
//
// Find build type
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

///////////////////////////////////////////////////////
//
// UNIX BUILD
//
// See README-UNIX.md for info about how to compile
//
#ifdef UNIX_BUILD
#define MINIMUM_FREE_RAM 200
#define NUMPINS 32
#undef SOFTWARE_SERIAL_TX
#define E2END 2047

#define DEFAULT_CONSOLE StdioStream
// No corresponding pin
#undef DEFAULT_OUTPIN

#endif // defined unix_build

////////////////////////////////////////////////////
//
// ARDUINO BUILD
// (This includes third-party boards like sanguino)
//
////////////////////////////////////////////////////
//
#if defined(ARDUINO) // this detects the Arduino build environment

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

// Arduino < 019 does not have the Stream class, so don't support
// passing a different Stream object to initBitlash
#if ARDUINO < 19
#define DEFAULT_CONSOLE_ONLY
#endif

#endif // defined(ARDUINO)


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

#endif // defined (644)


///////////////////////////////////////////////////////
//
// MEGA BUILD
//
// Note: These are speculative and untested.  Feedback welcome.
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

#endif // defined (1280)


///////////////////////////////////////////////////////
//
// Atmega64 BUILD
#if defined(__AVR_ATmega64__)
#define NUMPINS (53)
#endif


///////////////////////////////////////////////////////
//
// TINY BUILD OPTIONS
//
#if defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny84__)
#define TINY_BUILD 1
#undef MINIMUM_FREE_RAM
#define MINIMUM_FREE_RAM 20
#define NUMPINS 6
#undef SOFTWARE_SERIAL_TX
#endif // TINY_BUILD


///////////////////////////////////////////////////////
//
// AVROPENDOUS and TEENSY BUILD
//
#if defined(__AVR_AT90USB162__)

//#define AVROPENDOUS_BUILD
#if defined(AVROPENDOUS_BUILD)
#define MINIMUM_FREE_RAM 20
#define NUMPINS 24
#undef SOFTWARE_SERIAL_TX
// Serial is virtual (USB), so no corresponding pin
#undef DEFAULT_OUTPIN
#endif // defined AVRO

#define TEENSY
#ifdef TEENSY
// No TEENSY options yet
#endif // defined TEENSY

#endif // defined '162


///////////////////////////////////////////////////////
//
// AVROPENDOUS 32U4 and TEENSY2 BUILD
//
#if defined(__AVR_ATmega32U4__)

//#define AVROPENDOUS_BUILD
#if defined(AVROPENDOUS_BUILD)
#define MINIMUM_FREE_RAM 50
#define NUMPINS 40
#undef SOFTWARE_SERIAL_TX
// Serial is virtual (USB), so no corresponding pin
#undef DEFAULT_OUTPIN
#endif // AVRO

#define TEENSY2
#if defined(TEENSY2)
// No TEENSY2 options yet
#endif // TEENSY2

#endif // defined '32U4


////////////////////
//
// ARM BUILD
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

/////////////////////////////////////////////
// External EEPROM (I2C)
//
// Uncomment to enable EEPROM via I2C
// Supports a Microchip 24xx32A EEPROM module attached to the I2C bus
// http://ww1.microchip.com/downloads/en/DeviceDoc/21713J.pdf
// Specifically, the DigiX has such a module onboard
// https://digistump.com/wiki/digix/tutorials/eeprom
//#define EEPROM_MICROCHIP_24XX32A

#define EEPROM_ADDRESS 0x50 // default EEPROM address for DigiX boards

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


// Enable USER_FUNCTIONS to include the add_bitlash_function() extension mechanism
// This costs about 256 bytes
//
#if !defined(TINY_BUILD)
#define USER_FUNCTIONS
#endif

// String value buffer size
#ifdef AVR_BUILD
  #define STRVALSIZE 120
#else
  #define STRVALSIZE 512
#endif
#define STRVALLEN (STRVALSIZE-1)
#define LBUFLEN STRVALSIZE

#endif // _BITLASH_CONFIG_H
