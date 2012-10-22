# Notes on Tiny85 port
October 20, 2012

## Status

- Builds and links for Tiny85 and Tiny84
	8176/8192 bytes

- does not have serial input yet

- many features cut for space; see list below

- completely untested, no telling if it runs


## Using the Arduino-Tiny core (0100-0015) installed from:

	http://code.google.com/p/arduino-tiny/

## To avoid R_AVR_13_PCREL errors at link time

You must use a modified ld per this forum post:

	http://arduino.cc/forum/index.php/topic,116674.msg878023.html#msg878023


## Much has been cut:

feature	before	after	savings
numvar 16 bit	5322	3644	1678
print	3644	3206	438
help	3206	3066	140
peep	3066	2934	132
switch	2934	2700	234
ps	2700	2610	90
printf	2610	1918	692
slash functions	1918	646	1272
user functions	646	472	174
rm *	472	420	52
reboot	420	404	16
builtin lookup	404	278	126
banner	278	204	74
pointToError	204	136	68
random()		??	0
prompt function
setup function

