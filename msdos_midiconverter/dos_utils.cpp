/*
 * DOS utility functions implementation for DJGPP
 */
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <pc.h>
#include <dpmi.h>
#include <go32.h>
#include <sys/farptr.h>
#include <signal.h>
#include "dos_utils.h"

/* Global variables */
volatile sig_atomic_t keep_running = 1;

/* Original Ctrl+C handler */
static void (*original_sigint_handler)(int);

/* Cleanup function to ensure terminal is reset */
static void cleanup_terminal(void) {
    /* Make sure we reset the terminal, regardless of how we exit */
    /* This will be called by atexit() */
    signal(SIGINT, original_sigint_handler);
}

/* Initialize terminal settings */
void init_terminal() {
    /* Save original Ctrl+C handler */
    original_sigint_handler = signal(SIGINT, handle_sigint);
    
    /* Register cleanup handler to ensure terminal reset */
    atexit(cleanup_terminal);
}

/* Restore original terminal settings */
void restore_terminal() {
    /* Restore original Ctrl+C handler */
    signal(SIGINT, original_sigint_handler);
}

/* Signal handler for Ctrl+C */
void handle_sigint(int sig) {
    keep_running = 0;
    
    /* Let main program exit naturally, no need to call restore_terminal here
       as the atexit handler will take care of it */
       
    /* Don't call exit() here, just return to let the program clean up */
}
