/****************************************************************************
** Library to play 8bit uncompressed mono WAV files through the            **
** Sound Blaster                                                           **
**  by Steven H Don                                                        **
**                                                                         **
** Modified for DJGPP by adapting interrupt handling and DMA functions     **
**                                                                         **
** For questions, feel free to e-mail me.                                  **
**                                                                         **
**    shd@earthling.net                                                    **
**    http://shd.cjb.net                                                   **
**                                                                         **
****************************************************************************/
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>   // For memset, etc.
#include <dpmi.h>     // For DPMI services
#include <go32.h>     // For memory conversion
#include <sys/farptr.h>   // For farptr access
#include <sys/nearptr.h> // For __djgpp_conventional_base
#include <pc.h>       // For port I/O
#include <conio.h>    // For kbhit
#include <sys/segments.h> // For _chain_intr
#include "sbwav.h"

//A WAV header consits of several chunks:
struct
  RIFFChunkType {
    long RIFF;
    long NextChunkSize;
    long RIFFType;
  } RIFFChunk;

struct
  fmtChunkType {
    long  fmt;
    long  fmtLength;
    short WaveType;
    short Channels;
    long SampleRate;
    long BytesPerSecond;
    short BlockAlignment;
    short BitResolution;
  } fmtChunk;

struct
  dataChunkType {
    long data;
    long dataLength;
  } dataChunk;

  //Sound Blaster settings
  short         Base;        //Sound Blaster base address
  char          DMA;         //The DMA channel
  char          IRQ;         //The IRQ level
  
  // Replace old IRQ handler variable with DJGPP version
  _go32_dpmi_seginfo old_irq_handler;

  //Memory buffer
  short         RBuffer;     //Read buffer indicator
  unsigned char *DMABuffer;  //Pointer to DMA buffer
  short         Page;
  short         Offset;
  unsigned short dosmem_seg; // DOS memory segment for freeing

  //File access
  FILE          *WAVFile;
  volatile long ToBeRead;    //Amount of samples to be read from file
  volatile long ToBePlayed;  //Amount of samples to be played

  //Global indicator
  volatile int  Playing;

/****************************************************************************
** Checks to see if a Sound Blaster exists at a given address, returns     **
** true if Sound Blaster found, false if not.                              **
****************************************************************************/
int ResetDSP(short Test)
{
  //Reset the DSP
  outportb(Test + 0x6, 1);
  delay(10);
  outportb(Test + 0x6, 0);
  delay(10);
  //Check if reset was succesfull
  if (((inportb(Test + 0xE) & 0x80) == 0x80) && (inportb(Test + 0xA) == 0xAA))
  {
    //DSP was found
    Base = Test;
    return 1;
  }

  //No DSP was found
  return 0;
}

/****************************************************************************
** Send a byte to the DSP (Digital Signal Processor) on the Sound Blaster  **
****************************************************************************/
void WriteDSP(unsigned char Value)
{
  //Wait for the DSP to be ready to accept data
  while ((inportb(Base + 0xC) & 0x80) == 0x80);
  //Send byte
  outportb(Base + 0xC, Value);
}

/****************************************************************************
** The DMA controller is programmed with a block length of 32K - the       **
** entire buffer. The DSP is instructed to play blocks of 16K and then     **
** generate an interrupt (which allows the program to load the next part   **
** that should be played).                                                 **
****************************************************************************/
void AutoInitPlayback()
{
  outportb(0x0A, 4 | DMA);             //Mask DMA channel
  outportb(0x0C, 0);                   //Clear byte pointer
  outportb(0x0B, 0x58 | DMA);          //Set mode
  outportb(DMA << 1, Offset & 0xFF);   //Write the offset to the DMA controller
  outportb(DMA << 1, Offset >> 8);     //Write the offset to the DMA controller

  /*
    The mode consists of the following:
    0x58+x = binary 01 01 10 xx
                    |  |  |  |
                    |  |  |  +- DMA channel
                    |  |  +---- Read operation (the DSP reads from memory)
                    |  +------- Auto init mode
                    +---------- Block mode
  */

  //Write the page to the DMA controller
  switch (DMA) {
    case 0 : outportb(0x87, Page);
             break;
    case 1 : outportb(0x83, Page);
             break;
    case 3 : outportb(0x82, Page);
             break;
  }

  outportb((DMA << 1) + 1, 0xFF);  //Set the block length to 0x7FFF = 32 Kbyte
  outportb((DMA << 1) + 1, 0x7F);
  outportb(0x0A, DMA);             //Unmask DMA channel

  WriteDSP(0x48);                  //Set the block length to 0x3FFF bytes = 16 Kbyte
  WriteDSP(0xFF);
  WriteDSP(0x3F);

  WriteDSP(0x1C);                  //DSP-command 1Ch - Start auto-init playback
}

