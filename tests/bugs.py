# bug tests

import pexpect, sys, time
d = pexpect.spawn('python bitty.py -k');
d.expect('Listening')

c = pexpect.spawn('nc localhost 8080');
c.logfile_read = sys.stdout

c.sendline('boot')
c.expect('>')
c.sendline('print millis')
c.expect('>')

###
# BUG TEST CASE HERE
###
c.sendline('i=0; while i<5 {print i,"",;i++;}')
c.expect('0 1 2 3 4')
c.expect('>')
###

c.sendline('logout')
c.expect(pexpect.EOF)

c.close()
d.close() 
