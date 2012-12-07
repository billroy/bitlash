/***
	morseio.c: Morse input/output codec

	Bitlash is a tiny language interpreter that provides a serial port shell environment
	for bit banging and hardware hacking.

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
#include "src/bitlash.h"

#if defined(FLUXY_MORSE)

/***
	TODO: PTT management
	TODO: print prosigns BK SK AR correctly
***/

//#define tone_off() { led_on(LED1); led_on(LED2); }
//#define tone_on() { led_off(LED1);led_off(LED2); }
#define tone_off()
#define tone_on() 


/////////////////////////////////
// The Morsetab
//
// Each value in the table is bit-encoded as lllddddd, where:
//	lll is 3 bits of length [0..7] and 
//	ddddd is 5 bits of morse data, 1==dah and 0==dit
// since there are only 5 bits for morse data we handle 6 element symbols as a special case
// they are stored with a length of zero and an index into 'm6codes', an exception array below
//
#define m(a,b) ((a<<5) + b)
#define m6(b) m(0,b)
#define NIL m(6,0xc)

prog_char morsetab[] PROGMEM = {
	//2 SP      !      "      #    $       %    &        '          (         )      *    +         ,      -      .          slash
		NIL, m6(2), m6(6), NIL, m(7,9), NIL, m(5,8), m(6,0x1e), m(5,0x16), m6(3), NIL, m(5,0xa), m6(4), m6(5), m(6,0x15), m(5,0x12),
	//3   0   		1   		2   	3   	4   		5   	6   	7   		8   		9   	 :  	;  		<    =   	   >    ?
		m(5,0x1f), m(5,0x0f), m(5,7), m(5,3), m(5,1), m(5,0), m(5,0x10), m(5,0x18), m(5,0x1c), m(5,0x1e), m6(0), m6(1), NIL, m(5,0x11), NIL, m(6,0xc),
	//4	@       	a		b		c		d		e		f		g		h			i		j		k		l		m		n		o
		m(6,0x1a), m(2,1), m(4,8), m(4,0xa), m(3,4), m(1,0), m(4,2), m(3,6), m(4,0), m(2,0), m(4,7), m(3,5), m(4,4), m(2,3), m(2,2), m(3,7),
	//5  P   	Q   		R   	S   	T   	U   	V   	W   	X   	Y   		Z     [    \          ]    ^    _
		m(4,6), m(4,0xd), m(3,2), m(3,0), m(1,1), m(3,1), m(4,1), m(3,3), m(4,9), m(4,0xb), m(4,0xc), NIL, m(5,0x12), NIL, NIL, m(6,0xd)
};

/*
$ m(7,9)
*/

#define NUM_SPECIAL_CHARS 13
//                              012345 6789012
prog_char outliers[] PROGMEM = ":;!),-\"'.?@_#";

prog_char m6codes[NUM_SPECIAL_CHARS] PROGMEM = {
//	0=:  1=;  2=!  3=)  4=,  5=-  6="  7='  8=.  9=?  10=@ 11=_ 12=SK/#
	0x38,0x2a,0x2b,0x2d,0x33,0x21,0x12,0x1e,0x15,0x0c,0x1a,0x0d,0x05
};

//#define MORSE_SIDETONE 800
#define MORSE_WPM 15
#define morse_dit_ms (1200 / MORSE_WPM)
#define morse_dah_ms (3 * (1200 / MORSE_WPM))

void send_morse_element(int dt) {
	led_on(LED0);
	led_on(LED1);
	delay(dt);
	led_off(LED0);
	led_off(LED1);
	delay(morse_dit_ms);		// symbolspace of silence
#if 0
	led_flash(LED0, dt);
	led_flash(LED1, dt);
	delay(dt + morse_dit_ms);
#endif
}

void sendMorseElementPattern(byte bitcount, byte data) {
	byte mask = 1 << (bitcount-1);
	while (mask) {
		send_morse_element( (data & mask) ? morse_dah_ms : morse_dit_ms );
		mask >>= 1;
	}
	delay(2 * morse_dit_ms);	// letterspace less symbolspace taken above
}

