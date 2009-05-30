# Name: Makefile
# Project: hid-custom-rq example
# Author: Christian Starkjohann
# Creation Date: 2008-04-07
# Tabsize: 4
# Copyright: (c) 2008 by OBJECTIVE DEVELOPMENT Software GmbH
# License: GNU GPL v2 (see License.txt), GNU GPL v3 or proprietary (CommercialLicense.txt)
# This Revision: $Id: Makefile 692 2008-11-07 15:07:40Z cs $

#DEVICE = atmega168
DEVICE = attiny85

F_CPU   = 16500000

# Tiny85 fuses
FUSE_L  = 0xe1
FUSE_H  = 0xdd
#AVRDUDE = avrdude -c usbasp -p $(DEVICE) # edit this line for your programmer
AVRDUDE = avrdude -c usbtiny -p $(DEVICE)

CFLAGS  = -Iusbdrv -I. -DDEBUG_LEVEL=0

#---------------- Compiler Options C ----------------
#  -g*:          generate debugging information
#  -O*:          optimization level
#  -f...:        tuning, see GCC manual and avr-libc documentation
#  -Wall...:     warning level
#  -Wa,...:      tell GCC to pass this to the assembler.
#    -adhlns...: create assembler listing
#CFLAGS += -g$(DEBUG)
#CFLAGS += $(CDEFS)
#CFLAGS += -O$(OPT)
CFLAGS += -funsigned-char
#CFLAGS += -funsigned-bitfields
CFLAGS += -ffunction-sections
#CFLAGS += -fpack-struct
#CFLAGS += -fshort-enums
CFLAGS += -finline-limit=20
#
#CFLAGS += -mcall-prologues
#CFLAGS += -fdata-sections
#CFLAGS += -mshort-calls
#CFLAGS += -mno-tablejump
#CFLAGS += -fno-delete-null-pointer-checks
#
#CFLAGS += -combine
#CFLAGS += -fwhole-program
#
#CFLAGS += -Wall
#CFLAGS += -Wa,-adhlns
#CFLAGS += -Wstrict-prototypes
#CFLAGS += -Wundef
#CFLAGS += -fno-unit-at-a-time
#CFLAGS += -Wunreachable-code
#CFLAGS += -Wsign-compare
#CFLAGS += $(patsubst %,-I%,$(EXTRAINCDIRS))
#CFLAGS += $(CSTANDARD)




COMPILE = avr-gcc -Wall -Os -DF_CPU=$(F_CPU) $(CFLAGS) -mmcu=$(DEVICE)

# ATTiny85 fuse calculation for internal 16.5mhz pll clock:
#
# Fuse high byte:
# 0xdd = 1 1 0 1   1 1 0 1
#        ^ ^ ^ ^   ^ \-+-/ 
#        | | | |   |   +------ BODLEVEL 2..0 (brownout trigger level -> 2.7V)
#        | | | |   +---------- EESAVE (preserve EEPROM on Chip Erase -> not preserved)
#        | | | +-------------- WDTON (watchdog timer always on -> disable)
#        | | +---------------- SPIEN (enable serial programming -> enabled)
#        | +------------------ DWEN (debug wire enable)
#        +-------------------- RSTDISBL (disable external reset -> enabled)
#
# Fuse low byte:
# 0xe1 = 1 1 1 0   0 0 0 1
#        ^ ^ \+/   \--+--/
#        | |  |       +------- CKSEL 3..0 (clock selection -> HF PLL)
#        | |  +--------------- SUT 1..0 (BOD enabled, fast rising power)
#        | +------------------ CKOUT (clock output on CKOUT pin -> disabled)
#        +-------------------- CKDIV8 (divide clock by 8 -> don't divide)



