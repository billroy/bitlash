#! /usr/bin/python
#
# 	Bitlash test script
#
# 	requires bitty.py from bitlash.net
# 	which in turn requires pyserial
# 	also requires pexpect from http://pexpect.sourceforge.net/pexpect.html
# 	see also http://www.noah.org/wiki/Pexpect
#
#	LICENSE
#
#	Copyright 2010 by Bill Roy
#
#	Permission is hereby granted, free of charge, to any person
#	obtaining a copy of this software and associated documentation
#	files (the "Software"), to deal in the Software without
#	restriction, including without limitation the rights to use,
#	copy, modify, merge, publish, distribute, sublicense, and/or sell
#	copies of the Software, and to permit persons to whom the
#	Software is furnished to do so, subject to the following
#	conditions:
#	
#	The above copyright notice and this permission notice shall be
#	included in all copies or substantial portions of the Software.
#	
#	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
#	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
#	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
#	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
#	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
#	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
#	OTHER DEALINGS IN THE SOFTWARE.
#
###############################################################################

import serial, fdpexpect, sys, time, os, commands

device = None
#device = '/dev/tty.usbserial-A7006wXd'
baud = 57600

if not device:
	devicelist = commands.getoutput("ls /dev/tty.usbserial*")
	#devicelist = commands.getoutput("ls /dev/ttyUSB*")		# this works on Linux
	if devicelist[0] == '/': device = devicelist
	if not device: 
		print "Fatal: Can't find usb serial device."
		sys.exit(0);

serialport = serial.Serial(device, baud, timeout=0)
c = fdpexpect.fdspawn(serialport.fd)
c.logfile_read = sys.stdout

def waitprompt():
	c.expect('\n> ')

waitprompt()
c.sendline('stop *;print millis')
waitprompt()

c.sendline('print abs(-1), abs(0), abs(1)')
c.expect('1 0 1')
waitprompt()

c.sendline('print sign(-10), sign(0), sign(10)')
c.expect('-1 0 1')
waitprompt()

c.sendline('if 1 print 1; else print 0')
c.expect('1')
waitprompt()

c.sendline('if 0 print 1; else print 0')
c.expect('0')
waitprompt()

c.sendline('i=0; while i<5 {print i,"",;i++;} print;')
c.expect('0 1 2 3 4')
waitprompt()

c.sendline('rm foo')
waitprompt()
c.sendline('function foo {switch arg(1) {print 0,;print 1,;print 2,;}}')
c.expect('saved')
waitprompt()
c.sendline('i=-2; while i<4 foo(i++); print;')
c.expect('000122')
waitprompt()
c.sendline('rm foo')
waitprompt()

c.sendline('i=0;while i<1000 {i++; if i>100 return 4; } print i;')
c.expect('4')
waitprompt()

c.sendline('q=0; while q<10 {if q&1 {print "odd ",;} else {print "even ",;} q++;} print "done";')
c.expect('even odd even odd even odd even odd even odd done');
waitprompt()

c.sendline('t=millis;i=1000;while i {if i&1 j++; else j--; i--;} print millis-t,"done";')
c.expect('done')
waitprompt()

c.sendline('i=-2;while i++<3 if i<=0 print 0,;else print 1,; print;')
c.expect('00111')
waitprompt()

c.sendline('print millis')
waitprompt()

c.sendline('i=-2;while i++<3 {if i<=0 print 0,;else print 1,;} print;')
c.expect('00111')
waitprompt()

c.sendline('i=0; while i<32 print br(0xaaaaaaaa,i++),; print;')
c.expect('01010101010101010101010101010101')
waitprompt()

c.sendline('i=0; while i<32 print br(0x55555555,i++),; print;')
c.expect('10101010101010101010101010101010')
waitprompt();

c.sendline('x=0;i=0;while i<32 x=bs(x,i++); print x;')
c.expect('-1');
waitprompt()

c.sendline('x=-1;i=0;while i<32 x=bc(x,i++); print x;')
c.expect('0')
waitprompt()

c.sendline('print millis')
waitprompt()

#c.sendline('logout')
#c.expect(fdpexpect.EOF)

c.close()
#d.close()
