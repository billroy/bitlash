/*
	bitlash-unix.c: A minimal implementation of certain core Arduino functions	
	
	The author can be reached at: bill@bitlash.net

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
*/
#include "bitlash-private.h"
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

/*

Build:
	cd bitlash/src
	make

Issues


^C exits instead of STOP *
	signal handler

run scripts from file
	runs one script okay
	mux, which calls itself, fails

foo1 calling foo2 calling foo1

fprintf makes 0-byte file

system() using printf()
	print to buffer

command line options
	-e run command and exit
	-d working directory

full help text
boot segfaults ;)

*/

#define PATH_LEN 256
char bitlash_directory[PATH_LEN];
#define DEFAULT_BITLASH_PATH "/.bitlash/"

// fake eeprom
byte fake_eeprom[E2END];
byte eeread(int addr) { return fake_eeprom[addr]; }
void eewrite(int addr, byte value) { fake_eeprom[addr] = value; }
void init_fake_eeprom(void) {
int i=0;
	while (i <= E2END) eewrite(i++, 0xff);
}

FILE *savefd;
void fputbyte(byte b) {
	fwrite(&b, 1, 1, savefd);	
}

#ifdef SERIAL_OVERRIDE
numvar func_save(void) {
	const char *fname = "eeprom";
	if (getarg(0) > 0) fname = (const char *) getarg(1);
	savefd = fopen(fname, "w");
	if (!savefd) return 0;
	setOutputHandler(&fputbyte);
	cmd_ls();
	resetOutputHandler();
	fclose(savefd);
	return 1;
};
#endif



// background function thread
#include <pthread.h>
pthread_mutex_t executing;
pthread_t background_thread;
struct timespec wait_time;

void *BackgroundMacroThread(void *threadid) {
	for (;;) {
		pthread_mutex_lock(&executing);
		runBackgroundTasks();
		pthread_mutex_unlock(&executing);

		// sleep until next task runtime
		unsigned long sleep_time = millisUntilNextTask();
		if (sleep_time) {
			unsigned long seconds = sleep_time / 1000;
			wait_time.tv_sec = seconds;
			wait_time.tv_nsec = (sleep_time - (seconds * 1000)) * 1000000L;
			while (nanosleep(&wait_time, &wait_time) == -1) continue;
		}
	}
	return 0;
}


numvar func_system(void) {
	return system((const char *) getarg(1));
}

numvar func_exit(void) {
	if (getarg(0) > 0) exit(getarg(1));
	exit(0);
}


#include <signal.h>

byte break_received;

void inthandler(int signal) {
	break_received = 1;
}


int main () {

	FILE *shell = popen("echo ~", "r");
	if (!shell) {;}
	int got = fread(&bitlash_directory, 1, PATH_LEN, shell);
	pclose(shell);

	bitlash_directory[strlen(bitlash_directory) - 1] = 0;	// trim /n
	strcat(bitlash_directory, DEFAULT_BITLASH_PATH);

	//sp("Working directory: ");
	//sp(bitlash_directory); speol();

	if (chdir(bitlash_directory) != 0) {
		sp("Cannot enter .bitlash directory.  Does it exist?\n");
	}

	init_fake_eeprom();
	addBitlashFunction("system", (bitlash_function) &func_system);
	addBitlashFunction("exit", (bitlash_function) &func_exit);
	#ifdef SERIAL_OVERRIDE
	addBitlashFunction("save", (bitlash_function) &func_save);
	#endif

	// from bitlash-unix-file.c
	addBitlashFunction("exec", (bitlash_function) &exec);
	addBitlashFunction("dir", (bitlash_function) &sdls);
	addBitlashFunction("exists", (bitlash_function) &sdexists);
	addBitlashFunction("del", (bitlash_function) &sdrm);
//	addBitlashFunction("create", (bitlash_function) &sdcreate);
	addBitlashFunction("append", (bitlash_function) &sdappend);
	addBitlashFunction("type", (bitlash_function) &sdcat);
	addBitlashFunction("cd", (bitlash_function) &sdcd);
	addBitlashFunction("md", (bitlash_function) &sdmd);
	addBitlashFunction("pwd", (bitlash_function) &func_pwd);
	#ifdef SERIAL_OVERRIDE
	addBitlashFunction("fprintf", (bitlash_function) &func_fprintf);
	#endif


	init();
	initBitlash(0);

	//signal(SIGINT, inthandler);
	//signal(SIGKILL, inthandler);

	// run background functions on a separate thread
	pthread_create(&background_thread, NULL, BackgroundMacroThread, 0);

	// run the main stdin command loop
	for (;;) {
		const char * ret = fgets(lbuf, STRVALLEN, stdin);
		if (ret == NULL) break;	
		doCommand(lbuf);
		initlbuf();
	}

#if 0
	unsigned long next_key_time = 0L;
	unsigned long next_task_time = 0L;

	for (;;) {
		if (millis() > next_key_time) {
//		if (1) {

			// Pipe the serial input into the command handler
			while (serialAvailable()) doCharacter(serialRead());
			next_key_time = millis() + 100L;
		}

		// Background macro handler: feed it one call each time through
		if (millis() > next_task_time) {
			if (!runBackgroundTasks()) next_task_time = millis() + 10L;
		}
	}
#endif

}
