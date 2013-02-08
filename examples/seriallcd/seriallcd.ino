/***

	seriallcd.ino:	Bitlash Driver for Sparkfun SerLCD v2.5

	Copyright (C) 2008-2013 Bill Roy
	MIT License; see LICENSE file.

***/
#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif
#include "bitlash.h"

extern void spb(char);
extern void resetOutput(void);
extern numvar setBaud(numvar, unumvar);

// The LCD is assumed to be connected to pin 7 
// and listening at 9600 baud
//
#define LCD_PIN	7
#define LCD_BAUD 9600

// todo:
// - setbaud 0x7c [baud specifier]
// - set width and number of lines

#define LCD_COMMAND1 0xfe
#define LCD_COMMAND2 0x7c

void send_cmd_2(char c) {
	setOutput(LCD_PIN);
	spb(LCD_COMMAND2);
	spb(c);
	resetOutput();
}

numvar func_bright(void) { 
	int brightness = getarg(1);
	if (brightness < 0) brightness = 0;
	else if (brightness > 29) brightness = 29;
	send_cmd_2(brightness | 0x80);
	return 0;	
}
numvar func_setsplash(void) { send_cmd_2(0x0a); return 0; }
numvar func_splash(void) { send_cmd_2(0x09); return 0; }	// toggles splash display


void send_cmd(char c) {
	setOutput(LCD_PIN);
	spb(LCD_COMMAND1);
	spb(c);
	resetOutput();
}

numvar func_clear(void) { send_cmd(0x01); return 0; }
numvar func_right(void) { send_cmd(0x14); return 0; }
numvar func_left(void) { send_cmd(0x10); return 0; }
numvar func_scrollright(void) { send_cmd(0x1c); return 0; }
numvar func_scrollleft(void) { send_cmd(0x18); return 0; }

numvar func_display(void) { 
	if (getarg(1)) send_cmd(0x0c);
	else send_cmd(0x08);
	return 0;
}
numvar func_underline(void) { 
	if (getarg(1)) send_cmd(0x0e);
	else send_cmd(0x0c);
	return 0;
}
numvar func_box(void) {
	if (getarg(1)) send_cmd(0x0d);
	else send_cmd(0x0c);
	return 0;
}

void move(byte row, byte col) {
	if (row == 1) col += 64;
	else if (row == 2) col += 16;
	else if (row == 3) col += 80;
	send_cmd((char) col | 0x80);
}

numvar func_move(void) {
	move(getarg(1), getarg(2));
	return 0;
}

// printat(row, col, format, arg, arg, arg...);
numvar func_lcdprintat(void) {
	move(getarg(1), getarg(2));
	setOutput(LCD_PIN);
	func_printf_handler(3,4);
	resetOutput();
	return 0;
}

numvar func_lcdprint(void) {
	setOutput(LCD_PIN);
	func_printf_handler(1,2);
	resetOutput();
	return 0;
}


void setup(void) {
	initBitlash(57600);		// must be first to initialize serial port

	setBaud(LCD_PIN, LCD_BAUD);

	addBitlashFunction("clear", (bitlash_function) func_clear);
	addBitlashFunction("move", (bitlash_function) func_move);
	addBitlashFunction("printat", (bitlash_function) func_lcdprintat);
	addBitlashFunction("lcdprint", (bitlash_function) func_lcdprint);
	addBitlashFunction("bright", (bitlash_function) func_bright);
	addBitlashFunction("display", (bitlash_function) func_display);
	addBitlashFunction("box", (bitlash_function) func_box);
	addBitlashFunction("under", (bitlash_function) func_underline);
	addBitlashFunction("right", (bitlash_function) func_right);
	addBitlashFunction("left", (bitlash_function) func_left);
	addBitlashFunction("sright", (bitlash_function) func_scrollright);
	addBitlashFunction("sleft", (bitlash_function) func_scrollleft);
	addBitlashFunction("setsplash", (bitlash_function) func_setsplash);
	addBitlashFunction("splash", (bitlash_function) func_splash);
}

void loop(void) {
	runBitlash();
}
