ARDUINO_UNIX_PATH ?= ../../ArduinoUnix

include $(ARDUINO_UNIX_PATH)/Makefile.mk

CFLAGS := -DUNIX_BUILD -pthread -I$(ARDUINO_UNIX_INCLUDE_PATH)
LDFLAGS := -lstdc++ -pthread
SOURCES := $(wildcard *.cpp) $(ARDUINO_UNIX_SOURCES)
BINARY := bin/bitlash
PREFIX := /usr/local
BINPATH := $(PREFIX)/bin

all:
	gcc $(LDFLAGS) $(CFLAGS) $(SOURCES) -o $(BINARY)

install:
	sudo cp $(BINARY) $(BINPATH)
