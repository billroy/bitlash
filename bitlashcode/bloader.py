#! /usr/bin/python
#
#	bloader.py: python serial port loader and interactive tty
#
# 	usage: python bloader.py foobar.btl
#
# 	Requires:
#		1. Python
#		2. pyserial from http://pyserial.wiki.sourceforge.net/pySerial
# 		3. pexpect from http://pexpect.sourceforge.net/pexpect.html
# 			see also http://www.noah.org/wiki/Pexpect
#
#	LICENSE
#
#	Copyright 2011 by Bill Roy
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

import serial, fdpexpect, sys, time, commands

device = None
#device = '/dev/tty.usbserial-A7006wXd'
baud = 57600

if not device:
	#devicelist = commands.getoutput("ls /dev/tty.usbserial*")
	#devicelist = commands.getoutput("ls /dev/ttyUSB*")		# this works on Linux
	devicelist = commands.getoutput("ls /dev/tty.usb*")		# this works on Teensy
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

#c.sendline('boot')
#waitprompt()

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
try:
	c.interact()
except:
	pass
c.close()