##############################################################################
# Fuse values for particular devices
##############################################################################
# If your device is not listed here, go to
# http://palmavr.sourceforge.net/cgi-bin/fc.cgi
# and choose options for external crystal clock and no clock divider
#
################################## ATMega8 ##################################
# ATMega8 FUSE_L (Fuse low byte):
# 0x9f = 1 0 0 1   1 1 1 1
#        ^ ^ \ /   \--+--/
#        | |  |       +------- CKSEL 3..0 (external >8M crystal)
#        | |  +--------------- SUT 1..0 (crystal osc, BOD enabled)
#        | +------------------ BODEN (BrownOut Detector enabled)
#        +-------------------- BODLEVEL (2.7V)
# ATMega8 FUSE_H (Fuse high byte):
# 0xc9 = 1 1 0 0   1 0 0 1 <-- BOOTRST (boot reset vector at 0x0000)
#        ^ ^ ^ ^   ^ ^ ^------ BOOTSZ0
#        | | | |   | +-------- BOOTSZ1
#        | | | |   + --------- EESAVE (don't preserve EEPROM over chip erase)
#        | | | +-------------- CKOPT (full output swing)
#        | | +---------------- SPIEN (allow serial programming)
#        | +------------------ WDTON (WDT not always on)
#        +-------------------- RSTDISBL (reset pin is enabled)
#
############################## ATMega48/88/168 ##############################
# ATMega*8 FUSE_L (Fuse low byte):
# 0xdf = 1 1 0 1   1 1 1 1
#        ^ ^ \ /   \--+--/
#        | |  |       +------- CKSEL 3..0 (external >8M crystal)
#        | |  +--------------- SUT 1..0 (crystal osc, BOD enabled)
#        | +------------------ CKOUT (if 0: Clock output enabled)
#        +-------------------- CKDIV8 (if 0: divide by 8)
# ATMega*8 FUSE_H (Fuse high byte):
# 0xde = 1 1 0 1   1 1 1 0
#        ^ ^ ^ ^   ^ \-+-/
#        | | | |   |   +------ BODLEVEL 0..2 (110 = 1.8 V)
#        | | | |   + --------- EESAVE (preserve EEPROM over chip erase)
#        | | | +-------------- WDTON (if 0: watchdog always on)
#        | | +---------------- SPIEN (allow serial programming)
#        | +------------------ DWEN (debug wire enable)
#        +-------------------- RSTDISBL (reset pin is enabled)
#
############################## ATTiny25/45/85 ###############################
# ATMega*5 FUSE_L (Fuse low byte):
# 0xef = 1 1 1 0   1 1 1 1
#        ^ ^ \+/   \--+--/
#        | |  |       +------- CKSEL 3..0 (clock selection -> crystal @ 12 MHz)
#        | |  +--------------- SUT 1..0 (BOD enabled, fast rising power)
#        | +------------------ CKOUT (clock output on CKOUT pin -> disabled)
#        +-------------------- CKDIV8 (divide clock by 8 -> don't divide)
# ATMega*5 FUSE_H (Fuse high byte):
# 0xdd = 1 1 0 1   1 1 0 1
#        ^ ^ ^ ^   ^ \-+-/ 
#        | | | |   |   +------ BODLEVEL 2..0 (brownout trigger level -> 2.7V)
#        | | | |   +---------- EESAVE (preserve EEPROM on Chip Erase -> not preserved)
#        | | | +-------------- WDTON (watchdog timer always on -> disable)
#        | | +---------------- SPIEN (enable serial programming -> enabled)
#        | +------------------ DWEN (debug wire enable)
#        +-------------------- RSTDISBL (disable external reset -> enabled)
#
################################ ATTiny2313 #################################
# ATTiny2313 FUSE_L (Fuse low byte):
# 0xef = 1 1 1 0   1 1 1 1
#        ^ ^ \+/   \--+--/
#        | |  |       +------- CKSEL 3..0 (clock selection -> crystal @ 12 MHz)
#        | |  +--------------- SUT 1..0 (BOD enabled, fast rising power)
#        | +------------------ CKOUT (clock output on CKOUT pin -> disabled)
#        +-------------------- CKDIV8 (divide clock by 8 -> don't divide)
# ATTiny2313 FUSE_H (Fuse high byte):
# 0xdb = 1 1 0 1   1 0 1 1
#        ^ ^ ^ ^   \-+-/ ^
#        | | | |     |   +---- RSTDISBL (disable external reset -> enabled)
#        | | | |     +-------- BODLEVEL 2..0 (brownout trigger level -> 2.7V)
#        | | | +-------------- WDTON (watchdog timer always on -> disable)
#        | | +---------------- SPIEN (enable serial programming -> enabled)
#        | +------------------ EESAVE (preserve EEPROM on Chip Erase -> not preserved)
#        +-------------------- DWEN (debug wire enable)


# symbolic targets:
help:
	@echo "This Makefile has no default rule. Use one of the following:"
	@echo "make hex ....... to build main.hex"
	@echo "make program ... to flash fuses and firmware"
	@echo "make fuse ...... to flash the fuses"
	@echo "make flash ..... to flash the firmware (use this on metaboard)"
	@echo "make clean ..... to delete objects and hex file"

hex: main.hex

program: flash fuse eeprom

