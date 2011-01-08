# bitlash loader
#
# usage: python bloader.py foobar.btl
#
import pexpect, sys, time
d = pexpect.spawn('python ../tests/bitty.py -k');
d.expect('Listening')
d.logfile=None

c = pexpect.spawn('nc localhost 8080');
c.logfile_read = sys.stdout

c.sendline('boot')
c.expect('>')
c.sendline('print millis')
c.expect('>')
#######################

# open the passed-in file and send it line by line to Bitlash

filename = sys.argv[1]

f=open(filename)
lines = f.readlines()
for line in lines:
	line = line.strip()
	if (len(line) > 0) and (line[0] != '#'):
		c.sendline(line.strip())
		c.expect('\n> ')

##########################
c.sendline('print millis')
c.expect('\n> ')

c.sendline('logout')
c.expect(pexpect.EOF)

c.close()
d.close() 
