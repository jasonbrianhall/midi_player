/*
 * DOS utility functions for DJGPP
 * Replacements for Unix/Linux terminal functions used in the MIDI player
 */
#ifndef DOS_UTILS_H
#define DOS_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <pc.h>
#include <dpmi.h>
#include <go32.h>
#include <sys/farptr.h>
#include <signal.h>

/* Global variable to indicate if Ctrl+C was pressed */
extern volatile sig_atomic_t keep_running;

/* Initialize terminal settings for non-blocking input */
void init_terminal(void);

/* Restore original terminal settings */
void restore_terminal(void);

/* Signal handler for Ctrl+C */
void handle_sigint(int sig);

#endif /* DOS_UTILS_H */
