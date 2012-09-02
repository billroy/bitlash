/***
	radio-rfm22.c:	Bitlash Radio Interface for HopeRF RFM22

	Copyright (C) 2008-2012 Bill Roy

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:
	
	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.
	
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
***/

#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif
#include "bitlash.h"
#include "../../libraries/bitlash/src/bitlash.h"
#include "bitlash_rf.h"
#include "pkt.h"

#if defined(RADIO_RFM22)		// GUARDS THIS WHOLE FILE

////////////////////////////////////
// Turn this on to enable debug spew
//#define RADIO_DEBUG


// Radio Pin Assignments
//
// These pin assignments enable use of hardware SPI on the radio interface
//
#define L01_PORT		PORTB
#define L01_PORT_PIN	PINB
#define L01_PORT_DD		DDRB
#define L01_IRQ_PORT	PIND

#define L01_CSN	2 	// Output B2 is d10
#define L01_SCK	5 	// Output B5 is d13
#define MOSI	3 	// Output B3 is d11
#define MISO	4 	// Input  B4 is d12

#define L01_CE	0 	// Output B0 is d8
#define RF_IRQ	2 	// Input on D2 (PORTD2) which is d2

// radio primitives
//
#define sbi(a,b) (a|=(1<<b))
#define cbi(a,b) (a&=~(1<<b))

#define rf_begin()		cbi(L01_PORT, L01_CSN)
#define rf_end()		sbi(L01_PORT, L01_CSN)

#define rf_disable()	sbi(L01_PORT, L01_CE)
#define rf_enable()		cbi(L01_PORT, L01_CE)

#define rf_interrupt()	(!(L01_IRQ_PORT & (1<<RF_IRQ)))
byte rx_pkt_ready(void) { return rf_interrupt(); }



//////////////////////
//	led control: RF activity LED, by default pin 7
//
//	redefine LED_RF to move the activity light
//	undefine LED_RF to omit the activity light feature
// 	(saves 128 bytes!)
//
#define LED_RF	7

#ifdef LED_RF
#define led_on() digitalWrite(LED_RF,1)
#define led_off() digitalWrite(LED_RF,0)
void init_leds(void) {
	pinMode(LED_RF, OUTPUT);
	led_on();
	delay(200);
	led_off();
}
#else
#define led_on()
#define led_off()
#define init_leds()
#endif



// forward decls
byte rf_read_register(uint8_t);
void rf_set_register(uint8_t, uint8_t);


// RFM-22 Radio Module Register Definitions
// from "RFM22 ISM Transceiver Module" v1.1
// via http://hoperf.com
//
// rfm22 home page: http://www.hoperf.com/rf_fsk/rfm22.htm
// datasheet: http://www.hoperf.com/upfile/RFM22.PDF

#define REG_DEVICE_TYPE 	0x00
#define DEVICE_TYPE_RXTX 	0x08
#define DEVICE_TYPE_TX		0x07

#define REG_VERSION_CODE 	0x01
#define DEVICE_RFM22_VER_2 	2		// first prototype units
#define DEVICE_RFM22B		6		// upgraded RFM22B units

#define REG_DEVICE_STATUS 0x02
#define FFOVFL	7
#define FFUNFL	6
#define RXFFEM	5
#define HEADERR	4
#define FREQERR	3
#define LOCKDET 2
#define CPSTATE	0

#define REG_INTERRUPT_STATUS_1 0x03
#define IFERR 		7
#define ITXFFAFULL	6
#define ITXFFAEM	5
#define	IRXAFULL	4
#define IEXT		3
#define IPKSENT		2
#define IPKVALID	1
#define ICRERROR	0

#define tx_pkt_sent() (rf_read_register(REG_INTERRUPT_STATUS_1) & (1<<IPKSENT))
#define rx_pkt_valid() (rf_read_register(REG_INTERRUPT_STATUS_1) & (1<<IPKVALID))

#define REG_INTERRUPT_STATUS_2 0x04
#define ISWDET		7
#define IPREAVAL	6
#define IPREAINVAL	5
#define IRSSI		4
#define IWUT		3
#define ILBD		2
#define ICHIPRDY	1
#define IPOR		0

