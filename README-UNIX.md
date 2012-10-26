# Bitlash on Unix

Here are some notes on experimental Unix support for Bitlash.

## Status

- Built/tested on OS X and Ubuntu Linux
	- all the basic functions work
	- on OS X, numvar is 64 bits, so everything is 64 bits wide
	- Makefile in src/ will build the binary as src/bin/bitlash
	- precompiled binaries in src/bin/

- eeprom is simulated
	- starts out empty (todo: read from file)
	- 4k in size
	- lost when Bitlash terminates

- searches ~/.bitlash for scripts, in addition to eeprom

## Bugs

BUG: You can run a script from a file in ~/.bitlash/, but if that script calls another script file it fails.

BUG: Control+C exits instead of resetting to the Bitlash prompt

BUG: fprintf is not working

BUG: boot segfaults :)

## Unix-related functions

### system(shell-command)

Executes a command via the command shell.

### exit([return-code])

Exits Bitlash, passing the optional return code to the invoking shell.

### save([filename])

Saves the contents of eeprom in "ls" format as ~/.bitlash/filename.  Default filename is "eeprom".

You can load the saved functions later by simply typing the filename to run it as a script.

## Bitlash on Heroku

You can run the unix version of Bitlash in the cloud on Heroku, inside a web-based tty console courtesy of tty.js.  You will need a Heroku account.

	> heroku login 	(if you haven't already)
	> git clone https://github/billroy/bitlash
	> cd bitlash
	> heroku create
	> git push heroku master
	> heroku open