void SingleCyclePlayback()
{
  short BufOfs;

  ToBePlayed--;
  BufOfs = Offset + (RBuffer << 14);

  outportb(0x0A, 4 | DMA);             //Mask DMA channel
  outportb(0x0C, 0);                   //Clear byte pointer
  outportb(0x0B, 0x48 | DMA);          //Set mode
  outportb(DMA << 1, BufOfs & 0xFF);   //Write the offset to the DMA controller
  outportb(DMA << 1, BufOfs >> 8);     //Write the offset to the DMA controller
  /*
    The mode consists of the following:
    0x48+x = binary 01 00 10 xx
                    |  |  |  |
                    |  |  |  +- DMA channel
                    |  |  +---- Read operation (the DSP reads from memory)
                    |  +------- Single cycle mode
                    +---------- Block mode
  */

  //Write the page to the DMA controller
  if (DMA == 0) outportb(0x87, Page);
  if (DMA == 1) outportb(0x83, Page);
  if (DMA == 3) outportb(0x82, Page);

  //Set the block length
  outportb((DMA << 1) + 1, ToBePlayed & 0xFF);
  outportb((DMA << 1) + 1, ToBePlayed >> 8);
  outportb(0x0A, DMA);             //Unmask DMA channel

  //DSP-command 14h - 8bit single cycle playback
  WriteDSP(0x14);
  WriteDSP(ToBePlayed & 0xFF);
  WriteDSP(ToBePlayed >> 8);

  //Nothing left to play
  ToBePlayed = 0;
}

/****************************************************************************
** Loads one half of the DMA buffer from the file                          **
****************************************************************************/
void ReadBuffer(short Buffer)
{
  //If the remaining part of the file is smaller than 16K,
  //load it and fill out with silence
  if (ToBeRead <= 0) return;
  if (ToBeRead < 16384) {
    memset(DMABuffer + (Buffer << 14), 128, 16384);
    fread(DMABuffer + (Buffer << 14), 1, ToBeRead, WAVFile);
    ToBeRead = 0;
  } else {
    fread(DMABuffer + (Buffer << 14), 1, 16384, WAVFile);
    ToBeRead -= 16384;
  }
}

/****************************************************************************
** IRQ service routine - this is called when the DSP has finished playing  **
** a block                                                                 **
****************************************************************************/
void ServiceIRQ()
{
  //Relieve DSP, 8bit port
  inportb(Base + 0xE);
  
  //Acknowledge hardware interrupt
  outportb(0x20, 0x20);
  
  //Acknowledge cascade interrupt for IRQ 2, 10 and 11
  if (IRQ == 2 || IRQ == 10 || IRQ == 11) 
    outportb(0xA0, 0x20);
  
  //Take appropriate action for buffers
  if (Playing) {
    ToBePlayed -= 16384;
    if (ToBePlayed > 0) {
      ReadBuffer(RBuffer);
      if (ToBePlayed <= 16384) {
        RBuffer ^= 1;
        SingleCyclePlayback();
      } else if (ToBePlayed <= 32768) {
        WriteDSP(0xDA);
      }
    } else {
      Playing = 0;
    }
  }

  RBuffer ^= 1;
  
  // Let the default handler run
  _go32_dpmi_chain_protected_mode_interrupt_vector(
      (IRQ >= 8) ? IRQ - 8 + 0x70 : IRQ + 8, 
      &old_irq_handler);
}