//////////
// rf_read_status: read the interrupt status registers
// reading clears any pending interrupts so we save the status in globals
//
byte rfstat1, rfstat2;
//
byte rf_read_status(void) {
	rfstat1 = rf_read_register(REG_INTERRUPT_STATUS_1);
	rfstat2 = rf_read_register(REG_INTERRUPT_STATUS_2);
	return rfstat1;
}

#define REG_INTERRUPT_ENABLE_1 0x05
#define ENFFERR		7
#define ENTXFAFULL	6
#define ENTXFFAEM	5
#define ENRXAFULL	4
#define ENEXT		3
#define ENPKSENT	2
#define ENPKVALID	1
#define ENCRCERROR	0

#define enable_tx_interrupt() 

#define REG_INTERRUPT_ENABLE_2 0x06
#define ENSWDET		7
#define ENPREAVAL	6
#define ENPREAINVAL	5
#define ENRSSI		4
#define ENWUT		3
#define ENLBDI		2		// renamed ENLBDI to avoid conflict with ENLBD
#define ENCHIPRDY	1
#define ENPOR		0


#define REG_OP_MODE_1 0x07
#define SWRES	7
#define ENLBD	6
#define ENWT	5
#define X32KSEL	4
#define TXON	3
#define RXON	2
#define PLLON	1		// TUNE mode
#define XTON	0		// READY mode

#define REG_MCLK_OUTPUT		0x0a
#define MCLK_10MHZ			2

#define REG_GPIO0_CONFIG	0x0b
#define REG_GPIO1_CONFIG	0x0c
#define REG_GPIO2_CONFIG	0x0d
#define GPIODRV 6
#define PUP		5


#define REG_OP_MODE_2 0x08
//#define ANTDIV 	5
//#define RXMPK 	4
//#define AUTOTX	3
//#define ENIDM	2
#define FFCLRRX	1
//#define FFCLRTX 0




/////////////////////////////////////////////////////////////////
//
//	SPI Radio Interface
//
/////////////////////////////////////////////////////////////////

#define spi_ready() (SPSR & (1<<SPIF))

uint8_t spi_write(uint8_t outgoing) {
uint8_t status;

#ifdef RADIO_DEBUG
    Serial.print("spi:");
    Serial.print(outgoing,HEX);
    Serial.print("<");
#endif

	SPDR = outgoing;
	// TODO: a spin counter here might produce interesting amounts of entropy
	while (!spi_ready()) {;}
	status = SPDR;

#ifdef RADIO_DEBUG
	Serial.println(status, HEX);
#endif

	return status;	
}


// Send a command-of-one-argument, return status
//
uint8_t send_command(uint8_t cmd, uint8_t data) {
uint8_t status;

	// "Every new command must be started by a high to low transition on CSN"
	rf_begin();
	spi_write(cmd);
	status = spi_write(data);
	rf_end();

#ifdef RADIO_DEBUG
	Serial.print("rf: ");
	Serial.print(cmd, HEX);
	Serial.print(",");
	Serial.print(data, HEX);
	Serial.print(" -> ");
	Serial.println(status, HEX);
#endif

	return(status);
}


// Send a one byte command (no args), return status
//
uint8_t send_command_noarg(uint8_t cmd) {
uint8_t status;
	rf_begin(); //Select chip
	status = spi_write(cmd);
	rf_end(); //Deselect chip

#ifdef RADIO_DEBUG
	Serial.print("rf: ");
	Serial.print(cmd, HEX);
	Serial.print(" -> ");
	Serial.println(status, HEX);
#endif

	return(status);
}


// Sends a number of bytes of payload
//
void tx_send_payload(uint8_t cmd, uint8_t bytes, uint8_t *data) {

#ifdef RADIO_DEBUG
	Serial.println("tx_send_payload");
#endif

	rf_begin();
	spi_write(cmd);
	while (bytes--) spi_write(*data++);
	rf_end();
}

// Read incoming data
//
void rx_get_pkt(uint8_t cmd, uint8_t nbytes, pkt_t *pkt) {
uint8_t *buf = (uint8_t *) pkt;

#ifdef RADIO_DEBUG
	Serial.println("rx_get_packet");
#endif

	rf_begin();
	spi_write(cmd);
	while (nbytes--) *buf++ = spi_write(0);
	rf_end();
}


