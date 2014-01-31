CFLAGS := -DUNIX_BUILD -pthread
LDFLAGS := -lstdc++ -pthread
SOURCES := $(wildcard *.cpp)
BINARY := bin/bitlash
PREFIX := /usr/local
BINPATH := $(PREFIX)/bin

all:
	gcc -DUNIX_BUILD $(LDFLAGS) $(CFLAGS) $(SOURCES) -o $(BINARY)

install:
	sudo cp $(BINARY) $(BINPATH)