void sendMorseChar(byte c) {
	if (c == ' ') { delay(6 * morse_dit_ms); return; }		// wordspace
	if ((c >= 'a') && (c <= 'z')) c = c - 'a' + 'A';
	if ((c < ' ') || (c > '_')) return;    // ignore bogus 7-bit and all 8-bit
	byte tablecode = pgm_read_byte(morsetab + c - ' ');
	byte len = (tablecode >> 5) & 7;
	if (len) sendMorseElementPattern(len, tablecode & 0x1f);
	else sendMorseElementPattern(6, pgm_read_byte(m6codes + tablecode));
}

void sendMorseString(char *str) {while (*str) sendMorseChar(*str++); }


//
///////////////////////////////

unsigned long t_state_start;	// time this element started
byte element_count;				// how many elements we've collected
byte element_bits;				// 1's where the dahs are

#define INCLUDE_IAMBIC 0
#if INCLUDE_IAMBIC
byte iambic=0;					// true if we use iambic two key keying
#endif

// debouncing
byte keyUpCount;
byte keyDownCount;
unsigned long dt;


// a little state machine for our codec
void (*statefunc)(byte) = NULL;
void morseSetState(void (*func)(byte)) {
	statefunc = func;
	t_state_start = millis();
	keyUpCount = keyDownCount = 0;
	//morseSnooze(STATE_HYSTERESIS);
}

// morseSnooze: Suspend this state machine for (int) millis
unsigned long endmorseSnooze;
void morseSnooze(unsigned long ms) {
	endmorseSnooze = millis() + ms;
}




// element-to-character primitives
void startChar() {
	element_count = element_bits = 0; 
}

// Add a dit (0) or dah (1) to the currently accumulating character
//
void add_elt(byte b) { 
	++element_count; 
	element_bits <<= 1; 
	element_bits |= b;
	//Serial.print(b ? " dah " : " dit ");
	////Serial.print((char)(element_count<<5 | element_bits), HEX);
}
//void add_dit(void) { add_elt(0); }
//void add_dah(void) { add_elt(1); }


void echoChar(byte c) {

	// morse feedback: play back what we got
	//sendMorseChar((char) c);

	// stash the character where Bitlash macros can see it
	assignVar('c'-'a', c);					// c = input char;
	assignVar('t'-'a', 0);					// t = 0;

	// morse command dispatch: set the state variable 'd' and reset t to zero
	if ((c == 'E') || (c == 'T')) {			// a single keypress: increment mode
		incVar('d'-'a');					// d++;
	}
	else if ((c >= 'A') && (c <= 'M')) {	// a letter: switch mode
		assignVar('d'-'a', c-'A'+1);		// d = new mode;
	}
	else {
		assignVar('d'-'a', 'j'-'a' + 1);	// unrecognized input -> macro mode "j" repeats the char
	}

	// get the task manager to run immediately
	releaseAllTasks();
}
 

// Send the current character
// todo: insert into serial buffer
//
void emitChar(void) {
byte i;
byte c;
	if (element_count < 6) {
		c = (element_count << 5) | element_bits;
		for (i = 0; i < 64; i++) {
			if (pgm_read_byte(morsetab + i) == c) {
				echoChar((char) i + 0x20);
				startChar();
				return;
			}
		}
		//if (i >= 64) echoChar('?');
	}
	else {
		for (i=0; i < NUM_SPECIAL_CHARS; i++) {
			if (element_bits == pgm_read_byte(m6codes + i)) {
				echoChar(pgm_read_byte(outliers + i));
				startChar();
				return;
			}
		}
		//if (i >= 7) echoChar('@');
	}

	// we get here if we can't match the input pattern
	// todo: bogus character handling
	startChar();
}


/////////////////////////
//	State handlers
//

#define KEY_DOWN_COUNT 3
#define KEY_UP_COUNT 3
#define KEY_DEBOUNCE_TIMEOUT 5

void morse_idle(byte);


/////////////////////////
//	EndWord: watch for end of word timeout
//
void morse_endword(byte keyDown) {
	tone_off();

#define SEND_SPACE_AFTER_WORD_TIMEOUT 0
#if SEND_SPACE_AFTER_WORD_TIMEOUT
	// on timeout we print a blank to separate words
	if (dt > (5 * morse_dit_ms)) {
		echoChar(' ');
		morseSetState(morse_idle);
	}

	// if we see a keydown we bug out and let the experts handle it
	else if (keyDown) morseSetState(morse_idle);
#else
	// sometimes we don't want the word break behavior
	// in that case, we bug out to idle right away
	morseSetState(morse_idle);
#endif
	
}