////////////////////
//
//	Radio State Management
//
//
//////////
// set_rx_mode: turn on the packet receiver and enable the packet-ready interrupt
//
void set_rx_mode() {

	// clear the rx FIFO on entry to rx mode
	rf_set_register(REG_OP_MODE_2, 1<<FFCLRRX);
	rf_set_register(REG_OP_MODE_2, 0);

	rf_set_register(REG_INTERRUPT_ENABLE_1, (1<<ENPKVALID));
	rf_set_register(REG_OP_MODE_1, (1<<RXON | 1<<XTON));
}


//////////
// tx_start: initiate transmission of the packet in the fifo
//	set up for transmit-done interrupt, but don't wait
//
void tx_start() {
	rf_set_register(REG_INTERRUPT_ENABLE_1, (1<<ENPKSENT));
	rf_set_register(REG_OP_MODE_1, (1<<TXON | 1<<XTON));
}


//#define clear_tx_fifo() rf_set_register(REG_OP_MODE_2, 1<<FFCLRTX);rf_set_register(REG_OP_MODE_2, 0)

#define REG_IF_FILTER_BANDWIDTH 0x1c			// 0x01
#define DWN3_BYPASS	7
#define NDEC_EXP	4
#define FILSET		0

#define REG_CLOCK_RECOVERY_OVERSAMPLING_RATE 0x20		// 0x64

#define REG_CLOCK_RECOVERY_OFFSET_2	0x21		// 0x01
#define RXOSR_8_10	5
#define STALLCTRL	4
#define NCOFF_16_19	0
#define REG_CLOCK_RECOVERY_OFFSET_1 0x22		// 0x47
#define REG_CLOCK_RECOVERY_OFFSET_0 0x23		// 0xae
#define REG_CLOCK_RECOVERY_GAIN_1 0x24			// 0x02	DOCBUG: p.44 says crgain default is 0x521
#define REG_CLOCK_RECOVERY_GAIN_0 0x25			// 0x8f DOCBUG: p.44 says crgain default is 0x521

#define REG_RSSI 0x26

#define REG_DATA_ACCESS_CONTROL 0x30

#define ENPACRX 	7
#define LSBFRST		6
#define CDCDONLY	5
#define ENPACTX		3
#define ENCRC		2
#define CRCTYPE		0

// values for crctype
#define CRC_CCITT 	0
#define CRC_CRC16	1
#define CRC_IEC16	2
#define CRC_BIACH	3


#define REG_HEADER_CONTROL_1 0x32
#define BCEN 4
#define HDCH 0

#define REG_HEADER_CONTROL_2 0x33
#define HDLEN 		4
#define FIXPKLEN 	3
#define SYNCLEN 	1
#define PREALEN8 	0

#define REG_PREAMBLE_LENGTH 0x34	// default is 00010000 = 8 nibbles / 32 bits

#define REG_PREAMBLE_DETECTION_CONTROL_1 0x35	// sample default 0x20 -> 4 nibbles / 16 bits
#define PREATH	3

#define REG_TX_ADDR 0x3a

#define REG_PACKET_LENGTH 0x3e
#define set_tx_packet_length(nbytes) rf_set_register(REG_PACKET_LENGTH, nbytes)

#define REG_RX_ADDR 0x3f
#define REG_RECEIVED_PACKET_LENGTH 0x4b

#define REG_TX_DATA_RATE_HI 0x6e
#define REG_TX_DATA_RATE_LO 0x6f

#define REG_MODULATION_MODE_CONTROL_2 0x71
#define TRCLK	6
#define DTMOD	4
#define ENINV	3
#define FD_8	2
#define MODTYP	0

// values for dtmod
#define MOD_FIFO 2
//#define MOD_PN9	3

// values for modtyp
//#define MOD_NOT 0
#define MOD_OOK 1
#define MOD_FSK 2
// #define MOD_GFSK 3

#define REG_FREQUENCY_BAND_SELECT 0x75
#define SBSEL	6
#define HBSEL	5
#define REG_NOMINAL_CARRIER_FREQUENCY_HI 0x76
#define REG_NOMINAL_CARRIER_FREQUENCY_LO 0x77

