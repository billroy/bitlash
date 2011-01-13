# bitlash loader
#
# usage: python bloader.py foobar.btl
#
import serial, fdpexpect, sys, time

device = '/dev/tty.usbserial-A70063Md'
baud = 57600
serialport = serial.Serial(device, baud, timeout=0)
c = fdpexpect.fdspawn(serialport.fd)
c.logfile_read = sys.stdout

# no arguments: interactive mode, ^] to exit (like telnet)
if (len(sys.argv) < 2):
	c.sendline("");
	c.interact()
	sys.exit()

def waitprompt():
	c.expect('\n> ')
	time.sleep(0.1)

c.sendline('boot')
waitprompt()
c.sendline('print millis')
waitprompt()

#######################

# open the passed-in file and send it line by line to Bitlash

filename = sys.argv[1]

f=open(filename)
lines = f.readlines()
for line in lines:
	line = line.strip()
	if (len(line) > 0) and (line[0] != '#'):
		c.sendline(line.strip())
		waitprompt()

##########################

c.sendline('print millis')
waitprompt()
c.close()
