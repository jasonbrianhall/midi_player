/****************************************************************************
** Header file for the Sound Blaster WAV player library                    **
** by Steven H Don                                                        **
**                                                                         **
** Modified for DJGPP                                                     **
****************************************************************************/
#ifndef _SBWAV_H
#define _SBWAV_H

// Function prototypes for external interface
int  SBFound();      // Returns 1 if Sound Blaster is found, 0 otherwise
void SBOpen();       // Initialize Sound Blaster
void SBClose();      // Shut down Sound Blaster
void SBPlay(char *FileName);  // Play a WAV file
void SBStop();       // Stop playback
long SBLength();     // Get length of WAV file in seconds
int  SBPlaying();    // Returns 1 if Sound Blaster is playing, 0 otherwise

// Function to find the Sound Blaster
void FindSB();       // Find Sound Blaster and setup IRQ and DMA

// Expose the Sound Blaster configuration variables
extern short Base;   // Sound Blaster base address (e.g., 0x220)
extern char  IRQ;    // Sound Blaster IRQ channel
extern char  DMA;    // Sound Blaster DMA channel

#endif /* _SBWAV_H */