#define REG_FREQUENCY_DEVIATION 0x72

#define REG_FIFO 0x7f


//////////////////////
// rf_get_register: read n bytes from device starting at reg into buf
//
void rf_get_register(uint8_t reg, uint8_t nbytes, uint8_t *buf) {
	rx_get_pkt(reg, nbytes, (pkt_t *)buf);
}


//////////////////////
// rf_read_register: return 1-byte value from device reg
//
byte rf_read_register(uint8_t reg) {
	return send_command(reg, 0);
}


//////////////////////
// rf_set_register: set reg=value
//
void rf_set_register(uint8_t reg, uint8_t value) {
	send_command(0x80 + reg, value);
}


//////////
// func_rfget: function handler for Bitlash rfget(reg)
//
numvar func_rfget(void) {
numvar ret = 0;
	rf_get_register(getarg(1), 1, (uint8_t *) &ret);
	return ret;
}

//////////
// func_rfset: function handler for bitlash rfset(reg, value)
numvar func_rfset(void) {
	rf_set_register(getarg(1), getarg(2));
	return 0;
}


//////////
//	func_setfreq: set base operating frequency
//
numvar func_setfreq(void) {
numvar freq = getarg(1);
numvar offset;
int band, fc;		// band is fb in the rfm22 doc
byte hbsel;

	if (freq < 240000000L) freq = 240000000L;
	if (freq > 929999999L) freq = 929999999L;
	if (freq < 480000000L) {
		hbsel = 0;
		band = (freq - 240000000L) / 10000000L;
		offset = freq - (10000000L * (band + 24));
		//fc = offset * 64L / 10000L;
		fc = offset * 4L / 625L;
	}
	else {
		hbsel = 1;
		band = (freq - 480000000L) / 20000000L;
		offset = freq - (20000000L * (band + 24));
		//	fc = offset * 64L / 20000L;
		fc = offset * 4L / 1250L;
	}

	//printHex(hbsel); speol();
	//printHex(band); speol();
	//printInteger(offset,0); speol();
	//printHex(fc); speol();

	rf_set_register(REG_FREQUENCY_BAND_SELECT, band | (1<<SBSEL) | (hbsel<<HBSEL));
	rf_set_register(REG_NOMINAL_CARRIER_FREQUENCY_HI, fc >> 8);
	rf_set_register(REG_NOMINAL_CARRIER_FREQUENCY_LO, fc);
	return 0;
}


//////////
// func_degf: return temperature in degrees fahrenheit
// See p.55 in the data sheet
//
#define REG_ADC_CONFIG 0xf
#define ADCSTART 	7
#define ADCDONE 	7

#define REG_TEMP_SENSOR_CONTROL 0x12
#define TSRANGE 	6	
#define ENTSOFFS	5
#define	ENTSTRIM	4

#define REG_TEMP_SENSOR_VALUE_OFFSET 0x13
numvar func_degf(void) {	
	rf_set_register(REG_ADC_CONFIG, 0);
	rf_set_register(REG_TEMP_SENSOR_CONTROL, (3<<TSRANGE | 1<<ENTSOFFS | 1<<ENTSTRIM));
	rf_set_register(REG_ADC_CONFIG, 1<<ADCSTART);
	while (!(rf_read_register(REG_ADC_CONFIG) & (1<<ADCDONE))) {;}
	return rf_read_register(0x11) - 40;
}



//////////////////////
// rx_fetch_pkt: read packet into buffer if available
//
//	returns number of bytes transferred to pkt, 0 if no data available
//
byte rx_fetch_pkt(pkt_t *pkt) {

	// Does the radio have business for us?
	if (!rx_pkt_ready()) return 0;		// quick out if our packet interrupt hasn't fired

	if (!(rf_read_status() & (1<<IPKVALID))) {
#ifdef RADIO_DEBUG
		sp("rx: int no pkt!");
#endif
		return 0;
	}

#ifdef RADIO_DEBUG
	sp("rx_ got _packet");
#endif

	led_on();

	byte length = rf_read_register(REG_RECEIVED_PACKET_LENGTH);

	// If the packet length is bigger than our buffer We Have A Situation.
	// Clip to our max packet size; set_rx_mode() will clear the fifo,
	// silently discarding the excess.
	//
	if (length > RF_PACKET_SIZE) length = RF_PACKET_SIZE;

	rx_get_pkt(REG_FIFO, length, pkt);
	rx_packet_count++;						// got one? count it
	set_rx_mode();
	led_off();

	log_packet('R', pkt, length);

	return length;
}


