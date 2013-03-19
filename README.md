# Bitlash Release Notes

Questions / Bug Reports / Pull Requests welcome!  https://github.com/billroy/bitlash/issues

## March 19, 2013: Type checking for string arguments

Bitlash supports string constants in function argument lists, but until now there hasn't been a way to distinguish such string arguments from numeric arguments once you're inside the called function, making it easy to reference forbidden memory by mistake.

The new isstr() Bitlash function helps handle this problem by letting you check the type of a specified argument to the function it's called in:

	> function stringy {i=1;while (i<=arg(0)) {print i,arg(i),isstr(i);i++}}
	saved
	> stringy("foo",1,2,3,"bar","baz")
	1 541 1
	2 1 0
	3 2 0
	4 3 0
	5 545 1
	6 549 1

There is a corresponding C api for C User Functions:

	numvar isstringarg(numvar);

And a companion to getarg() that fetches an argument but throws an error if the argument is not a string:

	numvar getstringarg(numvar);

Here is an example sketch with a User Function called echo() that uses isstringarg() and getstringarg() to print string arguments separately from numeric ones.  The echo() function echoes back its arguments, with proper handling for strings:

	#include "Arduino.h"
	#include "bitlash.h"
	#include "src/bitlash.h"  // for sp() and printInteger()

	numvar func_echo(void) {
		for (int i=1; i <= getarg(0); i++) {
			if (isstringarg(i)) sp((const char *) getstringarg(i));
			else printInteger(getarg(i), 0, ' ');
			speol();
		}
	}

	void setup(void) {
		initBitlash(57600);	
		addBitlashFunction("echo", (bitlash_function) func_echo);	
	}

	void loop(void) {
		runBitlash();
	}


NOTE: The changes consume a little space on the expression evaluation stack, so some complex expressions that formerly worked may see an expression overflow.  Please report this if you see it.


## March 18, 2013: Bitlash running on Due

- Applied contributed Due patches; Bitlash should run on Due now.  Thanks, Bruce.

- Bumped copyright date in the startup banner.


## Feb 2013

- Added commander and ethernetcommander example sketches for Bitlash Commander.
- Added blinkm example sketch
- Added seriallcd example sketch

## November 10, 2012

- Fixed prog_char bug that broke compile on Ubuntu with avr-gcc 4.7.0


## November 3, 2012

Recent Bitlash changes of note:

- Published a new tool, "bloader.js", a program loader and serial monitor for Bitlash based on node.js.  See https://github.com/billroy/bloader.js

- Published a new tool, "serial-web-terminal", which provides a browser-based serial terminal monitor for a usb-connected Arduino.  See https://github.com/billroy/serial-web-terminal.git

- Fixed a millis rollover bug in the background task scheduler.  You can cherry-pick the fix at line 103 in src/bitlash-taskmgr.c -- also, there is a new example named examples/rollover that lets you set millis() to a value just before the rollover to test.

- Added getkey([prompt]) and getint([prompt]) to get user input.

- Compiles and runs on Linux and OS X, and there is a web-based Bitlash terminal you can run on Heroku so you can play with Bitlash in your browser.  See README-UNIX.md

- Compiles on the tiny core, with many feature amputations.  Not tested.  See README-TINY.md


## Bitlash Due Version compiles under Arduino 1.5

- Bitlash builds under the github working code for the new Arduino 1.5 IDE, both for the customary AVR targets and for the new ARM targets for the Arduino Due.  You may still get errors with the current Arduino 1.5 beta until a refresh is announced.  See the forum thread below to test using the latest github IDE code.

- The ARM has no eeprom.  Bitlash function storage in this alpha is in a 4k ram buffer that simulates EEPROM, except it vanishes at power-off.

- Without a Due here, I cannot test it, but field reports are welcome.

- Forum thread here: http://arduino.cc/forum/index.php/topic,128543.msg966899.html#msg966899


## September 10, 2012

- Let's call it "2.0"

- The Bitlash User's Guide is available as a pdf book at:
	http://bitlash.net/bitlash-users-guide.pdf

- Examples are updated for Arduino 1.0.1

- Bitlash is now licensed under the MIT license

- Released new wiki and landing page on Github

- New morse and morse2 examples: printf() to morse, blocking and non-blocking

- Check out the Textwhacker project at https://github.com/billroy/textwhacker
	for scrolling text output from Bitlash on a SparkFun LED matrix


## 2.0 RC5 Release Notes -- April 3, 2012

- Stalking Arduino 1.0: more #include fixes and warnings cleanup
	- The bitlashdemo example compiles and runs correctly in 1.0
	- Seeking bug reports

- The default for MAX_USER_FUNCTIONS is now 20 [bitlash-functions.c @ 268]