/****************************************************************************
** This procedure allocates 32K of conventional memory for the DMA buffer  **
** and makes sure it does not cross a 64K page boundary                    **
****************************************************************************/
void AssignBuffer()
{
  __dpmi_regs r;
  unsigned long LinearAddress;
  
  // Allocate 32K of conventional memory using DPMI
  r.x.ax = 0x0100;          // DPMI function 0x0100 = allocate DOS memory
  r.x.bx = (32768 + 15) >> 4; // Number of paragraphs (rounded up)
  __dpmi_int(0x31, &r);
  
  if (r.x.flags & 1) {  // Check carry flag in flags
    // Allocation failed
    printf("Failed to allocate conventional memory for DMA buffer\n");
    exit(1);
  }
  
  // Get the real-mode segment
  dosmem_seg = r.x.ax;
  
  // Convert to a protected mode pointer
  DMABuffer = (unsigned char *)(__djgpp_conventional_base + (dosmem_seg << 4));
  
  // Calculate page and offset for DMA controller
  LinearAddress = (unsigned long)(dosmem_seg << 4);
  Page = (LinearAddress >> 16) & 0xFF;
  Offset = LinearAddress & 0xFFFF;
}

/****************************************************************************
** Free conventional memory allocated for DMA buffer                        **
****************************************************************************/
void FreeBuffer()
{
  __dpmi_regs r;
  
  // DPMI function 0x0101 = free DOS memory
  r.x.ax = 0x0101;
  r.x.dx = dosmem_seg;
  __dpmi_int(0x31, &r);
}

/****************************************************************************
** This procedure checks the possible addresses to see whether a Sound     **
** Blaster is installed.                                                   **
****************************************************************************/
void FindSB()
{
  char Temp;
  char *BLASTER;

  //Nothing found yet
  Base = 0;

  //Check for Sound Blaster, address: ports 210, 220, 230, 240, 250, 260 or 280
  for (Temp = 1; Temp < 9; Temp++) {
    if (Temp != 7)
    if (ResetDSP(0x200 + (Temp << 4))) {
      break;
    }
  }
  if (Temp == 9) return;

  //Search for IRQ and DMA entry in BLASTER environment string
  BLASTER = getenv("BLASTER");
  DMA = 0;
  if (BLASTER) {
    for (Temp = 0; Temp < strlen(BLASTER); Temp++)
      if ((BLASTER[Temp] | 32) == 'd')
        DMA = BLASTER[Temp + 1] - '0';
    for (Temp = 0; Temp < strlen(BLASTER); Temp++)
      if ((BLASTER[Temp] | 32) == 'i') {
        IRQ = BLASTER[Temp + 1] - '0';
        if (BLASTER[Temp + 2] != ' ')
          IRQ = IRQ * 10 + BLASTER[Temp + 2] - '0';
      }
  }
}

/****************************************************************************
** This procedure sets up the program according to the values in IRQ and   **
** DMA.                                                                    **
****************************************************************************/
void InitIRQandDMA()
{
  _go32_dpmi_seginfo new_handler;
  
  // Save old IRQ vector
  int irq_num = (IRQ >= 8) ? IRQ - 8 + 0x70 : IRQ + 8;
  _go32_dpmi_get_protected_mode_interrupt_vector(irq_num, &old_irq_handler);
  
  // Set up our ISR
  new_handler.pm_offset = (unsigned long)ServiceIRQ;
  new_handler.pm_selector = _go32_my_cs();
  _go32_dpmi_set_protected_mode_interrupt_vector(irq_num, &new_handler);
  
  // Enable IRQ
  if (IRQ == 2 || IRQ == 10 || IRQ == 11) {
    if (IRQ == 2) outportb(0xA1, inportb(0xA1) & 253);
    if (IRQ == 10) outportb(0xA1, inportb(0xA1) & 251);
    if (IRQ == 11) outportb(0xA1, inportb(0xA1) & 247);
    outportb(0x21, inportb(0x21) & 251);
  } else {
    outportb(0x21, inportb(0x21) & ~(1 << IRQ));
  }
}