//////////////////////
// tx_send_packet: transmit a data packet
//
//	length: length of raw packet including header
//	pkt:	pointer to packet
//
void tx_send_pkt(pkt_t *pkt, uint8_t length) {

	log_packet('T', pkt, length);

	led_on();
	set_tx_packet_length(length);
	tx_send_payload(0x80 + REG_FIFO, length, (uint8_t *)pkt); 	// Clock out payload

	// Set TXON bit to start the tx cycle
	tx_start();
	led_off(); led_on();	// toggle to note time to here
	tx_packet_count++;

	// wait for the packet transmission to complete
	// per p.81 TXON is "Automatically cleared in FIFO mode once the packet is sent"
	// we spin for about 10ms here...
	while (!rf_interrupt()) {;}
	rf_read_status();

	// TODO: consider a better place to restore
	// this one wastes time going back into rx mode during long prints
	set_rx_mode();
	led_off();
}




////////////////////////////////////////
//
// Address handling
//
// The RFM22 packet engine has a built-in 4-byte header match feature we use for node id.
//
// On the receive side:
// We use the simplest case, where incoming packets are accepted if all four bytes 
// of incoming header match our node id.
// We also engage "broadcast match" to accept all broadcast packets, which are addressed
// to the all-ones broadcast address ("\xff\xff\xff\xff")
//
// 	[rprintf()] and [tell("*","...")] transmit to the all-ones broadcast address
//
#define RF_ADDRESS_LENGTH 4

char nodeid[RF_ADDRESS_LENGTH];

#define DEFAULT_RX_ADDRESS "noob"
#define BROADCAST_ADDRESS "\xff\xff\xff\xff"

// Set transmit or receive address
//
// whichaddr must be REG_TX_ADDR or REG_RX_ADDR to set tx or rx address
// rf_address points to a buffer of length RF_ADDRESS_LENGTH with the address
//
void rf_put_address(byte whichaddr, byte *rf_address) {

	// RFM22 Broadcast Address handling
	// 	If we see a null or zero-byte address here, point it to the BROADCAST_ADDRESS
	//	Same for "*" as the address.
	//
	if (!rf_address || !(*rf_address) || !strcmp((char *) rf_address, "*")) {
		rf_address = (byte *) BROADCAST_ADDRESS;
	}

	rf_set_register(whichaddr+0, rf_address[0]);
	rf_set_register(whichaddr+1, rf_address[1]);
	rf_set_register(whichaddr+2, rf_address[2]);
	rf_set_register(whichaddr+3, rf_address[3]);
}

void rf_set_rx_address(char *my_address) {

	// stash the nodeid for init_radio
	strncpy(nodeid, my_address, RF_ADDRESS_LENGTH);		// trailing pads with zeroes

	if (radio_go) rf_put_address(REG_RX_ADDR, (byte *) my_address);
}

void rf_set_tx_address(char *to_address) {
	rf_put_address(REG_TX_ADDR, (byte *) to_address);
}