- RF12 support for NanodeRF; see examples/bitlash_rf
	- very early alpha code


## 2.0 RC4 Release Notes -- September 29, 2011

- Arduino 1.0 status
	- The bitlashdemo.pde example compiles and runs properly on Arduino 1.0b4
	- There are known issues with some examples:
		- The BitlashWebServer.pde example does not work for either the Arduino or Nanode ethernet interfaces
		- SD card support is untested and at risk due to size constraints
		- Most other examples need touching for the Arduino.h fix at least

- The BitlashWebServer example now works on Nanode, as well as the official shield
	- (Note: This works in Arduino 0022 only, for the moment)
	- Nanode users need to enable #define NANODE in BitlashWebServer.pde at about line 125
		- requires EtherShield library from from https://github.com/thiseldo/EtherShield
		- requires NanodeMAC library from from https://github.com/thiseldo/NanodeMAC

- SD card support is, by default, disabled.  To enable it:
	- Edit libraries/bitlash/src/bitlash-instream.c to turn on the SDFILE define
		at or near line 32, make this:
			//#define SDFILE
		look like this:
			#define SDFILE

- printf() and fprintf() now respect the width argument, including leading 0:
	> printf("%3d\n", 99);
	 99
	> printf("%03d\n", 99);
	099
	>

- You can define symbolic names for pins at compile time to suit your project
	- Turn on PIN_ALIASES at line 401 in src/bitlash-parser.c
	- Add your aliases to the pinnames and pinvalues tables
	- Now you can say led=1 or x=vin
	- Don't forget to set the pinmode()

- You can define built-in named functions in Bitlash script at compile time to suit your project
	- Think of it as a built-in customizable dictionary of Bitlash functions
	- These BUILT_IN functions live in flash (PROGMEM) so you can free up EEPROM
	- Flash space is the limiting factor; the internal bitlash 2 limit is 2^28-1 bytes
	- Add to the table in src/bitlash-builtins.c and see the notes there



## 2.0 RC3d Release Notes -- June 4, 2011

### Quick Start for SD Card Support:

- Download and install Bitlash 2.0 RC3d from http://bitlash.net
- Download and install SD Card library from http://beta-lib.googlecode.com/files/SdFatBeta20110604.zip
- Restart Arduino 0022, open examples->bitlash->bitlashsd and upload to your Arduino

### Quick Start without SD Card Support:

- Download and install Bitlash 2.0 RC3d from http://bitlash.net
- Edit libraries/bitlash/src/bitlash-instream.c to turn off the SDFILE define
	at or near line 32, make this:
		#define SDFILE
	look like this:
		//#define SDFILE
- If you are using a Mega2560, edit SdFat/SdFatConfig.h @ line 85, make this change:
	#define MEGA_SOFT_SPI 1
- Restart Arduino 0022, open examples->bitlash->bitlashdemo and upload to your Arduino

### Summary for this version:

- Runs scripts from SDCard file systems
- Has a language worth running from a file
- String arguments!
	printf("%d:%d:%d\n",h,m,s)
- Can write SDFILE from script, too:
	fprintf("logfile.dat","%d:%d:%d\n",h,m,s)

- Tested on and requires Arduino 0022

- This version runs scripts from SDCARD as well as EEPROM
	- Tested on Sparkfun OpenLog 1.0 and Adafruit Datalogger Shield hardware

- bitlashsd sd card demo REQUIRES SDFat Beta 20110604 available from:
	- download link: http://beta-lib.googlecode.com/files/SdFatBeta20110604.zip

	- to install, copy the "SDFat" library from the SDFatBeta distribution
		into the Arduino/libraries directory and restart Arduino
		
	- open the "bitlashsd" example and upload

	- to disable SDFILE support:
		- turn off the SDFAT define in bitlash-instream.c
		- open and upload the bitlashdemo example

- BUGFIX: 0xb broken by 0b1000!
	print 0xbbbbbbbb
	0

- BUGFIX: MAX_USER_FUNCTIONS error allowed only n-1 functions
	- bumped MAX_USER_FUNCTIONS to 8

- BUGFIX: bloader.py was not well-synchronized at the start of a file upload
	as a result it could lose the first several lines of a file
	it now syncs correctly with the command prompt on the arduino

- string arguments
	- printing strings with :s
- unary & operator
- unary * operator
- // comments

- doCommand()
	- is re-entrant
	- returns numvar (-1L on error)

- printf("format string\n", "foo", "bar",3)
	- format specifiers %s %d %u %x %c %b %% work as per C printf()
	- BUG/FEAT: width specifier is not handled for numeric printing
		- %4s works but %4d or anything else doesn't
	- see also fprintf() below for printing to file on sd card

