/***
	bitlash-eeprom.c

	Bitlash is a tiny language interpreter that provides a serial port shell environment
	for bit banging and hardware hacking.

	See the file README for documentation.

	Bitlash lives at: http://bitlash.net
	The author can be reached at: bill@bitlash.net

	Copyright (C) 2008-2011 Bill Roy

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

***/
#include "bitlash.h"


/***
	This is a tiny attribute-value pair database for small EEPROM devices.
***/

// terrible horrible eeprom addressing kludge
// we steal a high bit to distinguish eeprom addresses
#define EEKLUDGE 0x8000
#define EEMASK   0x7fff
char isram(char *addr) { return (!((int) addr & EEKLUDGE)); }
char* kludge(int addr) { return ((char *)(addr | EEKLUDGE)); }
int dekludge(char *addr) { return ((int) addr & EEMASK); }


//////////////////////////////////////////
//
//	Reading from the EEPROM
//

#if 0
#define getString(addr, str, buflen) strcpy_P(addr, str)
#else
// Fetch a string from EEPROM at addr into buffer at str
void getString(int addr, char *str, int buflen) {
	while (--buflen) {
		byte c = eeread(addr++);
		if ((c == 0) || (c == 255)) break;
		*str++ = c;
	}
	*str = 0;
}
#endif


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
	underflow(M_eeprom);
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


// write id:value, unless value is empty, in which case we erase the id
void writeMacro(char *id) {
	eraseentry(id);

	// we need to know the macro value length to allocate space for it
	// we don't realistically have enough buffer space handy to parse it into
	// so this is a two-pass operation: first we measure the text, then on
	// the second pass we stuff it into the eeprom
	//
	// measure length of macro value
	// we get here with inchar = first char of macro and fetchptr one past that
	//
//	char *fetchmark = --fetchptr;		// back up and mark first char of macro text
//	primec();							// re-prime
	char *fetchmark = fetchptr;			// mark first char of macro text
	expval = 0;							// zero the count
	parsestring(&countByte);			// now expval is the macro value length
	if (!expval) return;				// empty string? we're done
	
	int addr = findhole(strlen(id) + expval + 2);	// longjmps on fail
	if (addr >= 0) {
		saveString(addr, id);

		// reset parse context
		fetchptr = fetchmark;
		primec();

		expval = addr + strlen(id) + 1;		// set up address for saveByte
		parsestring(&saveByte);
		saveByte(0);
	}
}


#if 0
// Write a macro definition to the eeprom
// We come here with sym == s_define (the := in id:="value")
// The global idbuf has the id parsed earlier
void defineMacro(void) {
char id[IDLEN+1];
	strncpy(id, idbuf, IDLEN+1);	// save id string through value parse
	getsym();						// scan past := to putative beginning s_quote
	if (sym != s_quote) expected(M_string);
	writeMacro(id);					// consumes closing quote

#if defined(SOFTWARE_SERIAL_TX) || defined(HARDWARE_SERIAL_TX)
	msgpl(M_saved);
#endif

	getsym();						// prefetch next sym past closing quote
}
#endif


// Parse and store a function definition
//
void cmd_function(void) {
char id[IDLEN+1];			// buffer for id

	getsym();				// eat "function", get putative id
	if ((sym != s_undef) && (sym != s_macro)) unexpected(M_id);
	strncpy(id, idbuf, IDLEN+1);	// save id string through value parse
	eraseentry(id);
	
	getsym();		// eat the id, move on to '{'

	// provide for function functionname()
	if (sym == s_lparen) {
		getsym();	// eat '('
		if (sym != s_rparen) expectedchar(')');
		getsym();	// eat ')'
	}

	if (sym != s_lcurly) expected(s_lcurly);

	// measure the macro text using skipstatement
	// fetchptr is on the character after '{'
	//
	char *startmark = fetchptr;		// mark first char of macro text
	void skipstatement(void);
	skipstatement();				// gobble it up without executing it
	char *endmark = fetchptr;		// and note the char past '}'

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
		//else if (c == '"') { spb('\\'); spb('"'); }
		else if (c == '\\') { spb('\\'); spb('\\'); }
		else if (c == '\n') { spb('\\'); spb('n'); }
		else if (c == '\t') { spb('\\'); spb('t'); }
		else if (c == '\r') { spb('\\'); spb('r'); }
		else if ((c >= 0x80) || (c < ' ')) {
			spb('\\'); spb('x'); 
			if (c < 0x10) spb('0'); printHex(c);
		}
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

#if 0
		eeputs(start);
		msgp(M_defmacro);
		start = findend(start);
		eeputs(start);
		spb('"');
		spb(';');
		speol();
		start = findend(start);
#endif
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

		if (c == 0) spb('\\');
		//else if ((c == 255) || (c < 0)) spb('.');
		else if (c == 255) spb('.');
		else if (c < ' ') spb('^');
		else spb(c);
		i++;
	}
	speol();
}