/////////////////////////////////////
// initialize the radio interface
//
// stick with me, this goes on for a while
// "may you live with interesting radios"
//
void init_radio(void) {

	init_leds();

	// Add Bitlash functions specific to this radio
	//	addBitlashFunction("degf", (bitlash_function) func_degf);
	addBitlashFunction("freq", (bitlash_function) func_setfreq);
	addBitlashFunction("rfget", (bitlash_function) func_rfget);
	addBitlashFunction("rfset", (bitlash_function) func_rfset);


	// Set output mode for outputs
	// TODO: Ensure SS is set to output if L01_CSN moves off SS!
	L01_PORT_DD |= (1<<L01_CE | 1<<L01_CSN | 1<<MOSI | 1<<L01_SCK);

	// Set MISO as an input
	L01_PORT_DD &= ~(1<<MISO);
	L01_PORT &= ~(1<<MISO);		// turn off the internal pullup

	rf_end();		// Set -CSN high to deselect radio

	// Initialize SPI interface to the radio
#if F_CPU == 8000000
	// Enable SPI; be the Master; SPI mode 0; set up for 8/2 prescale for 4 MHz clock
	SPCR = 0<<SPIE | 1<<SPE | 1<<MSTR | 0<<CPOL | 0<<CPHA | 0<<SPR1 | 0<<SPR0;
	SPSR = 1<<SPI2X;		// set 2x mode to make /4 into /2, so 4 MHz

#elif F_CPU == 16000000
	//// Enable SPI; be the Master; SPI mode 0; set up for 16/4 prescale for 4 MHz clock
	SPCR = 0<<SPIE | 1<<SPE | 1<<MSTR | 0<<CPOL | 0<<CPHA | 0<<SPR1 | 0<<SPR0; 
	SPSR = 0<<SPI2X;		// 16 MHz does _not_ need 2X mode

#else
	unsupported F_CPU
#endif

	// clear the status and data registers
	int junk = SPSR; 
	junk = SPDR;

	// For RFM22: set CE low to enable radio  (it's ~STBY)
	rf_enable();

	// Got radio?
	for (;;) {
		rf_set_register(REG_OP_MODE_1, 1<<SWRES);		// Reset the radio
		delay(1000);

		// Verify we have a radio, and it is the right type
		byte device_type = rf_read_register(REG_DEVICE_TYPE);
		byte device_version = rf_read_register(REG_VERSION_CODE);

		//printHex(device_type); speol();
		//printHex(device_version); speol();

		if ((device_type == DEVICE_TYPE_RXTX) &&
			((device_version == DEVICE_RFM22_VER_2) || (device_version == DEVICE_RFM22B))) {
			sp("RFM22 go!"); speol();
			break;
		} else {
			sp("No radio."); speol();
		}
	}

	//
	// Set up radio registers for transmit and receive
	//

	// Disable ENPOR and ENCHIPRDY interrupt sources
	// which are enabled by default
	rf_set_register(REG_INTERRUPT_ENABLE_2, 0);

	//set_ready_mode();

	// Configure GPIO0-1 for tx-rx antenna switching
	rf_set_register(REG_GPIO0_CONFIG, (3<<GPIODRV | 1<<PUP | 0x12));	// GPIO0 is TX-State -> TX-Ant control
	rf_set_register(REG_GPIO1_CONFIG, (3<<GPIODRV | 1<<PUP | 0x15));	// GPIO1 is RX-State -> RX-Ant control

	// Configure GPIO2 for 10MHz system clock output
	rf_set_register(REG_MCLK_OUTPUT, MCLK_10MHZ);

	// Data Access Control setup
	// default for REG_DATA_ACCESS_CONTROL is fine: use the packet engine & crc checks
	//	rf_set_register(REG_DATA_ACCESS_CONTROL, (1<<ENPACRX | 1<<ENPACTX | 1<<ENCRC | CRC_CRC16);

	// Frequency setup
	//
	// For North America, the default frequency band "fb" is 0x15 or 21
	// so 21 * 20 MHz = 420 MHz offset + 480 Mhz base = 900 MHz band
	//
	// the default carrier frequency setting "fc" is 0xBB80 = 48000
	//		48000/64000 = 0.75 * 2 * 10000 = 15000
	// giving 900 + 15 = 915 MHz as default frequency
	//
	// TODO: ignoring frequency offset registers for the moment
	//
	// This is equivalent to the default setting:
	//
	// rf_set_register(REG_FREQUENCY_BAND_SELECT, (1<<SBSEL | 1<<HBSEL | 0x15));
	// rf_set_register(REG_NOMINAL_CARRIER_FREQUENCY_HI, 0xbb);
	// rf_set_register(REG_NOMINAL_CARRIER_FREQUENCY_LO, 0x80);
	// or: rfset(0x75,0x75); rfset(0x76,0xbb); rfset(0x77,0x80)
	//
	// Decoding the example from the sample transmit/receive code for 434 MHz
	// band select low band with offset of 19
	// 240 + 19*10 = 430
	//
	// the remaining 4 mhz is to be found in the r76/77 carrier center freq
	// 	0x6400 = 25600
	// 	25600/64000 = 0.4
	// 	0.4 * 10000 = 4000
	//
	// This is the equivalent code to set 434 MHz using our defines:
	//rf_set_register(REG_FREQUENCY_BAND_SELECT, (1<<SBSEL | 0<<HBSEL | 0x13));
	//rf_set_register(REG_NOMINAL_CARRIER_FREQUENCY_HI, 0x64);
	//rf_set_register(REG_NOMINAL_CARRIER_FREQUENCY_LO, 0x00);
	// or from the console: rfset(0x75,0x53); rfset(0x76,0x64); rfset(0x77,0x00)


	// TX Data Rate Setup
	//
	// this fragment from the example sets up for 4800 bps:
	//	spi_write(0x6e, 0x27); // Tx data rate 1
	//	spi_write(0x6f, 0x52); // Tx data rate 0
	//
	// for HBSEL=1, TX_DR = 1000 * txdr / 2^16
	// for HBSEL=0, TX_DR = 1000 * txdr / 2^21
	//
	// the default on this sample part is 0x0a3d => 39.9938 kbps, call it 40kbps
	//
	// inverting the formula:
	// 		value = (baud/1000)*(2^21)/1000	for baud <  30000
	// 		value = (baud/1000)*(2^16)/1000	for baud >= 30000
	//
	// these values are copied from the file doc/rfm-22-calculations.ods:
	//
	//		baud	divisor	divisor, hex
	//		1200	2516	09D4
	//		2400	5033	13A9
	//		4800	10066	2752
	//		9600	20132	4EA4
	//
	//		40000	2621	0A3D	<-- this is the default 6e/6f setting
	//		57600	3774	0EBE
	//		64000	4194	1062
	//		96000	6291	1893
	//		100000  6553    1999
	//		128000	8388	20C4
	//


// Configure the data rate
//
#define TX_DATA_RATE 40000
//#define TX_DATA_RATE 100000

#if TX_DATA_RATE == 40000

	// 40kbps is the factory setting;
	// take all the defaults but turn on FIFO and FSK
	rf_set_register(REG_MODULATION_MODE_CONTROL_2,
		(0<<TRCLK | MOD_FIFO<<DTMOD | 0<<ENINV | 0<<FD_8 | MOD_FSK<<MODTYP));

#elif  TX_DATA_RATE == 100000

	// Set data rate to 100000 bps
	// NOTE: don't forget to double check the TXDTRTSCALE bit in 0x70 if you set below 30k
	rf_set_register(REG_TX_DATA_RATE_HI, 0x19);
	rf_set_register(REG_TX_DATA_RATE_LO, 0x99);

	// Frequency Deviation setup
	//
	// value in increments of 625 Hz
	// default of 0x20 gives 0x20 = 32 * 625 Hz = 20 KHz
	// example uses 45k/625 = 72
	//

#define TX_DEVIATION 50000

#if TX_DEVIATION == 50000

	// for 100kbps, with deviation at 50 kHz = 50000/625 = 80 = 0x050
	// NOTE: high bit goes in REG_MODULATION_CONTROL_2 as FD_8
	rf_set_register(REG_FREQUENCY_DEVIATION, 0x50);

	// Set up for FIFO io and FSK modulation
	// NOTE: high bit of deviation is here!
	rf_set_register(REG_MODULATION_MODE_CONTROL_2, 
		(0<<TRCLK | MOD_FIFO<<DTMOD | 0<<ENINV | 0<<FD_8 | MOD_FSK<<MODTYP));

	// Set up RX Modem Configuration per Table 16
	//
	// For 100kbps/100kHz settings:
	rf_set_register(REG_IF_FILTER_BANDWIDTH, (1<<DWN3_BYPASS | 0<<NDEC_EXP | 0xf<<FILSET));
	rf_set_register(REG_CLOCK_RECOVERY_OVERSAMPLING_RATE, 0x78);
	rf_set_register(REG_CLOCK_RECOVERY_OFFSET_2, (0<<RXOSR_8_10 | 0<<STALLCTRL | 1<<NCOFF_16_19));
	rf_set_register(REG_CLOCK_RECOVERY_OFFSET_1,(0x11));
	rf_set_register(REG_CLOCK_RECOVERY_OFFSET_0,(0x11));
	rf_set_register(REG_CLOCK_RECOVERY_GAIN_1,(0x04));
	rf_set_register(REG_CLOCK_RECOVERY_GAIN_0,(0x46));

#elif TX_DEVIATION == 300000

	// for 100kbps, set deviation at 300 kHz = 300000/625 = 480 = 0x1e0
	// NOTE: high bit goes in REG_MODULATION_CONTROL_2 as FD_8
	rf_set_register(REG_FREQUENCY_DEVIATION, 0xe0);

	// Set up for FIFO io and FSK modulation
	// NOTE: high bit of deviation is here!
	rf_set_register(REG_MODULATION_MODE_CONTROL_2, 
		(0<<TRCLK | MOD_FIFO<<DTMOD | 0<<ENINV | 1<<FD_8 | MOD_FSK<<MODTYP));

	// Set up RX Modem Configuration per Table 16
	//
	// For 100kbps/300kHz settings:
	rf_set_register(REG_IF_FILTER_BANDWIDTH, (1<<DWN3_BYPASS | 0<<NDEC_EXP | 0xe<<FILSET));
	rf_set_register(REG_CLOCK_RECOVERY_OVERSAMPLING_RATE, 0x78);
	rf_set_register(REG_CLOCK_RECOVERY_OFFSET_2, (0<<RXOSR_8_10 | 0<<STALLCTRL | 1<<NCOFF_16_19));
	rf_set_register(REG_CLOCK_RECOVERY_OFFSET_1,(0x11));
	rf_set_register(REG_CLOCK_RECOVERY_OFFSET_0,(0x11));
	rf_set_register(REG_CLOCK_RECOVERY_GAIN_1,(0x00));
	rf_set_register(REG_CLOCK_RECOVERY_GAIN_0,(0xb8));
#else
	unsupported TX_DEVIATION
#endif

#else
	unsupported TX_DATA_RATE
#endif

	// Header Configuration
	//
	// Enable header checking and broadcast matching on all address bytes
	rf_set_register(REG_HEADER_CONTROL_1, (0xf<<BCEN | 0xf<<HDCH));

	// Set up for 4 byte header, variable packet length, 2 byte sync, 0 for MSB Preamble Length
	// 	(default is 0x22: HDLEN 010 Header 3 and 2; Sync Word 3 and 2)
	//
	rf_set_register(REG_HEADER_CONTROL_2, (4<<HDLEN | 0<<FIXPKLEN | 1<<SYNCLEN | 0<<PREALEN8));

	// Preamble Length
	// the default preamble length is 8 nibbles / 32 bits
	// the sample transmit / receive code provided by HopeRF uses 64 nibbles instead(!)
	// the table on page 43 recommends 40 bits (10 nibbles), so we use that here
	//
	// TODO: test with shorter preamble for performance
	//
	rf_set_register(REG_PREAMBLE_LENGTH, (40/4));

	// Preamble Detect Threshold
	//
	// DOCBUG: possible doc issue regarding preamble threshold:
	// 	rfget(0x35) is 0x20 at bootup; this is 4<<3, not 5<<3 per doc p.106
	//	(not to mention register 0x35 is missing from the summary table)
	// 	4 nibbles is 16 bits, but on page 42 it is recommended to use 20 bits
	// so we set PREATH to 20 bits here; that's 5 nibbles
	//
	rf_set_register(REG_PREAMBLE_DETECTION_CONTROL_1, (20/4)<<PREATH);

	// Sync Pattern and Length
	// we use the default sync pattern of 2dd4 with 2 byte check

	// Enable the radio so rf_set_rx_address is a pass-thru below
	radio_go = 1;		// mark the radio as UP

	// Receive address setup: listen on NOOB, and the broadcast address,
	// until setid() is called to provide an updated address.
	// If setid() was called before now, use the cached value.
	// (Tx address is set up per packet)
	//
	rf_set_rx_address((*nodeid) ? nodeid : (char *) DEFAULT_RX_ADDRESS);

	set_rx_mode();		// engage the receiver and off we go
}

#endif	 // defined(RADIO_RFM22)