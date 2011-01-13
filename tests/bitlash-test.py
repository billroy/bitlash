# Bitlash test script
#
# requires bitty.py from bitlash.net
# which in turn requires pyserial
# also requires pexpect from http://pexpect.sourceforge.net/pexpect.html
# see also http://www.noah.org/wiki/Pexpect

import serial, fdpexpect, sys, time, os

device = '/dev/tty.usbserial-A70063Md'
baud = 57600
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
c.sendline('foo:="switch arg(1) {print 0,;print 1,;print 2,;}"')
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