/////////////////////////
// KeyDown: watch for key-up
//
void morse_keyup(byte);

void morse_keydown(byte keyDown) {
	tone_on();
	if (keyDown) {
		keyUpCount = 0; 		// while the key is down we just wait
		if (dt >  10 * morse_dit_ms) reboot();
	}
	else if (++keyUpCount < KEY_UP_COUNT) morseSnooze(KEY_DEBOUNCE_TIMEOUT);
	else {
		if (dt >  10 * morse_dit_ms) {
			//Serial.println("CANCEL");
			// TODO: reset handling here: turn device off; sleep?
			reboot();
			//morseSetState(morse_idle);			// long dah is a reset
			return;
		}
		add_elt(dt > (2 * morse_dit_ms));

		// check for max length overflow and resolve char if so
		//if (element_count >= 6) emitChar();

		morseSetState(morse_keyup);
	}
}

/////////////////////////
// KeyUp: Handle end-element
//
void morse_keyup(byte keyDown) {
	tone_off();
	if (keyDown) {				// keyDown within 3 dit times: this char continues
		if (++keyDownCount < KEY_DOWN_COUNT) morseSnooze(KEY_DEBOUNCE_TIMEOUT);
		else {
			if (dt > (2 * morse_dit_ms)) emitChar();
			morseSetState(morse_keydown);
		}
	}
	else {						// key still up: check char timeout
		keyDownCount = 0;
		if (dt > (3 * morse_dit_ms)) {	// it was a char break
			emitChar();
			morseSetState(morse_endword);
		}
	}
}




//////////
// Iambic keyer
//
//
#if INCLUDE_IAMBIC

#define DIT 0
#define DAH 1
#define DIT_MASK (1<<0)
#define DAH_MASK (1<<1)
#define BOTH_MASK (DIT_MASK | DAH_MASK)

byte iElt;	// last iambic element sent (DIT or DAH) or 0 between words

void morse_iambic_between(byte);

void morse_iambic_keydown(byte keyMask) {

	tone_on();

	// dit, or dah?
	iElt = ((keyMask == DAH_MASK) || ((keyMask == BOTH_MASK) && (iElt == DIT)));
	add_elt(iElt);
	morseSnooze(iElt ? morse_dah_ms : morse_dit_ms);
	morseSetState(morse_iambic_between);
}

void morse_iambic_next_element(byte keyMask) {
	if (keyMask) morseSetState(morse_iambic_keydown);
	else {
		emitChar();
		iElt = 0;		// note we're between words
		morseSetState(morse_endword);
	}
}

void morse_iambic_between(byte keyMask) {
	tone_off();
	morseSnooze(morse_dit_ms);				// intersymbol spacing
	morseSetState(morse_iambic_next_element);
}
#endif		// INCLUDE_IAMBIC


////////////////////////////
// Idle: look for start of character
//
void morse_idle(byte keyDown) {
	tone_off();
	if (keyDown) {
		if (++keyDownCount < KEY_DOWN_COUNT) morseSnooze(KEY_DEBOUNCE_TIMEOUT);
		else {
			startChar();	// start up in not-idle mode

#if INCLUDE_IAMBIC
			morseSetState(iambic ? morse_iambic_keydown : morse_keydown);
#else
			morseSetState(morse_keydown);
#endif
		}
	}
	else keyDownCount = 0;
}




///////////////////
//
// External API
//
void initMorse(void) {
	morseSetState(morse_idle);
}

///////////////////
// This must be called in loop(), frequently, to run the state machine
//
void runMorse(byte keyDown) {
	if (millis() > endmorseSnooze) {
		dt = millis() - t_state_start;
		(*statefunc)(keyDown);
	}
}



#ifndef TINY_BITLASH
//////////////////////////
// Test driver
//
void setup(void) {
	Serial.begin(115200);

	// set up lcd
	pinMode(LCD_PIN, OUTPUT);
	lcd.begin(LCD_BAUD);
	
	Serial.println("Morse input codec test here!");
	initMorse();
	digitalWrite(6,1);	// engage pullup
	digitalWrite(15,1);

	tone_init_morse();
	sendMorseString("ok");
}

void loop(void) {
	//Serial.print(digitalRead(6) ? "up " : "down ");
	//delay(100);

	//runMorse(!digitalRead(6));

	iambic = 1;
	runMorse(!digitalRead(15)<<1 | !digitalRead(6));
}
#endif

#endif