- EXAMPLE: commands supported in the bitlashsd sd card demo

	dir
	exists("filename") 
	del("filename") 
	create("filename", "first line\nsecondline\n")
	append("filename", "another line\n")
	type("filename") 
	cd("dirname")
	md("dirname")
	fprintf("filename", "format string %s%d\n", "foo", millis);

### Running Bitlash scripts from sd card

	- put your multi-line script on an SD card and run it from the command line
		- example: bitlashcode/memdump
		- example: bitlashcode/vars

	- //comments work (comment to end of line)

	- functions are looked up in this priority / order:
		- internal function table
		- user C function table (addBitlashFunction(...))
		- Bitlash functions in EEPROM
		- Bitlash functions in files on SD card

	- beware name conflicts: a script on SD card can't override a function in EEPROM

	- BUG: the run command only works with EEPROM functions
		- it does not work with file functions
		- for now, to use run with a file function, use this workaround:
			- make a small EEPROM function to call your file function
			- run the the EEPROM function

	- startup and prompt functions on sd card are honored, if they exist

	- you can upload files using bitlashcode/bloader.py
		- python bloader.py memdump md
			... uploads memdump as "md"


----------

## 2.0 RC2 -- 06 March 2011

This release fixes a bug in RC1 which broke function definition in some cases.

Please report bugs to bill@bitlash.net


### BUG: Function definition was broken if the line contained ; or additionalcommands


If the command defining the function was the last thing on a line, RC1 worked correctly:

	function hello {print "Hello, world!"}		<-- worked

If there were characters after the closing } of the function definition, the bug
caused the function text to be mismeasured and spurious text would be appended to
the function definition, causing it to fail in use:

	> function hello { print "Hello, world!"};	<-- this is legal but it triggered bug in RC1
	saved
	> ls
	function hello { print "Hello, world"}};	<-- BUG: extra } saved at the end

The function text is measured more accurately in RC2.

Users are encouraged to upgrade to fix this bug.  

Existing functions may need to be fixed, as well.  Make sure the {} balance.


### Defining a function within a function fails.  That's ok, for now.

Noting a behavior that may change in a future release: an attempt to define a function
from within a function will fail silently or worse.  Don't do that.

	function foo {function bar{print "bar"};bar};	<-- don't do this!
	> foo
	saved
	> ls
	function foo {function bar{print "bar"};bar};
	function bar {};	<-- sorry, doesn't work in 2.0
	> bar
	> 

This isn't new: Bitlash 1.1 has the same internal implementation limitation, but 
it was nearly impossible for a human to get the backslash-quote combinations right 
to attempt the test.


## 2.0 RC1 -- 05 Feb 2011

- Syntax overhaul: the Bitlash 2.0 language is considerably different,
	and old macros will need to be updated to run in v2

	if (expression) { stmt;...;stmt;} else {stmt;...;stmt;}
	while (expression) { stmt; }
	switch (expression) { stmt0; stmt1; ... stmtN; }
	function hello {print "Hello, world!";}

- Macros are now Functions, they take arguments and can return a value
	arg(0) is the count of args you got
	arg(1..n) are the args
	return expression; to return a value; zero is assumed
	User Functions in C use this same scheme now

- Function (/macro) definition syntax has changed:
	function printmyargs {i=0; while ++i<arg(0) {print arg(i);}}

- New Functions
	bc: bitclear
	bs: bitset
	br: bitread
	bw: bitwrite

- New API calls
	setOutputHandler() api allows capture of serial output
	doCharacter() api allows char-at-a-time input to Bitlash

- New Examples
	- Bitlash web and telnet server

- Small Beans
	- Input buffer is 140 characters, up from 80.  twitter is the new Hollerith
	- Binary constants of the form 0b01010101 are supported

## 1.1 -- 04 Feb 2010
- User Functions (addBitlashFunction)

## 1.0 -- 17 Jan 2010
- Fixed filenames in the examples/ folder to remove '-'
- Updated copyright date in signon banner
- Doc set update: see http://bitlash.net

## 1.0rc2 -- 22 Jun 2009
- Fixed bug which botched handling of escaped characters in string constants.

## 1.0rc1 -- 01 Jun 2009
- See http://bitlash.net for updates

## 0.9 -- 23 Nov 2008

- License updated to LGPL 2.1 
- Added functions: 
	- beep(pin, freq, duration)
	- shiftout()
- SOFTWARE_SERIAL_RX: build-time configurable software serial rx on pins D0-D7
	- to support Ethernet integration
- Adafruit XPort Direct Ethernet integration works pretty well
- Sanguino integration
	- Compile for Sanguino Does The Right Thing re: serial ports
	- (there are some serial port interrupt issues pending)

## 0.8 -- 31 Oct 2008

- SOFTWARE_TX:
	- baud(pin, baud)
	- print #pin:expr
