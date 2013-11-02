/***
	bitlash-eeprom.c

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


/***
	This is a tiny attribute-value pair database for small EEPROM devices.
***/


// scan from addr for an occupied byte
int findoccupied(int addr) {
	while (addr < ENDDB) {
		if (eeread(addr) != EMPTY) return addr;
		addr++;
	}
	return FAIL;
}


// return the address of the first unused space at or after addr
int findunoccupied(int addr) {
	while (addr < ENDDB) {
		if (eeread(addr) == EMPTY) return addr;
		addr++;
	}
	return FAIL;
}


// find the end of this occupied string slot.  returns location or ENDDB.
int findend(int addr) {
	while (++addr < ENDDB) {
		byte c = eeread(addr);
		if (c == EMPTY) return addr;	// return pointer to first empty byte
		if (!c) return (++addr);		// or first byte past terminator
	}
	return ENDDB;
}


// return true if string in EEPROM at addr matches string at str
char eestrmatch(int addr, char *str) {
	while (*str) if (eeread(addr++) != *str++) return 0;
	if (eeread(addr) == 0) return 1;	// ended at the same place?
	return 0;
}


// find an entry in the db; return offset of id or FAIL
int findKey(char *id) {
int start = STARTDB;
	while (start < ENDDB-4) {
		// find the next entry
		start = findoccupied(start);
		if (start == FAIL) return FAIL;

		// start points to EEPROM id - check for match with id		
		if (eestrmatch(start, id)) return start;

		// no match - skip the id and its value and continue scanning
		start = findend(start);		// scan past id
		start = findend(start);		// and value
	}
	return FAIL;
}


// Look up an entry by key.  Returns -1 on fail else addr of value.
int getValue(char *key) {
	int kaddr = findKey(key);
	return (kaddr < 0) ? kaddr : findend(kaddr);
}


// find an empty space of a given size or eep
int findhole(int size) {
int starthole = STARTDB, endhole;
	for (;;) {
		if (starthole + size > ENDDB) break;		// ain't gonna fit
		starthole = findunoccupied(starthole);		// first byte of next hole, or
		if (starthole == FAIL) break;				// outa holes

		endhole = findoccupied(starthole);			// first byte or next block, or
		if (endhole == FAIL) endhole = ENDDB+1;		// the first byte thou shall not touch

		// endhole is now on first char of next non-empty block, or one past ENDDB
		if ((endhole - starthole) >= size) return starthole;	// success
		starthole = endhole;		// find another hole
	}
	overflow(M_eeprom);
	return 0;		// placate compiler
}




///////////////////////////////
//
// Writing to the EEPROM
//

// Save string at str to EEPROM at addr
void saveString(int addr, char *str) {
	while (*str) eewrite(addr++, *str++);
	eewrite(addr, 0);
}

// erase string at addy.  return addy of byte past end.
int erasestr(int addr) {
	for (;;) {
		byte c = eeread(addr);
		if (c == EMPTY) return addr;
		eewrite(addr++, EMPTY);
		if (!c) return addr;
	}
}

// erase entry by id
void eraseentry(char *id) {
	int entry = findKey(id);
	if (entry >= 0) erasestr(erasestr(entry));
}

// parsestring helpers
void countByte(char c) { expval++; }
void saveByte(char c) { eewrite(expval++, c); }



// Parse and store a function definition
//
void cmd_function(void) {
char id[IDLEN+1];			// buffer for id

	getsym();				// eat "function", get putative id
	if ((sym != s_undef) && (sym != s_script_eeprom) &&
		(sym != s_script_progmem) && (sym != s_script_file)) unexpected(M_id);
	strncpy(id, idbuf, IDLEN+1);	// save id string through value parse
	eraseentry(id);
	
	getsym();		// eat the id, move on to '{'

	if (sym != s_lcurly) expected(s_lcurly);

	// measure the macro text using skipstatement
	// fetchptr is on the character after '{'
	//
	// BUG: This is broken for file scripts
	char *startmark = (char *) fetchptr;		// mark first char of macro text
	void skipstatement(void);
	skipstatement();				// gobble it up without executing it
	char *endmark = (char *) fetchptr;		// and note the char past '}'

	// endmark is past the closing '}' - back up and find it
	do {
		--endmark;
	} while ((endmark > startmark) && (*endmark != '}'));
	
	int idlen = strlen(id);
	int addr = findhole(idlen + (endmark-startmark) + 2);	// longjmps on fail
	if (addr >= 0) {
		saveString(addr, id);		// write the id and its terminator
		addr += idlen + 1;		// advance to payload offset
		while (startmark < endmark) eewrite(addr++, *startmark++);
		eewrite(addr, 0);
	}

	msgpl(M_saved);
}


// print eeprom string at addr
void eeputs(int addr) {
	for (;;) {
		byte c = eeread(addr++);
		if (!c || (c == EMPTY)) return;
#if 0
		//else if (c == '"') { spb('\\'); spb('"'); }
		else if (c == '\\') { spb('\\'); spb('\\'); }
		else if (c == '\n') { spb('\\'); spb('n'); }
		else if (c == '\t') { spb('\\'); spb('t'); }
		else if (c == '\r') { spb('\\'); spb('r'); }
		else if ((c >= 0x80) || (c < ' ')) {
			spb('\\'); spb('x'); 
			if (c < 0x10) spb('0'); printHex(c);
		}
#endif
		else spb(c);
	}
}

// list the strings in the avpdb
void cmd_ls(void) {
int start = STARTDB;
	for (;;) {
		// find the next entry
		start = findoccupied(start);
		if (start == FAIL) return;

		msgp(M_function);
		spb(' ');
		eeputs(start);
		spb(' ');
		spb('{');
		start = findend(start);
		eeputs(start);
		spb('}');
		spb(';');
		speol();
		start = findend(start);
	}
}

void cmd_peep(void) {
int i=0;

	while (i <= ENDEEPROM) {
		if (!(i&63)) {speol(); printHex(i+0xe000); spb(':'); }
		if (!(i&7)) spb(' ');
		if (!(i&3)) spb(' ');		
		byte c = eeread(i) & 0xff;

		//if (c == 0) spb('\\');
		if (c == 0) spb('$');
		//else if ((c == 255) || (c < 0)) spb('.');
		else if (c == 255) spb('.');
		else if (c < ' ') spb('^');
		else spb(c);
		i++;
	}
	speol();
}



