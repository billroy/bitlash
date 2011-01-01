# bug tests

import pexpect, sys, time
d = pexpect.spawn('python bitty.py -k');
time.sleep(1.0)

c = pexpect.spawn('nc localhost 8080');
c.logfile_read = sys.stdout

c.sendline('boot')
c.expect('>')
c.sendline('print millis')

###
#BUG: skipping null print string after while is done
c.sendline('i=0; while i++<5 {print "";}')
c.expect('\r\n\r\n\r\n\r\n\r\n>');
#works: i=0; while i<5 {print i,"x",;i++;}
#works: i=0; while i++<5 {print "woo hoo";}

c.sendline('i=0; while i<5 {print i,"",;i++;}')
c.expect('0 1 2 3 4')
c.expect('>')
###

c.sendline('logout')
c.expect(pexpect.EOF)

c.close()
d.close() 
