#! /usr/bin/python
#
#	bitty.py: serial-to-network multiplexer for Arduino
#
#	This is a serial port proxy.  It makes a usbserial port available over the network
#	for connection via telnet, nc, or your favorite telnet client.
#
#	There are some nice features for Arduino users.  You can unplug one Arduino and
#	plug in another and bitty will find it.  And the network port and baud rate are
#	adjustable.
#
#	Requires:
#		python 2.4 or 2.5
#		pyserial from http://pyserial.wiki.sourceforge.net/pySerial
#			tested with pyserial 2.4 on OS X and Fedora 4
#
#	To use: first, start the serial-to-net proxy and leave it running:
#		$ python bitty.py [options]
#
#	and then, in another terminal window, connect to it with your favorite telnet or nc program:
#		$ nc localhost 8080
#		$ telnet localhost 8080
#		$ telnet arduino.bitlash.net 8080
#
#	plug in Arduino and you should connect up automatically.
#	Type 'logout' when done for a clean disconnect.
#
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

__version__ = '1.0'
__copyright__ = 'Copyright 2010 by Bill Roy'

import serial, time, socket, commands, traceback, os, sys

# serial port config
usbdevice = None
baud = 57600

# network config
port = 8080

# options
kbdpassthru = False
showflow = False


# change below here at your own risk
serialport = None
netsocket = None
netclient = None
clientaddress = None

# Net pump thread, and queued interface
import Queue
netq = Queue.Queue()
serialq = Queue.Queue()


def closeSerialPort():
	global serialport
	if serialport and serialport.isOpen(): 
		print "Closing: ", serialport.portstr
		try:
			if netclient: 
				netclient.send("Closing ")
				netclient.send(serialport.portstr)
				netclient.send("\r\n");
		except: pass
		serialport.close()


def openSerialPort():
	global serialport
	closeSerialPort()


	# serial port autoconfig
	device = usbdevice					 # command line overrides auto select
	if not device:
		devicelist = commands.getoutput("ls /dev/tty.usbserial*")
		#devicelist = commands.getoutput("ls /dev/ttyUSB*")		# this works on the XO/Fedora
	
		if devicelist[0] == '/': device = devicelist
	if not device: 
		print "Waiting for device..."
		return False

	print "Connecting to", device, baud, "..."
	if netclient:
		netclient.send('Connecting to ');
		netclient.send(device);
		netclient.send('... ');

	try:
		# two stop bits helps paste-to-terminal not lose characters
		serialport = serial.Serial(device, baud, timeout=0, stopbits=serial.STOPBITS_TWO)
		print 'Opened port: ', serialport.portstr
		if netclient:
			netclient.send("connected.\r\n")
	except:
		print 'Failed to open port'
		if netclient:
			netclient.send("failed.\r\n")
		return False
	return True


# thread to read and queue serial input
# assumes opening the serial port is handled elsewhere
def serialPumpTask(usbdevice, baud):
	while True:
		if serialport and serialport.isOpen():
			try:
				data = serialport.read(1024);
				if data:
					if showflow: print "SER:",data
					serialq.put(data)
				else:
					time.sleep(0.1)
			except:
				print "Exception reading serial port"
				traceback.print_exc()
				closeSerialPort()
		else:
			time.sleep(1.0)

def kbdPumpTask(d1,d2):
	while True:
		try:
			netq.put(os.read(sys.stdin.fileno(), 1))
		except:
			print "Exception in keyboard handler"
			traceback.print_exc()

# send network data to the serial port
def handleNetworkInput(data):
	global serialport
	try:
		if serialport and serialport.isOpen(): 
			#serialport.write(data);
			for i in range(len(data)): 
				serialport.write(data[i])
				time.sleep(0.05)
	except:
		print "Exception writing serial port"
		traceback.print_exc()
		closeSerialPort()


# send serial port data to the network socket
def handleSerialInput(data):
	global netclient
	try:
		if netclient: netclient.send(data)
		if kbdpassthru: 
			sys.stdout.write(data)
			sys.stdout.flush()
	except:
		print "Exception writing network port"
		traceback.print_exc()
		closeSerialPort()


# thread to read and queue network input
def netPumpTask(port, dummy):

	global netsocket, netclient, clientaddress
	netsocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	netsocket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	#socket.setblocking(0)
	netsocket.bind(('', port)) 
	netsocket.listen(1) 	# allow no waiting connections

	while True: 
		try:
			print "Listening for network connection on", socket.getfqdn(), ':', port, '/', socket.gethostname()
			netclient, clientaddress = netsocket.accept()
			print "Connection from", clientaddress
			netclient.send("g'day from bitty ")
			netclient.send(__version__)
			netclient.send(" -- type 'logout' to disconnect")
			netclient.send("\r\n");

			while True:
				if not netclient: break		# client went away: go get another one
				while not openSerialPort():
					netclient.send("Waiting for device...\r\n");
					time.sleep(2);

				while serialport.isOpen():
					data = netclient.recv(10240) 
					if data: 
						if showflow: print "NET:", data
						if data[0:6].find('logout') == 0:
							if netclient: netclient.send("Disconnected.\r\n");
							netclient.close()
							netclient = None
							closeSerialPort()
							time.sleep(1.0)		# pause to allow disconnect
						else: 
							netq.put(data)
					else: time.sleep(0.1)
		except:
			print "Exception in net pump"
			traceback.print_exc()
			closeSerialPort()


def parseOptions():
	# parse command line options
	from optparse import OptionParser
	usage = "usage: %prog [options]"
	parser = OptionParser()

	parser.add_option("-p", "--port",
						dest="port", type='int',
						help="network connection port [8080]")
	
	parser.add_option("-u", "--usbdevice",
						dest="usbdevice",
						help="name of USB serial port device for serial connection [/dev/tty.usbserial*]")

	parser.add_option("-b", "--baud",
						dest="baud", type='int',
						help="baud rate for port specified by -u [57600]")

	parser.add_option("-k", "--keyboard-passthru", action="store_true", dest="kbdpassthru")

	parser.add_option("-d", "--debug", action="store_true", dest="debug")

	(options, args) = parser.parse_args()

	if options.port:
		global port
		port = options.port
		print "Listening for connections on port:", port
	
	if options.baud:
		global baud
		baud = options.baud
		print "Serial port baud rate set to:", baud
	
	if options.usbdevice:
		global usbdevice
		usbdevice = options.usbdevice
		print "Using USB serial device: ", usbdevice

	if options.debug:
		global showflow
		showflow = True

	if options.kbdpassthru:
		global kbdpassthru
		kbdpassthru = True

	#if options.password:
	#	import getpass
	#	password = getpass.getpass("Password:")


if __name__ == '__main__':

	parseOptions()

	import thread
	net_pump_thread = thread.start_new_thread(netPumpTask, (port, 0))
	serial_pump_thread = thread.start_new_thread(serialPumpTask, (usbdevice, baud))
	if kbdpassthru:
		kbd_pump_thread = thread.start_new_thread(kbdPumpTask, (0,0))

	while True:
		while not netq.empty(): 
			handleNetworkInput(netq.get())
			netq.task_done()

		while not serialq.empty():
			handleSerialInput(serialq.get())
			serialq.task_done()

		time.sleep(0.1)