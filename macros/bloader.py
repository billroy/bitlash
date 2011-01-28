# bitlash loader
#
# usage: python bloader.py foobar.btl
#
import serial, fdpexpect, sys, time, commands

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
	time.sleep(0.1)

c.sendline('boot')
waitprompt()

#######################

# if a filename argument was provided, open the file and send it line by line to Bitlash
if (len(sys.argv) >= 2):
	filename = sys.argv[1]
	f=open(filename)
	lines = f.readlines()
	for line in lines:
		line = line.strip()
		if (len(line) > 0) and (line[0] != '#'):
			c.sendline(line.strip())
			waitprompt()

##########################

# after the file is sent, or if there are no arguments: 
# interactive mode, ^] to exit (like telnet)
#c.sendline("");		# get a fresh prompt
c.interact()
c.close()
