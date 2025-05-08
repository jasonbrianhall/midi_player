#include <stdio.h>
#include <conio.h>
#include <dos.h>
#include <dpmi.h>
#include <go32.h>
#include <sys/nearptr.h>
#include "sbwav.h"

int main()
{
  // Enable near pointers for conventional memory access
  __djgpp_nearptr_enable();
  
  // Check for Sound Blaster
  FindSB();
  if (!SBFound()) {
    printf("Sound Blaster not found.\n");
    __djgpp_nearptr_disable();
    return 1;
  }

  printf("Sound Blaster found at port %Xh, IRQ %d, DMA %d\n", Base, IRQ, DMA);
  printf("Initializing Sound Blaster...\n");
  
  // Initialize Sound Blaster
  SBOpen();

  printf("Playing TEST.WAV... Press any key to stop.\n");
  
  // Play a file
  SBPlay("TEST.WAV");
  
  // Wait until playback finishes or key is pressed
  do {
    // Small delay to prevent CPU hogging
    delay(10);
  } while (SBPlaying() && !kbhit());
  
  if (kbhit()) getch(); // Clear keyboard buffer
  
  printf("Stopping playback...\n");
  SBStop();

  printf("Closing Sound Blaster...\n");
  // Close Sound Blaster
  SBClose();
  
  // Disable near pointers before exiting
  __djgpp_nearptr_disable();
  
  printf("Done.\n");
  return 0;
}
