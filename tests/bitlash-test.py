# Bitlash test script
#
# requires bitty.py from bitlash.net
# which in turn requires pyserial
# also requires pexpect from http://pexpect.sourceforge.net/pexpect.html
# see also http://www.noah.org/wiki/Pexpect

import pexpect, sys, time
d = pexpect.spawn('python bitty.py -k');
time.sleep(1.0)

c = pexpect.spawn('nc localhost 8080');
c.logfile_read = sys.stdout

c.sendline('boot')
c.expect('>')
c.sendline('print millis')
c.expect('>')

#c.sendcontrol('c')
#c.expect('^C\r\n>')

c.sendline('print abs(-1), abs(0), abs(1)')
c.expect('1 0 1')
c.expect('>')

c.sendline('print sign(-10), sign(0), sign(10)')
c.expect('-1 0 1')
c.expect('>')

c.sendline('if 1 print 1; else print 0')
c.expect('1')
c.expect('>')

c.sendline('if 0 print 1; else print 0')
c.expect('0')
c.expect('>')

c.sendline('i=0; while i<5 {print i,"",;i++;}')
c.expect('0 1 2 3 4')
c.expect('>')

c.sendline('rm foo')
c.expect('>')
c.sendline('foo:="switch arg(1) {print 0,;print 1,;print 2,;}"')
c.expect('saved')
c.expect('>')
c.sendline('i=-2; while i<4 foo(i++);')
c.expect('000122>')
c.sendline('rm foo')
c.expect('>')

c.sendline('i=0;while i<1000 {i++; if i>100 return 4; } print i;')
c.expect('4')
c.expect('>')

c.sendline('q=0; while q<10 {if q&1 {print "odd ",;} else {print "even ",;} q++;} print "done";')
c.expect('even odd even odd even odd even odd even odd done');
c.expect('>')


c.sendline('logout')
c.expect(pexpect.EOF)

c.close()
d.close() 