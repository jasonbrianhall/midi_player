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
#include <dos.h>
#include "dos_utils.h"

/* Global variables */
volatile sig_atomic_t keep_running = 1;

/* Original interrupt vectors */
static void (*original_sigint_handler)(int);

/* Initialize terminal settings */
void init_terminal(void) {
    /* Save original Ctrl+C handler */
    original_sigint_handler = signal(SIGINT, handle_sigint);
}

/* Restore original terminal settings */
void restore_terminal(void) {
    /* Restore original Ctrl+C handler */
    signal(SIGINT, original_sigint_handler);
}

/* Signal handler for Ctrl+C */
void handle_sigint(int sig) {
    keep_running = 0;
    /* Don't exit here, let the program clean up properly */
}