/****************************************************************************
** This procedure releases the DMA channel and IRQ level                   **
****************************************************************************/
void ReleaseIRQandDMA()
{
  int irq_num = (IRQ >= 8) ? IRQ - 8 + 0x70 : IRQ + 8;
  
  // Restore original interrupt handler
  _go32_dpmi_set_protected_mode_interrupt_vector(irq_num, &old_irq_handler);
  
  // Mask IRQ (disable it)
  if (IRQ == 2 || IRQ == 10 || IRQ == 11) {
    if (IRQ == 2) outportb(0xA1, inportb(0xA1) | 2);
    if (IRQ == 10) outportb(0xA1, inportb(0xA1) | 4);
    if (IRQ == 11) outportb(0xA1, inportb(0xA1) | 8);
    outportb(0x21, inportb(0x21) | 4);
  } else {
    outportb(0x21, inportb(0x21) | (1 << IRQ));
  }
}

/*----------------------------------------------------------------------------
   EXTERNALLY VISIBLE FUNCTIONS
*/

/****************************************************************************
** This procedure returns true if the Sound Blaster is present.            **
****************************************************************************/
int SBFound()
{
  return Base != 0;
}

/****************************************************************************
** This procedure initialises the Sound Blaster                            **
****************************************************************************/
void SBOpen()
{
  //Set up IRQ and DMA channels
  InitIRQandDMA();

  //Assign memory to the DMA Buffer
  AssignBuffer();

  //Set read buffer to first buffer
  RBuffer = Playing = 0;
}

/****************************************************************************
** This procedure gracefully shuts down the unit.                          **
****************************************************************************/
void SBClose()
{
  //Stop any operation in progress
  if (Playing) SBStop();
  
  //Release the memory buffer
  FreeBuffer();
  
  //Release the IRQ and DMA channels
  ReleaseIRQandDMA();
}

/****************************************************************************
** Starts playing a WAV file.                                              **
****************************************************************************/
void SBPlay(char *FileName)
{
  long Before;

  //Don't play if there's no buffer
  if (!DMABuffer) return;

  //Start playback in buffer 0 and clear the buffer
  RBuffer = 0;
  memset(DMABuffer, 0, 32768);

  //Open the file for output
  WAVFile = fopen(FileName, "rb");
  if (!WAVFile) return;

  //Read RIFF chunk
  fread(&RIFFChunk, sizeof(RIFFChunk), 1, WAVFile);
  if (RIFFChunk.RIFF != 0x46464952 || RIFFChunk.RIFFType != 0x45564157) {
    fclose(WAVFile);
    return;
  }

  //Read fmt chunk
  do {
    Before = ftell(WAVFile);
    fread(&fmtChunk, sizeof(fmtChunk), 1, WAVFile);
    fseek(WAVFile, Before + fmtChunk.fmtLength + 8, SEEK_SET);
  } while (fmtChunk.fmt != 0x20746D66);

  //Set playback frequency
  WriteDSP(0x40);
  WriteDSP(256 - 1000000 / fmtChunk.SampleRate);

  //Read data chunk
  do {
    Before = ftell(WAVFile);
    fread(&dataChunk, sizeof(dataChunk), 1, WAVFile);
    if (dataChunk.data != 0x61746164)
      fseek(WAVFile, Before + dataChunk.dataLength + 8, SEEK_SET);
  } while (dataChunk.data != 0x61746164);
  ToBeRead = ToBePlayed = dataChunk.dataLength;

  //DSP-command D1h - Enable speaker
  WriteDSP(0xD1);

  //Read first bit of data
  ReadBuffer(0);
  ReadBuffer(1);

  if (ToBeRead > 0) AutoInitPlayback();
  else SingleCyclePlayback();

  Playing = 1;
}

/****************************************************************************
** Stops playback                                                          **
****************************************************************************/
void SBStop()
{
  //Stops DMA-transfer
  WriteDSP(0xD0);

  //Playback has completed
  Playing = 0;

  //Close the file
  fclose(WAVFile);
}

/****************************************************************************
** This procedure returns the length of the WAV file in whole seconds.     **
****************************************************************************/
long SBLength()
{
  return dataChunk.dataLength / fmtChunk.BytesPerSecond;
}

/****************************************************************************
** This procedure returns true if the WAV is still playing.                **
****************************************************************************/
int SBPlaying()
{
  return Playing;
}
