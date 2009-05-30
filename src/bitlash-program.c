/***
	bitlash-program.c

	Bitlash is a tiny language interpreter that provides a serial port shell environment
	for bit banging and hardware hacking.

	See the file README for documentation.  Or just upload this file as a sketch and play.

	Bitlash lives at: http://bitlash.net
	The author can be reached at: bill@bitlash.net

	Copyright (C) 2008, 2009 Bill Roy

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
#include "avr/eeprom.h"

// This file contains the source of the eeprom macros which are
// loaded when the chip is flashed.

// Choose a program load:
#define BLINK 1
//#define MOUSEDEMO 1
//#define PONG 1


#ifdef BLINK
char startup[] EEMEM = 

	// startup code
	"go\0run t1,100\0"

	// basic led toggling
	"t1\0d1=!d1\0"

#if 0
	// test: macros
	"inca\0++a\0"
	"incb\0++b\0"
	"five\01;2;3;4;5\0"
#endif
;
#endif


#ifdef MOUSEDEMO
// A simple startup command set

char startup[] EEMEM = 

	// startup code
	"go\0d3=1;t=1000;r=13;u=25;s=7<<6;c=0;run t1,97;run demo,11\0"

	// basic led toggling
	"t1\0d1=!d1\0"
	"t4\0d4=!d4\0"

	// demo
	"demo\0switch d%4:sq,bx,circle,mr;if !d3:d++;snooze(10);delay(999)\0"

	// square
	// p: state tick counter
	// q: state selector
	// u: ticks to be in this state
	"sq\0switch q%4:ea,so,we,no;if ++p>u:p=0;q++\0"

	// butterfly XX: 6-state pattern generator
	// b: state of the butterfly [0..6]
	// p: counts cycles in this state, up to u
	"bx\0switch b%6:se,ne,so,nw,sw,no;if ++p>u:p=0;b++\0"

	// circle (from AVRUSB HIDMouse example)
	//"cini\0s=7<<6;c=0\0"
	// s,c: sin and cos values
	// x,y: dx,dy values for this step
	"cstp\0x=(c+32*sign(c))>>6;s=s+x;y=(s+32*sign(s))>>6;c=c-y\0"
	"circle\0cstp;move(x,y)\0"

	// mr: mouse random
	// r: radius of motion
	"mr\0move(random(r)-r/2,random(r)-r/2);snooze(random(t));\0"
	//"gonzo\0t=50;r=50\0"
	//"namaste\0t=10000;r=16\0"

	// wr: wheel random
	//"wr\0wheel(random(r)-r/2);snooze(t)\0"

	// Mouse moves of r steps in the various cardinal directions
	"no\0move(0,-r)\0"
	"ne\0move(r,-r)\0"
	"ea\0move(r,0)\0"
	"se\0move(r,r)\0"
	"so\0move(0,r)\0"
	"sw\0move(-r,r)\0"
	"we\0move(-r,0)\0"
	"nw\0move(-r,-r)\0"
;
#endif


#ifdef PONG
// Pong, without paddles
// assumes piezo on D4 for the chirps
//
// x,y: ball location
// u,v: ball velocity
// m: initial velocity bound
// w,h: width,height of playing field
// p: probablilty the paddle misses and we restart (%)
// s: true for sound on D4
//
char startup[] EEMEM = 

	// "go" to start 'er up
	"go\0pi;run ps\0"

	// pi: Pong Init
	"pi\0w=200;h=150;g=0;p=5;m=5;s=1;t=20;bi\0"

	// bi: Ball Init
	"bi\0x=random(w);y=random(h);u=random(m)-7;v=random(m)-7\0"

	// bip: signal wall collision
	"bip\0i=20;while i--:d1=!d1;if s:d4=!d4\0"

	// ps: Pong Step -- do one frame
	"ps\0dx;dy;move(u,v);snooze(t)\0"

	// ds: step in x; check for collision and restart
	"dx\0x=x+u;if x<0||x>w:u=-u;bip;if random(100)<p:bi\0"

	// dy: step in y
	"dy\0y=y+v;if y<0||y>h:v=-v;bip\0"
;
#endif	