# rule for programming fuse bits:
fuse:
	@[ "$(FUSE_H)" != "" -a "$(FUSE_L)" != "" ] || \
		{ echo "*** Edit Makefile and choose values for FUSE_L and FUSE_H!"; exit 1; }
	$(AVRDUDE) -U hfuse:w:$(FUSE_H):m -U lfuse:w:$(FUSE_L):m

# rule for uploading firmware:
flash: main.hex
	$(AVRDUDE) -U flash:w:main.hex:i
	
eeprom: main.eep.hex
	$(AVRDUDE) -U eeprom:w:main.eep.hex:i

# rule for deleting dependent files (those which can be built by Make):
clean:
	rm -f main.hex main.lst main.obj main.cof main.list main.map main.eep.hex main.elf *.o usbdrv/*.o main.s usbdrv/oddebug.s usbdrv/usbdrv.s *.lst

# Generic rule for compiling C files:
.c.o:
	$(COMPILE) -c $< -o $@

# Generic rule for assembling Assembler source files:
.S.o:
	$(COMPILE) -c $< -o $@
#	$(COMPILE) -x assembler-with-cpp -c $< -o $@
# "-x assembler-with-cpp" should not be necessary since this is the default
# file type for the .S (with capital S) extension. However, upper case
# characters are not always preserved on Windows. To ensure WinAVR
# compatibility define the file type manually.

# Generic rule for compiling C to assembler, used for debugging only.
.c.s:
	$(COMPILE) -S $< -o $@

# file targets:

# Since we don't want to ship the driver multipe times, we copy it into this project:
usbdrv:
	cp -r ../../../usbdrv .


#OBJECTS = main.o bitlash.o eeprom.o usbdrv/usbdrv.o usbdrv/usbdrvasm.o usbdrv/oddebug.o
#OBJECTS = main.o usbdrv/usbdrv.o eeprom.o usbdrv/usbdrvasm.o bitlash.o
#OBJECTS = main.o usbdrv/usbdrv.o eeprom.o bitlash.o usbdrv/usbdrvasm.o
#OBJECTS =  usbdrv/usbdrv.o usbdrv/usbdrvasm.o \

OBJECTS = main.o usbdrv/usbdrvasm.o \
	bitlash-cmdline.o \
	bitlash.o \
	bitlash-program.o \
	bitlash-eh1.o \
	bitlash-taskmgr.o \
	bitlash-parser.o \
	bitlash-interpreter.o \
	eeprom.o \
	bitlash-functions.o \
	bitlash-arduino.o \
	bitlash-serial.o \
	bitlash-eeprom.o \
	bitlash-error.o \



#---------------- Linker Options ----------------
#  -Wl,...:     tell GCC to pass this to linker.
#    -Map:      create map file
#    --cref:    add cross reference to  map file
#LDFLAGS = -lm
LDFLAGS += -Wl,-Map=main.map,--cref
LDFLAGS += -Wl,--relax 
LDFLAGS += -Wl,--gc-sections
#LDFLAGS += -lc
#LDFLAGS += -lm


#	$(COMPILE) -o main.elf usbdrv/usbdrv.o usbdrv/usbdrvasm.o main.o \


#main.elf: usbdrv $(OBJECTS)	# usbdrv dependency only needed because we copy it

main.elf: $(OBJECTS)
	$(COMPILE) -o main.elf usbdrv/usbdrvasm.o main.o \
	bitlash-arduino.o \
	bitlash.o -lc -lm \
	bitlash-program.o \
	bitlash-functions.o -lc -lm \
	bitlash-eh1.o \
	bitlash-error.o -lc -lm \
	bitlash-cmdline.o \
	bitlash-taskmgr.o \
	bitlash-parser.o \
	bitlash-interpreter.o \
	eeprom.o \
	bitlash-serial.o \
	bitlash-eeprom.o \
 $(LDFLAGS)

#	$(COMPILE) -o main.elf $(OBJECTS) $(LDFLAGS)



HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0 --no-change-warnings



main.hex: main.elf
	rm -f main.hex main.eep.hex
	avr-objcopy -j .text -j .data -O ihex main.elf main.hex
	avr-objcopy $(HEX_EEPROM_FLAGS) -O ihex main.elf main.eep.hex
	avr-size main.hex main.eep.hex
	avr-objdump -S $< > main.lss
	avr-readelf -x .eeprom main.elf


# debugging targets:

disasm:	main.elf
	avr-objdump -d main.elf

cpp:
	$(COMPILE) -E main.c
