/***
	bitlash-builtins.c

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
#include "bitlash.h"

/**********

Built-in Bitlash Scripts

This feature allows you to add built-in scripts at compile time.  

The scripts are stored in flash in the table defined below.  

Add your entries before the sentinel at the end, in pairs, using the BUILT_IN define: 
one string for the name, one string for the script.  

The BUILT_IN macro supplies the necessary null terminations.

NOTE WELL: The table is searched _last_ among all the spaces, so any function defined anywhere
will override the value in the table here.  This means you can easily shoot yourself in the foot, 
like this, from the command line:

	> function high {return low}
	
... for an endless supply of interesting debugging experiences.  Be careful.

**********/

// use this define to add to the builtin_table
#define BUILT_IN(name, script) name "\0" script "\0"

const prog_char builtin_table[] PROGMEM = {

	// The banner must be first.  Add new builtins below.
	BUILT_IN("banner",	
#if defined(TINY_BUILD)
		"print 2+2")
//		"while 1 {print millis;delay(999);}")
#else
		"print \"bitlash here! v2.0 (c) 2013 Bill Roy -type HELP-\",free,\"bytes free\"")
#endif

	// Add user built-ins here.  Some examples:
#if 0
	BUILT_IN("low",		"return 0")
	BUILT_IN("high",	"return 1")
	BUILT_IN("input",	"return 0")
	BUILT_IN("output",	"return 1")
	BUILT_IN("digitalread",	"return dr(arg(1))")
#endif

	// This sentinel must be last	
	BUILT_IN("","")						// guards end of table, must be last
};



byte findbuiltin(char *name) {
const prog_char *wordlist = builtin_table;

	while (pgm_read_byte(wordlist)) {
		int result = strcmp_P(name, wordlist);
		wordlist += strlen_P(wordlist) + 1;		// skip the name we just tested
		if (!result) {							// got a match:
			sym = s_script_progmem;				// set type to progmem script
			symval = (numvar) wordlist;			// value is starting address of script text
			return 1;
		}

		// Speed optimization: sort the builtins_table by function name,
		// and then enable this line for a small speedup
		//else if (result < 0) break;

		else wordlist += strlen_P(wordlist) + 1;	// else skip value string and move along
	}
	return 0;
}


//	Banner and copyright notice
//
void displayBanner(void) {
	// print the banner and copyright notice
	// please note the license requires that you maintain this notice
	execscript(SCRIPT_PROGMEM, (numvar) (builtin_table + strlen(builtin_table)+1), 0);
}
