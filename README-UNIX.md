# Bitlash on Unix

Here are some notes on experimental Unix support for Bitlash.

## Status

- Built/tested on OS X and Ubuntu Linux
	- the basic functions work
	- on OS X, numvar is 64 bits, so everything is 64 bits wide
	- Makefile in src/ will build the binary as src/bin/bitlash
	- there are some precompiled binaries in src/bin/

- eeprom is simulated
	- starts out empty (todo: read from file)
	- 4k in size
	- lost when Bitlash terminates

- searches ~/.bitlash for scripts, in addition to eeprom

## Bugs

BUG: Control+C exits instead of resetting to the Bitlash prompt

BUG: boot segfaults :)

BUG: the file handling commands should be merged with the internal unix-flavored eeprom function management commands

## Functions in the Unix version


### These are inherited from the bitlashsd version:

- append(filename, "string")
- cd(dirname)
- exec(script)
- fprintf(filename, format, var1, var2,...)
- exists(filename)
- del(filename)
- dir
- md(dirname)
- pwd
- type(filename

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

