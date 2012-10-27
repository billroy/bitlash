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
#include "bitlash.h"

/*

Build:
				cd bitlash/src
	mac:		gcc *.c -o bitlash
	linux:		gcc -pthread *.c -o bitlash

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


#if _POSIX_TIMERS	// not on the Mac, unfortunately
struct timespec startup_time, current_time, elapsed_time;

// from http://www.guyrutenberg.com/2007/09/22/profiling-code-using-clock_gettime/
struct timespec time_diff(struct timespec start, struct timespec end) {
	struct timespec temp;
	if ((end.tv_nsec-start.tv_nsec)<0) {
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	return temp;
}

void init_millis(void) {
	clock_gettime(CLOCK_REALTIME, &startup_time);
}

unsigned long millis(void) {
	clock_gettime(CLOCK_REALTIME, &current_time);	
	elapsed_time = time_diff(startup_time, current_time);
	return (elapsed_time.tv_sec * 1000UL) + (elapsed_time.tv_nsec / 1000000UL);
}
#else
#include <sys/time.h>

unsigned long startup_millis, current_millis, elapsed_millis;
struct timeval startup_time, current_time;

// after http://laclefyoshi.blogspot.com/2011/05/getting-nanoseconds-in-c-on-freebsd.html
void init_millis(void) {
	gettimeofday(&startup_time, NULL);
	startup_millis = (startup_time.tv_sec * 1000) + (startup_time.tv_usec /1000);
}

unsigned long millis(void) {
	gettimeofday(&current_time, NULL);
	current_millis = (current_time.tv_sec * 1000) + (current_time.tv_usec / 1000);
	elapsed_millis = current_millis - startup_millis;
	return elapsed_millis;
}

#endif


#if 0
// after http://stackoverflow.com/questions/4025891/create-a-function-to-check-for-key-press-in-unix-using-ncurses
//#include <ncurses.h>

int init_keyboard(void) {
	initscr();
	cbreak();
	noecho();
	nodelay(stdscr, TRUE);
	scrollok(stdscr, TRUE);
}

int serialAvailable(void) {
	int ch = getch();

	if (ch != ERR) {
		ungetch(ch);
		return 1;
	} 
	else return 0;
}

int serialRead(void) { return getch(); }
#endif

#if 0
//#include "conio.h"
int lookahead_key = -1;

int serialAvailable(void) { 
	if (lookahead_key != -1) return 1; 
	lookahead_key = mygetch();
	if (lookahead_key == -1) return 0;
	//printf("getch: %d ", lookahead_key);
	return 1;
}

int serialRead(void) {
	if (lookahead_key != -1) {
		int retval = lookahead_key;
		lookahead_key = -1;
		//printf("key: %d", retval);
		return retval;
	}
	return mygetch();
}
#endif

#if 1
int serialAvailable(void) { 
	return 0;
}

int serialRead(void) {
	return '$';
}

#endif
	
void spb (char c) {
	if (serial_override_handler) (*serial_override_handler)(c);
	else {
		putchar(c);
		//printf("%c", c);
		fflush(stdout);
	}
}
void sp(const char *str) { while (*str) spb(*str++); }
void speol(void) { spb(13); spb(10); }

numvar setBaud(numvar pin, unumvar baud) { return 0; }

// stubs for the hardware IO functions
//
unsigned long pins;
void pinMode(byte pin, byte mode) { ; }
int digitalRead(byte pin) { return ((pins & (1<<pin)) != 0); }
void digitalWrite(byte pin, byte value) {
	if (value) pins |= 1<<pin;
	else pins &= ~(1<<pin);
}
int analogRead(byte pin) { return 0; }
void analogWrite(byte pin, int value) { ; }
int pulseIn(int pin, int mode, int duration) { return 0; }

// stubs for the time functions
//
void delay(unsigned long ms) {
//	unsigned long start = millis();
//	while (millis() - start < ms) { ; }
	struct timespec delay_time;
	long seconds = ms / 1000L;
	delay_time.tv_sec = seconds;
	delay_time.tv_nsec = (ms - (seconds * 1000L)) * 1000000L;
	while (nanosleep(&delay_time, &delay_time) == -1) continue;
}

void delayMicroseconds(unsigned int us) {
	struct timespec delay_time;
	long seconds = us / 1000000L;
	delay_time.tv_sec = seconds;
	delay_time.tv_nsec = (us - (seconds * 1000000L)) * 1000L;
	while (nanosleep(&delay_time, &delay_time) == -1) continue;
}

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

numvar func_save(void) {
	char *fname = "eeprom";
	if (getarg(0) > 0) fname = (char *) getarg(1);
	savefd = fopen(fname, "w");
	if (!savefd) return 0;
	setOutputHandler(&fputbyte);
	cmd_ls();
	resetOutputHandler();
	fclose(savefd);
	return 1;
};



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
	return system((char *) getarg(1));
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
	addBitlashFunction("save", (bitlash_function) &func_save);

	// from bitlash-unix-file.c
	extern bitlash_function exec, sdls, sdexists, sdrm, sdcreate, sdappend, sdcat, sdcd, sdmd, func_pwd;
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
	addBitlashFunction("fprintf", (bitlash_function) &func_fprintf);


	init_millis();
	initBitlash(0);

	//signal(SIGINT, inthandler);
	//signal(SIGKILL, inthandler);

	// run background functions on a separate thread
	pthread_create(&background_thread, NULL, BackgroundMacroThread, 0);

	// run the main stdin command loop
	for (;;) {
		char * ret = fgets(lbuf, STRVALLEN, stdin);
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
