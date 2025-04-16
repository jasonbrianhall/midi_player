#include "sfmidi.h"

// Global declaration of FM instrument array
FMInstrument g_fmInstruments[181];// Initialize FM instrument data

void initFMInstruments() {
    // This function initializes the FM instrument data array
    // with 180 FM instrument definitions from the original QBasic code
    
    // GM1: Acoustic Grand Piano
    g_fmInstruments[0].modChar1 = 1;   g_fmInstruments[0].carChar1 = 1;
    g_fmInstruments[0].modChar2 = 143; g_fmInstruments[0].carChar2 = 6;
    g_fmInstruments[0].modChar3 = 242; g_fmInstruments[0].carChar3 = 242;
    g_fmInstruments[0].modChar4 = 244; g_fmInstruments[0].carChar4 = 247;
    g_fmInstruments[0].modChar5 = 0;   g_fmInstruments[0].carChar5 = 0;
    g_fmInstruments[0].fbConn = 56;    g_fmInstruments[0].percNote = 0;
    
    // GM2: Bright Acoustic Grand
    g_fmInstruments[1].modChar1 = 1;   g_fmInstruments[1].carChar1 = 1;
    g_fmInstruments[1].modChar2 = 75;  g_fmInstruments[1].carChar2 = 0;
    g_fmInstruments[1].modChar3 = 242; g_fmInstruments[1].carChar3 = 242;
    g_fmInstruments[1].modChar4 = 244; g_fmInstruments[1].carChar4 = 247;
    g_fmInstruments[1].modChar5 = 0;   g_fmInstruments[1].carChar5 = 0;
    g_fmInstruments[1].fbConn = 56;    g_fmInstruments[1].percNote = 0;
    
    // GM3: Electric Grand Piano
    g_fmInstruments[2].modChar1 = 1;   g_fmInstruments[2].carChar1 = 1;
    g_fmInstruments[2].modChar2 = 73;  g_fmInstruments[2].carChar2 = 0;
    g_fmInstruments[2].modChar3 = 242; g_fmInstruments[2].carChar3 = 242;
    g_fmInstruments[2].modChar4 = 244; g_fmInstruments[2].carChar4 = 246;
    g_fmInstruments[2].modChar5 = 0;   g_fmInstruments[2].carChar5 = 0;
    g_fmInstruments[2].fbConn = 56;    g_fmInstruments[2].percNote = 0;
    
    // GM4: Honky-tonk Piano
    g_fmInstruments[3].modChar1 = 129; g_fmInstruments[3].carChar1 = 65;
    g_fmInstruments[3].modChar2 = 18;  g_fmInstruments[3].carChar2 = 0;
    g_fmInstruments[3].modChar3 = 242; g_fmInstruments[3].carChar3 = 242;
    g_fmInstruments[3].modChar4 = 247; g_fmInstruments[3].carChar4 = 247;
    g_fmInstruments[3].modChar5 = 0;   g_fmInstruments[3].carChar5 = 0;
    g_fmInstruments[3].fbConn = 54;    g_fmInstruments[3].percNote = 0;
    
    // GM5: Rhodes Piano
    g_fmInstruments[4].modChar1 = 1;   g_fmInstruments[4].carChar1 = 1;
    g_fmInstruments[4].modChar2 = 87;  g_fmInstruments[4].carChar2 = 0;
    g_fmInstruments[4].modChar3 = 241; g_fmInstruments[4].carChar3 = 242;
    g_fmInstruments[4].modChar4 = 247; g_fmInstruments[4].carChar4 = 247;
    g_fmInstruments[4].modChar5 = 0;   g_fmInstruments[4].carChar5 = 0;
    g_fmInstruments[4].fbConn = 48;    g_fmInstruments[4].percNote = 0;
    
    // GM6: Chorused Piano
    g_fmInstruments[5].modChar1 = 1;   g_fmInstruments[5].carChar1 = 1;
    g_fmInstruments[5].modChar2 = 147; g_fmInstruments[5].carChar2 = 0;
    g_fmInstruments[5].modChar3 = 241; g_fmInstruments[5].carChar3 = 242;
    g_fmInstruments[5].modChar4 = 247; g_fmInstruments[5].carChar4 = 247;
    g_fmInstruments[5].modChar5 = 0;   g_fmInstruments[5].carChar5 = 0;
    g_fmInstruments[5].fbConn = 48;    g_fmInstruments[5].percNote = 0;
    
    // GM7: Harpsichord
    g_fmInstruments[6].modChar1 = 1;   g_fmInstruments[6].carChar1 = 22;
    g_fmInstruments[6].modChar2 = 128; g_fmInstruments[6].carChar2 = 14;
    g_fmInstruments[6].modChar3 = 161; g_fmInstruments[6].carChar3 = 242;
    g_fmInstruments[6].modChar4 = 242; g_fmInstruments[6].carChar4 = 245;
    g_fmInstruments[6].modChar5 = 0;   g_fmInstruments[6].carChar5 = 0;
    g_fmInstruments[6].fbConn = 56;    g_fmInstruments[6].percNote = 0;
    
    // GM8: Clavinet
    g_fmInstruments[7].modChar1 = 1;   g_fmInstruments[7].carChar1 = 1;
    g_fmInstruments[7].modChar2 = 146; g_fmInstruments[7].carChar2 = 0;
    g_fmInstruments[7].modChar3 = 194; g_fmInstruments[7].carChar3 = 194;
    g_fmInstruments[7].modChar4 = 248; g_fmInstruments[7].carChar4 = 248;
    g_fmInstruments[7].modChar5 = 0;   g_fmInstruments[7].carChar5 = 0;
    g_fmInstruments[7].fbConn = 58;    g_fmInstruments[7].percNote = 0;
    
    // GM9: Celesta
    g_fmInstruments[8].modChar1 = 12;  g_fmInstruments[8].carChar1 = 129;
    g_fmInstruments[8].modChar2 = 92;  g_fmInstruments[8].carChar2 = 0;
    g_fmInstruments[8].modChar3 = 246; g_fmInstruments[8].carChar3 = 243;
    g_fmInstruments[8].modChar4 = 244; g_fmInstruments[8].carChar4 = 245;
    g_fmInstruments[8].modChar5 = 0;   g_fmInstruments[8].carChar5 = 0;
    g_fmInstruments[8].fbConn = 48;    g_fmInstruments[8].percNote = 0;
    
    // GM10: Glockenspiel
    g_fmInstruments[9].modChar1 = 7;   g_fmInstruments[9].carChar1 = 17;
    g_fmInstruments[9].modChar2 = 151; g_fmInstruments[9].carChar2 = 128;
    g_fmInstruments[9].modChar3 = 243; g_fmInstruments[9].carChar3 = 242;
    g_fmInstruments[9].modChar4 = 242; g_fmInstruments[9].carChar4 = 241;
    g_fmInstruments[9].modChar5 = 0;   g_fmInstruments[9].carChar5 = 0;
    g_fmInstruments[9].fbConn = 50;    g_fmInstruments[9].percNote = 0;
    
    // GM11: Music box
    g_fmInstruments[10].modChar1 = 23;  g_fmInstruments[10].carChar1 = 1;
    g_fmInstruments[10].modChar2 = 33;  g_fmInstruments[10].carChar2 = 0;
    g_fmInstruments[10].modChar3 = 84;  g_fmInstruments[10].carChar3 = 244;
    g_fmInstruments[10].modChar4 = 244; g_fmInstruments[10].carChar4 = 244;
    g_fmInstruments[10].modChar5 = 0;   g_fmInstruments[10].carChar5 = 0;
    g_fmInstruments[10].fbConn = 50;    g_fmInstruments[10].percNote = 0;
    
    // GM12: Vibraphone
    g_fmInstruments[11].modChar1 = 152; g_fmInstruments[11].carChar1 = 129;
    g_fmInstruments[11].modChar2 = 98;  g_fmInstruments[11].carChar2 = 0;
    g_fmInstruments[11].modChar3 = 243; g_fmInstruments[11].carChar3 = 242;
    g_fmInstruments[11].modChar4 = 246; g_fmInstruments[11].carChar4 = 246;
    g_fmInstruments[11].modChar5 = 0;   g_fmInstruments[11].carChar5 = 0;
    g_fmInstruments[11].fbConn = 48;    g_fmInstruments[11].percNote = 0;
    
    // GM13: Marimba
    g_fmInstruments[12].modChar1 = 24;  g_fmInstruments[12].carChar1 = 1;
    g_fmInstruments[12].modChar2 = 35;  g_fmInstruments[12].carChar2 = 0;
    g_fmInstruments[12].modChar3 = 246; g_fmInstruments[12].carChar3 = 231;
    g_fmInstruments[12].modChar4 = 246; g_fmInstruments[12].carChar4 = 247;
    g_fmInstruments[12].modChar5 = 0;   g_fmInstruments[12].carChar5 = 0;
    g_fmInstruments[12].fbConn = 48;    g_fmInstruments[12].percNote = 0;
    
    // GM14: Xylophone
    g_fmInstruments[13].modChar1 = 21;  g_fmInstruments[13].carChar1 = 1;
    g_fmInstruments[13].modChar2 = 145; g_fmInstruments[13].carChar2 = 0;
    g_fmInstruments[13].modChar3 = 246; g_fmInstruments[13].carChar3 = 246;
    g_fmInstruments[13].modChar4 = 246; g_fmInstruments[13].carChar4 = 246;
    g_fmInstruments[13].modChar5 = 0;   g_fmInstruments[13].carChar5 = 0;
    g_fmInstruments[13].fbConn = 52;    g_fmInstruments[13].percNote = 0;
    
    // GM15: Tubular Bells
    g_fmInstruments[14].modChar1 = 69;  g_fmInstruments[14].carChar1 = 129;
    g_fmInstruments[14].modChar2 = 89;  g_fmInstruments[14].carChar2 = 128;
    g_fmInstruments[14].modChar3 = 211; g_fmInstruments[14].carChar3 = 163;
    g_fmInstruments[14].modChar4 = 243; g_fmInstruments[14].carChar4 = 243;
    g_fmInstruments[14].modChar5 = 0;   g_fmInstruments[14].carChar5 = 0;
    g_fmInstruments[14].fbConn = 60;    g_fmInstruments[14].percNote = 0;
    
    // GM16: Dulcimer
    g_fmInstruments[15].modChar1 = 3;   g_fmInstruments[15].carChar1 = 129;
    g_fmInstruments[15].modChar2 = 73;  g_fmInstruments[15].carChar2 = 128;
    g_fmInstruments[15].modChar3 = 117; g_fmInstruments[15].carChar3 = 181;
    g_fmInstruments[15].modChar4 = 245; g_fmInstruments[15].carChar4 = 245;
    g_fmInstruments[15].modChar5 = 1;   g_fmInstruments[15].carChar5 = 0;
    g_fmInstruments[15].fbConn = 52;    g_fmInstruments[15].percNote = 0;
    
    // GM17: Hammond Organ
    g_fmInstruments[16].modChar1 = 113; g_fmInstruments[16].carChar1 = 49;
    g_fmInstruments[16].modChar2 = 146; g_fmInstruments[16].carChar2 = 0;
    g_fmInstruments[16].modChar3 = 246; g_fmInstruments[16].carChar3 = 241;
    g_fmInstruments[16].modChar4 = 20;  g_fmInstruments[16].carChar4 = 7;
    g_fmInstruments[16].modChar5 = 0;   g_fmInstruments[16].carChar5 = 0;
    g_fmInstruments[16].fbConn = 50;    g_fmInstruments[16].percNote = 0;
    
    // GM18: Percussive Organ
    g_fmInstruments[17].modChar1 = 114; g_fmInstruments[17].carChar1 = 48;
    g_fmInstruments[17].modChar2 = 20;  g_fmInstruments[17].carChar2 = 0;
    g_fmInstruments[17].modChar3 = 199; g_fmInstruments[17].carChar3 = 199;
    g_fmInstruments[17].modChar4 = 88;  g_fmInstruments[17].carChar4 = 8;
    g_fmInstruments[17].modChar5 = 0;   g_fmInstruments[17].carChar5 = 0;
    g_fmInstruments[17].fbConn = 50;    g_fmInstruments[17].percNote = 0;
    
    // GM19: Rock Organ
    g_fmInstruments[18].modChar1 = 112; g_fmInstruments[18].carChar1 = 177;
    g_fmInstruments[18].modChar2 = 68;  g_fmInstruments[18].carChar2 = 0;
    g_fmInstruments[18].modChar3 = 170; g_fmInstruments[18].carChar3 = 138;
    g_fmInstruments[18].modChar4 = 24;  g_fmInstruments[18].carChar4 = 8;
    g_fmInstruments[18].modChar5 = 0;   g_fmInstruments[18].carChar5 = 0;
    g_fmInstruments[18].fbConn = 52;    g_fmInstruments[18].percNote = 0;
    
    // GM20: Church Organ
    g_fmInstruments[19].modChar1 = 35;  g_fmInstruments[19].carChar1 = 177;
    g_fmInstruments[19].modChar2 = 147; g_fmInstruments[19].carChar2 = 0;
    g_fmInstruments[19].modChar3 = 151; g_fmInstruments[19].carChar3 = 85;
    g_fmInstruments[19].modChar4 = 35;  g_fmInstruments[19].carChar4 = 20;
    g_fmInstruments[19].modChar5 = 1;   g_fmInstruments[19].carChar5 = 0;
    g_fmInstruments[19].fbConn = 52;    g_fmInstruments[19].percNote = 0;
    
    // GM21: Reed Organ
    g_fmInstruments[20].modChar1 = 97;  g_fmInstruments[20].carChar1 = 177;
    g_fmInstruments[20].modChar2 = 19;  g_fmInstruments[20].carChar2 = 128;
    g_fmInstruments[20].modChar3 = 151; g_fmInstruments[20].carChar3 = 85;
    g_fmInstruments[20].modChar4 = 4;   g_fmInstruments[20].carChar4 = 4;
    g_fmInstruments[20].modChar5 = 1;   g_fmInstruments[20].carChar5 = 0;
    g_fmInstruments[20].fbConn = 48;    g_fmInstruments[20].percNote = 0;
    
    // GM22: Accordion
    g_fmInstruments[21].modChar1 = 36;  g_fmInstruments[21].carChar1 = 177;
    g_fmInstruments[21].modChar2 = 72;  g_fmInstruments[21].carChar2 = 0;
    g_fmInstruments[21].modChar3 = 152; g_fmInstruments[21].carChar3 = 70;
    g_fmInstruments[21].modChar4 = 42;  g_fmInstruments[21].carChar4 = 26;
    g_fmInstruments[21].modChar5 = 1;   g_fmInstruments[21].carChar5 = 0;
    g_fmInstruments[21].fbConn = 60;    g_fmInstruments[21].percNote = 0;
    
    // GM23: Harmonica
    g_fmInstruments[22].modChar1 = 97;  g_fmInstruments[22].carChar1 = 33;
    g_fmInstruments[22].modChar2 = 19;  g_fmInstruments[22].carChar2 = 0;
    g_fmInstruments[22].modChar3 = 145; g_fmInstruments[22].carChar3 = 97;
    g_fmInstruments[22].modChar4 = 6;   g_fmInstruments[22].carChar4 = 7;
    g_fmInstruments[22].modChar5 = 1;   g_fmInstruments[22].carChar5 = 0;
    g_fmInstruments[22].fbConn = 58;    g_fmInstruments[22].percNote = 0;
    
    // GM24: Tango Accordion
    g_fmInstruments[23].modChar1 = 33;  g_fmInstruments[23].carChar1 = 161;
    g_fmInstruments[23].modChar2 = 19;  g_fmInstruments[23].carChar2 = 137;
    g_fmInstruments[23].modChar3 = 113; g_fmInstruments[23].carChar3 = 97;
    g_fmInstruments[23].modChar4 = 6;   g_fmInstruments[23].carChar4 = 7;
    g_fmInstruments[23].modChar5 = 0;   g_fmInstruments[23].carChar5 = 0;
    g_fmInstruments[23].fbConn = 54;    g_fmInstruments[23].percNote = 0;
    
    // GM25: Acoustic Guitar1
    g_fmInstruments[24].modChar1 = 2;   g_fmInstruments[24].carChar1 = 65;
    g_fmInstruments[24].modChar2 = 156; g_fmInstruments[24].carChar2 = 128;
    g_fmInstruments[24].modChar3 = 243; g_fmInstruments[24].carChar3 = 243;
    g_fmInstruments[24].modChar4 = 148; g_fmInstruments[24].carChar4 = 200;
    g_fmInstruments[24].modChar5 = 1;   g_fmInstruments[24].carChar5 = 0;
    g_fmInstruments[24].fbConn = 60;    g_fmInstruments[24].percNote = 0;
    
    // GM26: Acoustic Guitar2
    g_fmInstruments[25].modChar1 = 3;   g_fmInstruments[25].carChar1 = 17;
    g_fmInstruments[25].modChar2 = 84;  g_fmInstruments[25].carChar2 = 0;
    g_fmInstruments[25].modChar3 = 243; g_fmInstruments[25].carChar3 = 241;
    g_fmInstruments[25].modChar4 = 154; g_fmInstruments[25].carChar4 = 231;
    g_fmInstruments[25].modChar5 = 1;   g_fmInstruments[25].carChar5 = 0;
    g_fmInstruments[25].fbConn = 60;    g_fmInstruments[25].percNote = 0;
    
    // GM27: Electric Guitar1
    g_fmInstruments[26].modChar1 = 35;  g_fmInstruments[26].carChar1 = 33;
    g_fmInstruments[26].modChar2 = 95;  g_fmInstruments[26].carChar2 = 0;
    g_fmInstruments[26].modChar3 = 241; g_fmInstruments[26].carChar3 = 242;
    g_fmInstruments[26].modChar4 = 58;  g_fmInstruments[26].carChar4 = 248;
    g_fmInstruments[26].modChar5 = 0;   g_fmInstruments[26].carChar5 = 0;
    g_fmInstruments[26].fbConn = 48;    g_fmInstruments[26].percNote = 0;
    
    // GM28: Electric Guitar2
    g_fmInstruments[27].modChar1 = 3;   g_fmInstruments[27].carChar1 = 33;
    g_fmInstruments[27].modChar2 = 135; g_fmInstruments[27].carChar2 = 128;
    g_fmInstruments[27].modChar3 = 246; g_fmInstruments[27].carChar3 = 243;
    g_fmInstruments[27].modChar4 = 34;  g_fmInstruments[27].carChar4 = 248;
    g_fmInstruments[27].modChar5 = 1;   g_fmInstruments[27].carChar5 = 0;
    g_fmInstruments[27].fbConn = 54;    g_fmInstruments[27].percNote = 0;
    
    // GM29: Electric Guitar3
    g_fmInstruments[28].modChar1 = 3;   g_fmInstruments[28].carChar1 = 33;
    g_fmInstruments[28].modChar2 = 71;  g_fmInstruments[28].carChar2 = 0;
    g_fmInstruments[28].modChar3 = 249; g_fmInstruments[28].carChar3 = 246;
    g_fmInstruments[28].modChar4 = 84;  g_fmInstruments[28].carChar4 = 58;
    g_fmInstruments[28].modChar5 = 0;   g_fmInstruments[28].carChar5 = 0;
    g_fmInstruments[28].fbConn = 48;    g_fmInstruments[28].percNote = 0;
    
    // GM30: Overdrive Guitar
    g_fmInstruments[29].modChar1 = 35;  g_fmInstruments[29].carChar1 = 33;
    g_fmInstruments[29].modChar2 = 74;  g_fmInstruments[29].carChar2 = 5;
    g_fmInstruments[29].modChar3 = 145; g_fmInstruments[29].carChar3 = 132;
    g_fmInstruments[29].modChar4 = 65;  g_fmInstruments[29].carChar4 = 25;
    g_fmInstruments[29].modChar5 = 1;   g_fmInstruments[29].carChar5 = 0;
    g_fmInstruments[29].fbConn = 56;    g_fmInstruments[29].percNote = 0;
    
    // GM31: Distortion Guitar
    g_fmInstruments[30].modChar1 = 35;  g_fmInstruments[30].carChar1 = 33;
    g_fmInstruments[30].modChar2 = 74;  g_fmInstruments[30].carChar2 = 0;
    g_fmInstruments[30].modChar3 = 149; g_fmInstruments[30].carChar3 = 148;
    g_fmInstruments[30].modChar4 = 25;  g_fmInstruments[30].carChar4 = 25;
    g_fmInstruments[30].modChar5 = 1;   g_fmInstruments[30].carChar5 = 0;
    g_fmInstruments[30].fbConn = 56;    g_fmInstruments[30].percNote = 0;
    
    // GM32: Guitar Harmonics
    g_fmInstruments[31].modChar1 = 9;   g_fmInstruments[31].carChar1 = 132;
    g_fmInstruments[31].modChar2 = 161; g_fmInstruments[31].carChar2 = 128;
    g_fmInstruments[31].modChar3 = 32;  g_fmInstruments[31].carChar3 = 209;
    g_fmInstruments[31].modChar4 = 79;  g_fmInstruments[31].carChar4 = 248;
    g_fmInstruments[31].modChar5 = 0;   g_fmInstruments[31].carChar5 = 0;
    g_fmInstruments[31].fbConn = 56;    g_fmInstruments[31].percNote = 0;
    
    // GM33: Acoustic Bass
    g_fmInstruments[32].modChar1 = 33;  g_fmInstruments[32].carChar1 = 162;
    g_fmInstruments[32].modChar2 = 30;  g_fmInstruments[32].carChar2 = 0;
    g_fmInstruments[32].modChar3 = 148; g_fmInstruments[32].carChar3 = 195;
    g_fmInstruments[32].modChar4 = 6;   g_fmInstruments[32].carChar4 = 166;
    g_fmInstruments[32].modChar5 = 0;   g_fmInstruments[32].carChar5 = 0;
    g_fmInstruments[32].fbConn = 50;    g_fmInstruments[32].percNote = 0;
    
    // GM34: Electric Bass 1
    g_fmInstruments[33].modChar1 = 49;  g_fmInstruments[33].carChar1 = 49;
    g_fmInstruments[33].modChar2 = 18;  g_fmInstruments[33].carChar2 = 0;
    g_fmInstruments[33].modChar3 = 241; g_fmInstruments[33].carChar3 = 241;
    g_fmInstruments[33].modChar4 = 40;  g_fmInstruments[33].carChar4 = 24;
    g_fmInstruments[33].modChar5 = 0;   g_fmInstruments[33].carChar5 = 0;
    g_fmInstruments[33].fbConn = 58;    g_fmInstruments[33].percNote = 0;
    
    // GM35: Electric Bass 2
    g_fmInstruments[34].modChar1 = 49;  g_fmInstruments[34].carChar1 = 49;
    g_fmInstruments[34].modChar2 = 141; g_fmInstruments[34].carChar2 = 0;
    g_fmInstruments[34].modChar3 = 241; g_fmInstruments[34].carChar3 = 241;
    g_fmInstruments[34].modChar4 = 232; g_fmInstruments[34].carChar4 = 120;
    g_fmInstruments[34].modChar5 = 0;   g_fmInstruments[34].carChar5 = 0;
    g_fmInstruments[34].fbConn = 58;    g_fmInstruments[34].percNote = 0;
    
    // GM36: Fretless Bass
    g_fmInstruments[35].modChar1 = 49;  g_fmInstruments[35].carChar1 = 50;
    g_fmInstruments[35].modChar2 = 91;  g_fmInstruments[35].carChar2 = 0;
    g_fmInstruments[35].modChar3 = 81;  g_fmInstruments[35].carChar3 = 113;
    g_fmInstruments[35].modChar4 = 40;  g_fmInstruments[35].carChar4 = 72;
    g_fmInstruments[35].modChar5 = 0;   g_fmInstruments[35].carChar5 = 0;
    g_fmInstruments[35].fbConn = 60;    g_fmInstruments[35].percNote = 0;
    
    // GM37: Slap Bass 1
    g_fmInstruments[36].modChar1 = 1;   g_fmInstruments[36].carChar1 = 33;
    g_fmInstruments[36].modChar2 = 139; g_fmInstruments[36].carChar2 = 64;
    g_fmInstruments[36].modChar3 = 161; g_fmInstruments[36].carChar3 = 242;
    g_fmInstruments[36].modChar4 = 154; g_fmInstruments[36].carChar4 = 223;
    g_fmInstruments[36].modChar5 = 0;   g_fmInstruments[36].carChar5 = 0;
    g_fmInstruments[36].fbConn = 56;    g_fmInstruments[36].percNote = 0;
    
    // GM38: Slap Bass 2
    g_fmInstruments[37].modChar1 = 33;  g_fmInstruments[37].carChar1 = 33;
    g_fmInstruments[37].modChar2 = 139; g_fmInstruments[37].carChar2 = 8;
    g_fmInstruments[37].modChar3 = 162; g_fmInstruments[37].carChar3 = 161;
    g_fmInstruments[37].modChar4 = 22;  g_fmInstruments[37].carChar4 = 223;
    g_fmInstruments[37].modChar5 = 0;   g_fmInstruments[37].carChar5 = 0;
    g_fmInstruments[37].fbConn = 56;    g_fmInstruments[37].percNote = 0;
    
    // GM39: Synth Bass 1
    g_fmInstruments[38].modChar1 = 49;  g_fmInstruments[38].carChar1 = 49;
    g_fmInstruments[38].modChar2 = 139; g_fmInstruments[38].carChar2 = 0;
    g_fmInstruments[38].modChar3 = 244; g_fmInstruments[38].carChar3 = 241;
    g_fmInstruments[38].modChar4 = 232; g_fmInstruments[38].carChar4 = 120;
    g_fmInstruments[38].modChar5 = 0;   g_fmInstruments[38].carChar5 = 0;
    g_fmInstruments[38].fbConn = 58;    g_fmInstruments[38].percNote = 0;
    
    // GM40: Synth Bass 2
    g_fmInstruments[39].modChar1 = 49;  g_fmInstruments[39].carChar1 = 49;
    g_fmInstruments[39].modChar2 = 18;  g_fmInstruments[39].carChar2 = 0;
    g_fmInstruments[39].modChar3 = 241; g_fmInstruments[39].carChar3 = 241;
    g_fmInstruments[39].modChar4 = 40;  g_fmInstruments[39].carChar4 = 24;
    g_fmInstruments[39].modChar5 = 0;   g_fmInstruments[39].carChar5 = 0;
    g_fmInstruments[39].fbConn = 58;    g_fmInstruments[39].percNote = 0;
    
    // GM41: Violin
    g_fmInstruments[40].modChar1 = 49;  g_fmInstruments[40].carChar1 = 33;
    g_fmInstruments[40].modChar2 = 21;  g_fmInstruments[40].carChar2 = 0;
    g_fmInstruments[40].modChar3 = 221; g_fmInstruments[40].carChar3 = 86;
    g_fmInstruments[40].modChar4 = 19;  g_fmInstruments[40].carChar4 = 38;
    g_fmInstruments[40].modChar5 = 1;   g_fmInstruments[40].carChar5 = 0;
    g_fmInstruments[40].fbConn = 56;    g_fmInstruments[40].percNote = 0;
    
    // GM42: Viola
    g_fmInstruments[41].modChar1 = 49;  g_fmInstruments[41].carChar1 = 33;
    g_fmInstruments[41].modChar2 = 22;  g_fmInstruments[41].carChar2 = 0;
    g_fmInstruments[41].modChar3 = 221; g_fmInstruments[41].carChar3 = 102;
    g_fmInstruments[41].modChar4 = 19;  g_fmInstruments[41].carChar4 = 6;
    g_fmInstruments[41].modChar5 = 1;   g_fmInstruments[41].carChar5 = 0;
    g_fmInstruments[41].fbConn = 56;    g_fmInstruments[41].percNote = 0;
    
    // GM43: Cello
    g_fmInstruments[42].modChar1 = 113; g_fmInstruments[42].carChar1 = 49;
    g_fmInstruments[42].modChar2 = 73;  g_fmInstruments[42].carChar2 = 0;
    g_fmInstruments[42].modChar3 = 209; g_fmInstruments[42].carChar3 = 97;
    g_fmInstruments[42].modChar4 = 28;  g_fmInstruments[42].carChar4 = 12;
    g_fmInstruments[42].modChar5 = 1;   g_fmInstruments[42].carChar5 = 0;
    g_fmInstruments[42].fbConn = 56;    g_fmInstruments[42].percNote = 0;
    
    // GM44: Contrabass
    g_fmInstruments[43].modChar1 = 33;  g_fmInstruments[43].carChar1 = 35;
    g_fmInstruments[43].modChar2 = 77;  g_fmInstruments[43].carChar2 = 128;
    g_fmInstruments[43].modChar3 = 113; g_fmInstruments[43].carChar3 = 114;
    g_fmInstruments[43].modChar4 = 18;  g_fmInstruments[43].carChar4 = 6;
    g_fmInstruments[43].modChar5 = 1;   g_fmInstruments[43].carChar5 = 0;
    g_fmInstruments[43].fbConn = 50;    g_fmInstruments[43].percNote = 0;
    
    // GM45: Tremulo Strings
    g_fmInstruments[44].modChar1 = 241; g_fmInstruments[44].carChar1 = 225;
    g_fmInstruments[44].modChar2 = 64;  g_fmInstruments[44].carChar2 = 0;
    g_fmInstruments[44].modChar3 = 241; g_fmInstruments[44].carChar3 = 111;
    g_fmInstruments[44].modChar4 = 33;  g_fmInstruments[44].carChar4 = 22;
    g_fmInstruments[44].modChar5 = 1;   g_fmInstruments[44].carChar5 = 0;
    g_fmInstruments[44].fbConn = 50;    g_fmInstruments[44].percNote = 0;
    
    // GM46: Pizzicato String
    g_fmInstruments[45].modChar1 = 2;   g_fmInstruments[45].carChar1 = 1;
    g_fmInstruments[45].modChar2 = 26;  g_fmInstruments[45].carChar2 = 128;
    g_fmInstruments[45].modChar3 = 245; g_fmInstruments[45].carChar3 = 133;
    g_fmInstruments[45].modChar4 = 117; g_fmInstruments[45].carChar4 = 53;
    g_fmInstruments[45].modChar5 = 1;   g_fmInstruments[45].carChar5 = 0;
    g_fmInstruments[45].fbConn = 48;    g_fmInstruments[45].percNote = 0;
    
    // GM47: Orchestral Harp
    g_fmInstruments[46].modChar1 = 2;   g_fmInstruments[46].carChar1 = 1;
    g_fmInstruments[46].modChar2 = 29;  g_fmInstruments[46].carChar2 = 128;
    g_fmInstruments[46].modChar3 = 245; g_fmInstruments[46].carChar3 = 243;
    g_fmInstruments[46].modChar4 = 117; g_fmInstruments[46].carChar4 = 244;
    g_fmInstruments[46].modChar5 = 1;   g_fmInstruments[46].carChar5 = 0;
    g_fmInstruments[46].fbConn = 48;    g_fmInstruments[46].percNote = 0;
    
    // GM48: Timpany
    g_fmInstruments[47].modChar1 = 16;  g_fmInstruments[47].carChar1 = 17;
    g_fmInstruments[47].modChar2 = 65;  g_fmInstruments[47].carChar2 = 0;
    g_fmInstruments[47].modChar3 = 245; g_fmInstruments[47].carChar3 = 242;
    g_fmInstruments[47].modChar4 = 5;   g_fmInstruments[47].carChar4 = 195;
    g_fmInstruments[47].modChar5 = 1;   g_fmInstruments[47].carChar5 = 0;
    g_fmInstruments[47].fbConn = 50;    g_fmInstruments[47].percNote = 0;
    
    // GM49: String Ensemble1
    g_fmInstruments[48].modChar1 = 33;  g_fmInstruments[48].carChar1 = 162;
    g_fmInstruments[48].modChar2 = 155; g_fmInstruments[48].carChar2 = 1;
    g_fmInstruments[48].modChar3 = 177; g_fmInstruments[48].carChar3 = 114;
    g_fmInstruments[48].modChar4 = 37;  g_fmInstruments[48].carChar4 = 8;
    g_fmInstruments[48].modChar5 = 1;   g_fmInstruments[48].carChar5 = 0;
    g_fmInstruments[48].fbConn = 62;    g_fmInstruments[48].percNote = 0;
    
    // GM50: String Ensemble2
    g_fmInstruments[49].modChar1 = 161; g_fmInstruments[49].carChar1 = 33;
    g_fmInstruments[49].modChar2 = 152; g_fmInstruments[49].carChar2 = 0;
    g_fmInstruments[49].modChar3 = 127; g_fmInstruments[49].carChar3 = 63;
    g_fmInstruments[49].modChar4 = 3;   g_fmInstruments[49].carChar4 = 7;
    g_fmInstruments[49].modChar5 = 1;   g_fmInstruments[49].carChar5 = 1;
    g_fmInstruments[49].fbConn = 48;    g_fmInstruments[49].percNote = 0;
    
    // GM51: Synth Strings 1
    g_fmInstruments[50].modChar1 = 161; g_fmInstruments[50].carChar1 = 97;
    g_fmInstruments[50].modChar2 = 147; g_fmInstruments[50].carChar2 = 0;
    g_fmInstruments[50].modChar3 = 193; g_fmInstruments[50].carChar3 = 79;
    g_fmInstruments[50].modChar4 = 18;  g_fmInstruments[50].carChar4 = 5;
    g_fmInstruments[50].modChar5 = 0;   g_fmInstruments[50].carChar5 = 0;
    g_fmInstruments[50].fbConn = 58;    g_fmInstruments[50].percNote = 0;
    
    // GM52: SynthStrings 2
    g_fmInstruments[51].modChar1 = 33;  g_fmInstruments[51].carChar1 = 97;
    g_fmInstruments[51].modChar2 = 24;  g_fmInstruments[51].carChar2 = 0;
    g_fmInstruments[51].modChar3 = 193; g_fmInstruments[51].carChar3 = 79;
    g_fmInstruments[51].modChar4 = 34;  g_fmInstruments[51].carChar4 = 5;
    g_fmInstruments[51].modChar5 = 0;   g_fmInstruments[51].carChar5 = 0;
    g_fmInstruments[51].fbConn = 60;    g_fmInstruments[51].percNote = 0;
    
    // GM53: Choir Aahs
    g_fmInstruments[52].modChar1 = 49;  g_fmInstruments[52].carChar1 = 114;
    g_fmInstruments[52].modChar2 = 91;  g_fmInstruments[52].carChar2 = 131;
    g_fmInstruments[52].modChar3 = 244; g_fmInstruments[52].carChar3 = 138;
    g_fmInstruments[52].modChar4 = 21;  g_fmInstruments[52].carChar4 = 5;
    g_fmInstruments[52].modChar5 = 0;   g_fmInstruments[52].carChar5 = 0;
    g_fmInstruments[52].fbConn = 48;    g_fmInstruments[52].percNote = 0;
    
    // GM54: Voice Oohs
    g_fmInstruments[53].modChar1 = 161; g_fmInstruments[53].carChar1 = 97;
    g_fmInstruments[53].modChar2 = 144; g_fmInstruments[53].carChar2 = 0;
    g_fmInstruments[53].modChar3 = 116; g_fmInstruments[53].carChar3 = 113;
    g_fmInstruments[53].modChar4 = 57;  g_fmInstruments[53].carChar4 = 103;
    g_fmInstruments[53].modChar5 = 0;   g_fmInstruments[53].carChar5 = 0;
    g_fmInstruments[53].fbConn = 48;    g_fmInstruments[53].percNote = 0;
    
    // GM55: Synth Voice
    g_fmInstruments[54].modChar1 = 113; g_fmInstruments[54].carChar1 = 114;
    g_fmInstruments[54].modChar2 = 87;  g_fmInstruments[54].carChar2 = 0;
    g_fmInstruments[54].modChar3 = 84;  g_fmInstruments[54].carChar3 = 122;
    g_fmInstruments[54].modChar4 = 5;   g_fmInstruments[54].carChar4 = 5;
    g_fmInstruments[54].modChar5 = 0;   g_fmInstruments[54].carChar5 = 0;
    g_fmInstruments[54].fbConn = 60;    g_fmInstruments[54].percNote = 0;
    
    // GM56: Orchestra Hit
    g_fmInstruments[55].modChar1 = 144; g_fmInstruments[55].carChar1 = 65;
    g_fmInstruments[55].modChar2 = 0;   g_fmInstruments[55].carChar2 = 0;
    g_fmInstruments[55].modChar3 = 84;  g_fmInstruments[55].carChar3 = 165;
    g_fmInstruments[55].modChar4 = 99;  g_fmInstruments[55].carChar4 = 69;
    g_fmInstruments[55].modChar5 = 0;   g_fmInstruments[55].carChar5 = 0;
    g_fmInstruments[55].fbConn = 56;    g_fmInstruments[55].percNote = 0;
    
    // GM57: Trumpet
    g_fmInstruments[56].modChar1 = 33;  g_fmInstruments[56].carChar1 = 33;
    g_fmInstruments[56].modChar2 = 146; g_fmInstruments[56].carChar2 = 1;
    g_fmInstruments[56].modChar3 = 133; g_fmInstruments[56].carChar3 = 143;
    g_fmInstruments[56].modChar4 = 23;  g_fmInstruments[56].carChar4 = 9;
    g_fmInstruments[56].modChar5 = 0;   g_fmInstruments[56].carChar5 = 0;
    g_fmInstruments[56].fbConn = 60;    g_fmInstruments[56].percNote = 0;
    
    // GM58: Trombone
    g_fmInstruments[57].modChar1 = 33;  g_fmInstruments[57].carChar1 = 33;
    g_fmInstruments[57].modChar2 = 148; g_fmInstruments[57].carChar2 = 5;
    g_fmInstruments[57].modChar3 = 117; g_fmInstruments[57].carChar3 = 143;
    g_fmInstruments[57].modChar4 = 23;  g_fmInstruments[57].carChar4 = 9;
    g_fmInstruments[57].modChar5 = 0;   g_fmInstruments[57].carChar5 = 0;
    g_fmInstruments[57].fbConn = 60;    g_fmInstruments[57].percNote = 0;
    
    // GM59: Tuba
    g_fmInstruments[58].modChar1 = 33;  g_fmInstruments[58].carChar1 = 97;
    g_fmInstruments[58].modChar2 = 148; g_fmInstruments[58].carChar2 = 0;
    g_fmInstruments[58].modChar3 = 118; g_fmInstruments[58].carChar3 = 130;
    g_fmInstruments[58].modChar4 = 21;  g_fmInstruments[58].carChar4 = 55;
    g_fmInstruments[58].modChar5 = 0;   g_fmInstruments[58].carChar5 = 0;
    g_fmInstruments[58].fbConn = 60;    g_fmInstruments[58].percNote = 0;
    
    // GM60: Muted Trumpet
    g_fmInstruments[59].modChar1 = 49;  g_fmInstruments[59].carChar1 = 33;
    g_fmInstruments[59].modChar2 = 67;  g_fmInstruments[59].carChar2 = 0;
    g_fmInstruments[59].modChar3 = 158; g_fmInstruments[59].carChar3 = 98;
    g_fmInstruments[59].modChar4 = 23;  g_fmInstruments[59].carChar4 = 44;
    g_fmInstruments[59].modChar5 = 1;   g_fmInstruments[59].carChar5 = 1;
    g_fmInstruments[59].fbConn = 50;    g_fmInstruments[59].percNote = 0;
    
    // GM61: French Horn
    g_fmInstruments[60].modChar1 = 33;  g_fmInstruments[60].carChar1 = 33;
    g_fmInstruments[60].modChar2 = 155; g_fmInstruments[60].carChar2 = 0;
    g_fmInstruments[60].modChar3 = 97;  g_fmInstruments[60].carChar3 = 127;
    g_fmInstruments[60].modChar4 = 106; g_fmInstruments[60].carChar4 = 10;
    g_fmInstruments[60].modChar5 = 0;   g_fmInstruments[60].carChar5 = 0;
    g_fmInstruments[60].fbConn = 50;    g_fmInstruments[60].percNote = 0;
    
    // GM62: Brass Section
    g_fmInstruments[61].modChar1 = 97;  g_fmInstruments[61].carChar1 = 34;
    g_fmInstruments[61].modChar2 = 138; g_fmInstruments[61].carChar2 = 6;
    g_fmInstruments[61].modChar3 = 117; g_fmInstruments[61].carChar3 = 116;
    g_fmInstruments[61].modChar4 = 31;  g_fmInstruments[61].carChar4 = 15;
    g_fmInstruments[61].modChar5 = 0;   g_fmInstruments[61].carChar5 = 0;
    g_fmInstruments[61].fbConn = 56;    g_fmInstruments[61].percNote = 0;
    
    // GM63: Synth Brass 1
    g_fmInstruments[62].modChar1 = 161; g_fmInstruments[62].carChar1 = 33;
    g_fmInstruments[62].modChar2 = 134; g_fmInstruments[62].carChar2 = 131;
    g_fmInstruments[62].modChar3 = 114; g_fmInstruments[62].carChar3 = 113;
    g_fmInstruments[62].modChar4 = 85;  g_fmInstruments[62].carChar4 = 24;
    g_fmInstruments[62].modChar5 = 1;   g_fmInstruments[62].carChar5 = 0;
    g_fmInstruments[62].fbConn = 48;    g_fmInstruments[62].percNote = 0;
    
    // GM64: Synth Brass 2
    g_fmInstruments[63].modChar1 = 33;  g_fmInstruments[63].carChar1 = 33;
    g_fmInstruments[63].modChar2 = 77;  g_fmInstruments[63].carChar2 = 0;
    g_fmInstruments[63].modChar3 = 84;  g_fmInstruments[63].carChar3 = 166;
    g_fmInstruments[63].modChar4 = 60;  g_fmInstruments[63].carChar4 = 28;
    g_fmInstruments[63].modChar5 = 0;   g_fmInstruments[63].carChar5 = 0;
    g_fmInstruments[63].fbConn = 56;    g_fmInstruments[63].percNote = 0;
    
    // GM65: Soprano Sax
    g_fmInstruments[64].modChar1 = 49;  g_fmInstruments[64].carChar1 = 97;
    g_fmInstruments[64].modChar2 = 143; g_fmInstruments[64].carChar2 = 0;
    g_fmInstruments[64].modChar3 = 147; g_fmInstruments[64].carChar3 = 114;
    g_fmInstruments[64].modChar4 = 2;   g_fmInstruments[64].carChar4 = 11;
    g_fmInstruments[64].modChar5 = 1;   g_fmInstruments[64].carChar5 = 0;
    g_fmInstruments[64].fbConn = 56;    g_fmInstruments[64].percNote = 0;
    
    // GM66: Alto Sax
    g_fmInstruments[65].modChar1 = 49;  g_fmInstruments[65].carChar1 = 97;
    g_fmInstruments[65].modChar2 = 142; g_fmInstruments[65].carChar2 = 0;
    g_fmInstruments[65].modChar3 = 147; g_fmInstruments[65].carChar3 = 114;
    g_fmInstruments[65].modChar4 = 3;   g_fmInstruments[65].carChar4 = 9;
    g_fmInstruments[65].modChar5 = 1;   g_fmInstruments[65].carChar5 = 0;
    g_fmInstruments[65].fbConn = 56;    g_fmInstruments[65].percNote = 0;
    
    // GM67: Tenor Sax
    g_fmInstruments[66].modChar1 = 49;  g_fmInstruments[66].carChar1 = 97;
    g_fmInstruments[66].modChar2 = 145; g_fmInstruments[66].carChar2 = 0;
    g_fmInstruments[66].modChar3 = 147; g_fmInstruments[66].carChar3 = 130;
    g_fmInstruments[66].modChar4 = 3;   g_fmInstruments[66].carChar4 = 9;
    g_fmInstruments[66].modChar5 = 1;   g_fmInstruments[66].carChar5 = 0;
    g_fmInstruments[66].fbConn = 58;    g_fmInstruments[66].percNote = 0;
    
    // GM68: Baritone Sax
    g_fmInstruments[67].modChar1 = 49;  g_fmInstruments[67].carChar1 = 97;
    g_fmInstruments[67].modChar2 = 142; g_fmInstruments[67].carChar2 = 0;
    g_fmInstruments[67].modChar3 = 147; g_fmInstruments[67].carChar3 = 114;
    g_fmInstruments[67].modChar4 = 15;  g_fmInstruments[67].carChar4 = 15;
    g_fmInstruments[67].modChar5 = 1;   g_fmInstruments[67].carChar5 = 0;
    g_fmInstruments[67].fbConn = 58;    g_fmInstruments[67].percNote = 0;
    
    // GM69: Oboe
    g_fmInstruments[68].modChar1 = 33;  g_fmInstruments[68].carChar1 = 33;
    g_fmInstruments[68].modChar2 = 75;  g_fmInstruments[68].carChar2 = 0;
    g_fmInstruments[68].modChar3 = 170; g_fmInstruments[68].carChar3 = 143;
    g_fmInstruments[68].modChar4 = 22;  g_fmInstruments[68].carChar4 = 10;
    g_fmInstruments[68].modChar5 = 1;   g_fmInstruments[68].carChar5 = 0;
    g_fmInstruments[68].fbConn = 56;    g_fmInstruments[68].percNote = 0;
    
    // GM70: English Horn
    g_fmInstruments[69].modChar1 = 49;  g_fmInstruments[69].carChar1 = 33;
    g_fmInstruments[69].modChar2 = 144; g_fmInstruments[69].carChar2 = 0;
    g_fmInstruments[69].modChar3 = 126; g_fmInstruments[69].carChar3 = 139;
    g_fmInstruments[69].modChar4 = 23;  g_fmInstruments[69].carChar4 = 12;
    g_fmInstruments[69].modChar5 = 1;   g_fmInstruments[69].carChar5 = 1;
    g_fmInstruments[69].fbConn = 54;    g_fmInstruments[69].percNote = 0;
    
    // GM71: Bassoon
    g_fmInstruments[70].modChar1 = 49;  g_fmInstruments[70].carChar1 = 50;
    g_fmInstruments[70].modChar2 = 129; g_fmInstruments[70].carChar2 = 0;
    g_fmInstruments[70].modChar3 = 117; g_fmInstruments[70].carChar3 = 97;
    g_fmInstruments[70].modChar4 = 25;  g_fmInstruments[70].carChar4 = 25;
    g_fmInstruments[70].modChar5 = 1;   g_fmInstruments[70].carChar5 = 0;
    g_fmInstruments[70].fbConn = 48;    g_fmInstruments[70].percNote = 0;
    
    // GM72: Clarinet
    g_fmInstruments[71].modChar1 = 50;  g_fmInstruments[71].carChar1 = 33;
    g_fmInstruments[71].modChar2 = 144; g_fmInstruments[71].carChar2 = 0;
    g_fmInstruments[71].modChar3 = 155; g_fmInstruments[71].carChar3 = 114;
    g_fmInstruments[71].modChar4 = 33;  g_fmInstruments[71].carChar4 = 23;
    g_fmInstruments[71].modChar5 = 0;   g_fmInstruments[71].carChar5 = 0;
    g_fmInstruments[71].fbConn = 52;    g_fmInstruments[71].percNote = 0;
    
    // GM73: Piccolo
    g_fmInstruments[72].modChar1 = 225; g_fmInstruments[72].carChar1 = 225;
    g_fmInstruments[72].modChar2 = 31;  g_fmInstruments[72].carChar2 = 0;
    g_fmInstruments[72].modChar3 = 133; g_fmInstruments[72].carChar3 = 101;
    g_fmInstruments[72].modChar4 = 95;  g_fmInstruments[72].carChar4 = 26;
    g_fmInstruments[72].modChar5 = 0;   g_fmInstruments[72].carChar5 = 0;
    g_fmInstruments[72].fbConn = 48;    g_fmInstruments[72].percNote = 0;
    
    // GM74: Flute
    g_fmInstruments[73].modChar1 = 225; g_fmInstruments[73].carChar1 = 225;
    g_fmInstruments[73].modChar2 = 70;  g_fmInstruments[73].carChar2 = 0;
    g_fmInstruments[73].modChar3 = 136; g_fmInstruments[73].carChar3 = 101;
    g_fmInstruments[73].modChar4 = 95;  g_fmInstruments[73].carChar4 = 26;
    g_fmInstruments[73].modChar5 = 0;   g_fmInstruments[73].carChar5 = 0;
    g_fmInstruments[73].fbConn = 48;    g_fmInstruments[73].percNote = 0;
    
    // GM75: Recorder
    g_fmInstruments[74].modChar1 = 161; g_fmInstruments[74].carChar1 = 33;
    g_fmInstruments[74].modChar2 = 156; g_fmInstruments[74].carChar2 = 0;
    g_fmInstruments[74].modChar3 = 117; g_fmInstruments[74].carChar3 = 117;
    g_fmInstruments[74].modChar4 = 31;  g_fmInstruments[74].carChar4 = 10;
    g_fmInstruments[74].modChar5 = 0;   g_fmInstruments[74].carChar5 = 0;
    g_fmInstruments[74].fbConn = 50;    g_fmInstruments[74].percNote = 0;
    
    // GM76: Pan Flute
    g_fmInstruments[75].modChar1 = 49;  g_fmInstruments[75].carChar1 = 33;
    g_fmInstruments[75].modChar2 = 139; g_fmInstruments[75].carChar2 = 0;
    g_fmInstruments[75].modChar3 = 132; g_fmInstruments[75].carChar3 = 101;
    g_fmInstruments[75].modChar4 = 88;  g_fmInstruments[75].carChar4 = 26;
    g_fmInstruments[75].modChar5 = 0;   g_fmInstruments[75].carChar5 = 0;
    g_fmInstruments[75].fbConn = 48;    g_fmInstruments[75].percNote = 0;
    
    // GM77: Bottle Blow
    g_fmInstruments[76].modChar1 = 225; g_fmInstruments[76].carChar1 = 161;
    g_fmInstruments[76].modChar2 = 76;  g_fmInstruments[76].carChar2 = 0;
    g_fmInstruments[76].modChar3 = 102; g_fmInstruments[76].carChar3 = 101;
    g_fmInstruments[76].modChar4 = 86;  g_fmInstruments[76].carChar4 = 38;
    g_fmInstruments[76].modChar5 = 0;   g_fmInstruments[76].carChar5 = 0;
    g_fmInstruments[76].fbConn = 48;    g_fmInstruments[76].percNote = 0;
    
    // GM78: Shakuhachi
    g_fmInstruments[77].modChar1 = 98;  g_fmInstruments[77].carChar1 = 161;
    g_fmInstruments[77].modChar2 = 203; g_fmInstruments[77].carChar2 = 0;
    g_fmInstruments[77].modChar3 = 118; g_fmInstruments[77].carChar3 = 85;
    g_fmInstruments[77].modChar4 = 70;  g_fmInstruments[77].carChar4 = 54;
    g_fmInstruments[77].modChar5 = 0;   g_fmInstruments[77].carChar5 = 0;
    g_fmInstruments[77].fbConn = 48;    g_fmInstruments[77].percNote = 0;
    
    // GM79: Whistle
    g_fmInstruments[78].modChar1 = 98;  g_fmInstruments[78].carChar1 = 161;
    g_fmInstruments[78].modChar2 = 153; g_fmInstruments[78].carChar2 = 0;
    g_fmInstruments[78].modChar3 = 87;  g_fmInstruments[78].carChar3 = 86;
    g_fmInstruments[78].modChar4 = 7;   g_fmInstruments[78].carChar4 = 7;
    g_fmInstruments[78].modChar5 = 0;   g_fmInstruments[78].carChar5 = 0;
    g_fmInstruments[78].fbConn = 59;    g_fmInstruments[78].percNote = 0;
    
    // GM80: Ocarina
    g_fmInstruments[79].modChar1 = 98;  g_fmInstruments[79].carChar1 = 161;
    g_fmInstruments[79].modChar2 = 147; g_fmInstruments[79].carChar2 = 0;
    g_fmInstruments[79].modChar3 = 119; g_fmInstruments[79].carChar3 = 118;
    g_fmInstruments[79].modChar4 = 7;   g_fmInstruments[79].carChar4 = 7;
    g_fmInstruments[79].modChar5 = 0;   g_fmInstruments[79].carChar5 = 0;
    g_fmInstruments[79].fbConn = 59;    g_fmInstruments[79].percNote = 0;
    
    // GM81: Lead 1 squareea
    g_fmInstruments[80].modChar1 = 34;  g_fmInstruments[80].carChar1 = 33;
    g_fmInstruments[80].modChar2 = 89;  g_fmInstruments[80].carChar2 = 0;
    g_fmInstruments[80].modChar3 = 255; g_fmInstruments[80].carChar3 = 255;
    g_fmInstruments[80].modChar4 = 3;   g_fmInstruments[80].carChar4 = 15;
    g_fmInstruments[80].modChar5 = 2;   g_fmInstruments[80].carChar5 = 0;
    g_fmInstruments[80].fbConn = 48;    g_fmInstruments[80].percNote = 0;
    
    // GM82: Lead 2 sawtooth
    g_fmInstruments[81].modChar1 = 33;  g_fmInstruments[81].carChar1 = 33;
    g_fmInstruments[81].modChar2 = 14;  g_fmInstruments[81].carChar2 = 0;
    g_fmInstruments[81].modChar3 = 255; g_fmInstruments[81].carChar3 = 255;
    g_fmInstruments[81].modChar4 = 15;  g_fmInstruments[81].carChar4 = 15;
    g_fmInstruments[81].modChar5 = 1;   g_fmInstruments[81].carChar5 = 1;
    g_fmInstruments[81].fbConn = 48;    g_fmInstruments[81].percNote = 0;
    
    // GM83: Lead 3 calliope
    g_fmInstruments[82].modChar1 = 34;  g_fmInstruments[82].carChar1 = 33;
    g_fmInstruments[82].modChar2 = 70;  g_fmInstruments[82].carChar2 = 128;
    g_fmInstruments[82].modChar3 = 134; g_fmInstruments[82].carChar3 = 100;
    g_fmInstruments[82].modChar4 = 85;  g_fmInstruments[82].carChar4 = 24;
    g_fmInstruments[82].modChar5 = 0;   g_fmInstruments[82].carChar5 = 0;
    g_fmInstruments[82].fbConn = 48;    g_fmInstruments[82].percNote = 0;
    
    // GM84: Lead 4 chiff
    g_fmInstruments[83].modChar1 = 33;  g_fmInstruments[83].carChar1 = 161;
    g_fmInstruments[83].modChar2 = 69;  g_fmInstruments[83].carChar2 = 0;
    g_fmInstruments[83].modChar3 = 102; g_fmInstruments[83].carChar3 = 150;
    g_fmInstruments[83].modChar4 = 18;  g_fmInstruments[83].carChar4 = 10;
    g_fmInstruments[83].modChar5 = 0;   g_fmInstruments[83].carChar5 = 0;
    g_fmInstruments[83].fbConn = 48;    g_fmInstruments[83].percNote = 0;

    // GM85: Lead 5 charang
    g_fmInstruments[84].modChar1 = 33;  g_fmInstruments[84].carChar1 = 34;
    g_fmInstruments[84].modChar2 = 139; g_fmInstruments[84].carChar2 = 0;
    g_fmInstruments[84].modChar3 = 146; g_fmInstruments[84].carChar3 = 145;
    g_fmInstruments[84].modChar4 = 42;  g_fmInstruments[84].carChar4 = 42;
    g_fmInstruments[84].modChar5 = 1;   g_fmInstruments[84].carChar5 = 0;
    g_fmInstruments[84].fbConn = 48;    g_fmInstruments[84].percNote = 0;
    
    // GM86: Lead 6 voice
    g_fmInstruments[85].modChar1 = 162; g_fmInstruments[85].carChar1 = 97;
    g_fmInstruments[85].modChar2 = 158; g_fmInstruments[85].carChar2 = 64;
    g_fmInstruments[85].modChar3 = 223; g_fmInstruments[85].carChar3 = 111;
    g_fmInstruments[85].modChar4 = 5;   g_fmInstruments[85].carChar4 = 7;
    g_fmInstruments[85].modChar5 = 0;   g_fmInstruments[85].carChar5 = 0;
    g_fmInstruments[85].fbConn = 50;    g_fmInstruments[85].percNote = 0;
    
    // GM87: Lead 7 fifths
    g_fmInstruments[86].modChar1 = 32;  g_fmInstruments[86].carChar1 = 96;
    g_fmInstruments[86].modChar2 = 26;  g_fmInstruments[86].carChar2 = 0;
    g_fmInstruments[86].modChar3 = 239; g_fmInstruments[86].carChar3 = 143;
    g_fmInstruments[86].modChar4 = 1;   g_fmInstruments[86].carChar4 = 6;
    g_fmInstruments[86].modChar5 = 0;   g_fmInstruments[86].carChar5 = 2;
    g_fmInstruments[86].fbConn = 48;    g_fmInstruments[86].percNote = 0;
    
    // GM88: Lead 8 brass
    g_fmInstruments[87].modChar1 = 33;  g_fmInstruments[87].carChar1 = 33;
    g_fmInstruments[87].modChar2 = 143; g_fmInstruments[87].carChar2 = 128;
    g_fmInstruments[87].modChar3 = 241; g_fmInstruments[87].carChar3 = 244;
    g_fmInstruments[87].modChar4 = 41;  g_fmInstruments[87].carChar4 = 9;
    g_fmInstruments[87].modChar5 = 0;   g_fmInstruments[87].carChar5 = 0;
    g_fmInstruments[87].fbConn = 58;    g_fmInstruments[87].percNote = 0;
    
    // GM89: Pad 1 new age
    g_fmInstruments[88].modChar1 = 119; g_fmInstruments[88].carChar1 = 161;
    g_fmInstruments[88].modChar2 = 165; g_fmInstruments[88].carChar2 = 0;
    g_fmInstruments[88].modChar3 = 83;  g_fmInstruments[88].carChar3 = 160;
    g_fmInstruments[88].modChar4 = 148; g_fmInstruments[88].carChar4 = 5;
    g_fmInstruments[88].modChar5 = 0;   g_fmInstruments[88].carChar5 = 0;
    g_fmInstruments[88].fbConn = 50;    g_fmInstruments[88].percNote = 0;
    
    // GM90: Pad 2 warm
    g_fmInstruments[89].modChar1 = 97;  g_fmInstruments[89].carChar1 = 177;
    g_fmInstruments[89].modChar2 = 31;  g_fmInstruments[89].carChar2 = 128;
    g_fmInstruments[89].modChar3 = 168; g_fmInstruments[89].carChar3 = 37;
    g_fmInstruments[89].modChar4 = 17;  g_fmInstruments[89].carChar4 = 3;
    g_fmInstruments[89].modChar5 = 0;   g_fmInstruments[89].carChar5 = 0;
    g_fmInstruments[89].fbConn = 58;    g_fmInstruments[89].percNote = 0;
    
    // GM91: Pad 3 polysynth
    g_fmInstruments[90].modChar1 = 97;  g_fmInstruments[90].carChar1 = 97;
    g_fmInstruments[90].modChar2 = 23;  g_fmInstruments[90].carChar2 = 0;
    g_fmInstruments[90].modChar3 = 145; g_fmInstruments[90].carChar3 = 85;
    g_fmInstruments[90].modChar4 = 52;  g_fmInstruments[90].carChar4 = 22;
    g_fmInstruments[90].modChar5 = 0;   g_fmInstruments[90].carChar5 = 0;
    g_fmInstruments[90].fbConn = 60;    g_fmInstruments[90].percNote = 0;
    
    // GM92: Pad 4 choir
    g_fmInstruments[91].modChar1 = 113; g_fmInstruments[91].carChar1 = 114;
    g_fmInstruments[91].modChar2 = 93;  g_fmInstruments[91].carChar2 = 0;
    g_fmInstruments[91].modChar3 = 84;  g_fmInstruments[91].carChar3 = 106;
    g_fmInstruments[91].modChar4 = 1;   g_fmInstruments[91].carChar4 = 3;
    g_fmInstruments[91].modChar5 = 0;   g_fmInstruments[91].carChar5 = 0;
    g_fmInstruments[91].fbConn = 48;    g_fmInstruments[91].percNote = 0;
    
    // GM93: Pad 5 bowedpad
    g_fmInstruments[92].modChar1 = 33;  g_fmInstruments[92].carChar1 = 162;
    g_fmInstruments[92].modChar2 = 151; g_fmInstruments[92].carChar2 = 0;
    g_fmInstruments[92].modChar3 = 33;  g_fmInstruments[92].carChar3 = 66;
    g_fmInstruments[92].modChar4 = 67;  g_fmInstruments[92].carChar4 = 53;
    g_fmInstruments[92].modChar5 = 0;   g_fmInstruments[92].carChar5 = 0;
    g_fmInstruments[92].fbConn = 56;    g_fmInstruments[92].percNote = 0;
    
    // GM94: Pad 6 metallic
    g_fmInstruments[93].modChar1 = 161; g_fmInstruments[93].carChar1 = 33;
    g_fmInstruments[93].modChar2 = 28;  g_fmInstruments[93].carChar2 = 0;
    g_fmInstruments[93].modChar3 = 161; g_fmInstruments[93].carChar3 = 49;
    g_fmInstruments[93].modChar4 = 119; g_fmInstruments[93].carChar4 = 71;
    g_fmInstruments[93].modChar5 = 1;   g_fmInstruments[93].carChar5 = 1;
    g_fmInstruments[93].fbConn = 48;    g_fmInstruments[93].percNote = 0;
    
    // GM95: Pad 7 halo
    g_fmInstruments[94].modChar1 = 33;  g_fmInstruments[94].carChar1 = 97;
    g_fmInstruments[94].modChar2 = 137; g_fmInstruments[94].carChar2 = 3;
    g_fmInstruments[94].modChar3 = 17;  g_fmInstruments[94].carChar3 = 66;
    g_fmInstruments[94].modChar4 = 51;  g_fmInstruments[94].carChar4 = 37;
    g_fmInstruments[94].modChar5 = 0;   g_fmInstruments[94].carChar5 = 0;
    g_fmInstruments[94].fbConn = 58;    g_fmInstruments[94].percNote = 0;
    
    // GM96: Pad 8 sweep
    g_fmInstruments[95].modChar1 = 161; g_fmInstruments[95].carChar1 = 33;
    g_fmInstruments[95].modChar2 = 21;  g_fmInstruments[95].carChar2 = 0;
    g_fmInstruments[95].modChar3 = 17;  g_fmInstruments[95].carChar3 = 207;
    g_fmInstruments[95].modChar4 = 71;  g_fmInstruments[95].carChar4 = 7;
    g_fmInstruments[95].modChar5 = 1;   g_fmInstruments[95].carChar5 = 0;
    g_fmInstruments[95].fbConn = 48;    g_fmInstruments[95].percNote = 0;
    
    // GM97: FX 1 rain
    g_fmInstruments[96].modChar1 = 58;  g_fmInstruments[96].carChar1 = 81;
    g_fmInstruments[96].modChar2 = 206; g_fmInstruments[96].carChar2 = 0;
    g_fmInstruments[96].modChar3 = 248; g_fmInstruments[96].carChar3 = 134;
    g_fmInstruments[96].modChar4 = 246; g_fmInstruments[96].carChar4 = 2;
    g_fmInstruments[96].modChar5 = 0;   g_fmInstruments[96].carChar5 = 0;
    g_fmInstruments[96].fbConn = 50;    g_fmInstruments[96].percNote = 0;
    
    // GM98: FX 2 soundtrack
    g_fmInstruments[97].modChar1 = 33;  g_fmInstruments[97].carChar1 = 33;
    g_fmInstruments[97].modChar2 = 21;  g_fmInstruments[97].carChar2 = 0;
    g_fmInstruments[97].modChar3 = 33;  g_fmInstruments[97].carChar3 = 65;
    g_fmInstruments[97].modChar4 = 35;  g_fmInstruments[97].carChar4 = 19;
    g_fmInstruments[97].modChar5 = 1;   g_fmInstruments[97].carChar5 = 0;
    g_fmInstruments[97].fbConn = 48;    g_fmInstruments[97].percNote = 0;
    
    // GM99: FX 3 crystal
    g_fmInstruments[98].modChar1 = 6;   g_fmInstruments[98].carChar1 = 1;
    g_fmInstruments[98].modChar2 = 91;  g_fmInstruments[98].carChar2 = 0;
    g_fmInstruments[98].modChar3 = 116; g_fmInstruments[98].carChar3 = 165;
    g_fmInstruments[98].modChar4 = 149; g_fmInstruments[98].carChar4 = 114;
    g_fmInstruments[98].modChar5 = 0;   g_fmInstruments[98].carChar5 = 0;
    g_fmInstruments[98].fbConn = 48;    g_fmInstruments[98].percNote = 0;
    
    // GM100: FX 4 atmosphere
    g_fmInstruments[99].modChar1 = 34;  g_fmInstruments[99].carChar1 = 97;
    g_fmInstruments[99].modChar2 = 146; g_fmInstruments[99].carChar2 = 131;
    g_fmInstruments[99].modChar3 = 177; g_fmInstruments[99].carChar3 = 242;
    g_fmInstruments[99].modChar4 = 129; g_fmInstruments[99].carChar4 = 38;
    g_fmInstruments[99].modChar5 = 0;   g_fmInstruments[99].carChar5 = 0;
    g_fmInstruments[99].fbConn = 60;    g_fmInstruments[99].percNote = 0;
    
    // GM101: FX 5 brightness
    g_fmInstruments[100].modChar1 = 65;  g_fmInstruments[100].carChar1 = 66;
    g_fmInstruments[100].modChar2 = 77;  g_fmInstruments[100].carChar2 = 0;
    g_fmInstruments[100].modChar3 = 241; g_fmInstruments[100].carChar3 = 242;
    g_fmInstruments[100].modChar4 = 81;  g_fmInstruments[100].carChar4 = 245;
    g_fmInstruments[100].modChar5 = 1;   g_fmInstruments[100].carChar5 = 0;
    g_fmInstruments[100].fbConn = 48;    g_fmInstruments[100].percNote = 0;
    
    // GM102: FX 6 goblins
    g_fmInstruments[101].modChar1 = 97;  g_fmInstruments[101].carChar1 = 163;
    g_fmInstruments[101].modChar2 = 148; g_fmInstruments[101].carChar2 = 128;
    g_fmInstruments[101].modChar3 = 17;  g_fmInstruments[101].carChar3 = 17;
    g_fmInstruments[101].modChar4 = 81;  g_fmInstruments[101].carChar4 = 19;
    g_fmInstruments[101].modChar5 = 1;   g_fmInstruments[101].carChar5 = 0;
    g_fmInstruments[101].fbConn = 54;    g_fmInstruments[101].percNote = 0;
    
    // GM103: FX 7 echoes
    g_fmInstruments[102].modChar1 = 97;  g_fmInstruments[102].carChar1 = 161;
    g_fmInstruments[102].modChar2 = 140; g_fmInstruments[102].carChar2 = 128;
    g_fmInstruments[102].modChar3 = 17;  g_fmInstruments[102].carChar3 = 29;
    g_fmInstruments[102].modChar4 = 49;  g_fmInstruments[102].carChar4 = 3;
    g_fmInstruments[102].modChar5 = 0;   g_fmInstruments[102].carChar5 = 0;
    g_fmInstruments[102].fbConn = 54;    g_fmInstruments[102].percNote = 0;
    
    // GM104: FX 8 sci-fi
    g_fmInstruments[103].modChar1 = 164; g_fmInstruments[103].carChar1 = 97;
    g_fmInstruments[103].modChar2 = 76;  g_fmInstruments[103].carChar2 = 0;
    g_fmInstruments[103].modChar3 = 243; g_fmInstruments[103].carChar3 = 129;
    g_fmInstruments[103].modChar4 = 115; g_fmInstruments[103].carChar4 = 35;
    g_fmInstruments[103].modChar5 = 1;   g_fmInstruments[103].carChar5 = 0;
    g_fmInstruments[103].fbConn = 52;    g_fmInstruments[103].percNote = 0;
    
    // GM105: Sitar
    g_fmInstruments[104].modChar1 = 2;   g_fmInstruments[104].carChar1 = 7;
    g_fmInstruments[104].modChar2 = 133; g_fmInstruments[104].carChar2 = 3;
    g_fmInstruments[104].modChar3 = 210; g_fmInstruments[104].carChar3 = 242;
    g_fmInstruments[104].modChar4 = 83;  g_fmInstruments[104].carChar4 = 246;
    g_fmInstruments[104].modChar5 = 0;   g_fmInstruments[104].carChar5 = 1;
    g_fmInstruments[104].fbConn = 48;    g_fmInstruments[104].percNote = 0;
    
    // GM106: Banjo
    g_fmInstruments[105].modChar1 = 17;  g_fmInstruments[105].carChar1 = 19;
    g_fmInstruments[105].modChar2 = 12;  g_fmInstruments[105].carChar2 = 128;
    g_fmInstruments[105].modChar3 = 163; g_fmInstruments[105].carChar3 = 162;
    g_fmInstruments[105].modChar4 = 17;  g_fmInstruments[105].carChar4 = 229;
    g_fmInstruments[105].modChar5 = 1;   g_fmInstruments[105].carChar5 = 0;
    g_fmInstruments[105].fbConn = 48;    g_fmInstruments[105].percNote = 0;
    
    // GM107: Shamisen
    g_fmInstruments[106].modChar1 = 17;  g_fmInstruments[106].carChar1 = 17;
    g_fmInstruments[106].modChar2 = 6;   g_fmInstruments[106].carChar2 = 0;
    g_fmInstruments[106].modChar3 = 246; g_fmInstruments[106].carChar3 = 242;
    g_fmInstruments[106].modChar4 = 65;  g_fmInstruments[106].carChar4 = 230;
    g_fmInstruments[106].modChar5 = 1;   g_fmInstruments[106].carChar5 = 2;
    g_fmInstruments[106].fbConn = 52;    g_fmInstruments[106].percNote = 0;
    
    // GM108: Koto
    g_fmInstruments[107].modChar1 = 147; g_fmInstruments[107].carChar1 = 145;
    g_fmInstruments[107].modChar2 = 145; g_fmInstruments[107].carChar2 = 0;
    g_fmInstruments[107].modChar3 = 212; g_fmInstruments[107].carChar3 = 235;
    g_fmInstruments[107].modChar4 = 50;  g_fmInstruments[107].carChar4 = 17;
    g_fmInstruments[107].modChar5 = 0;   g_fmInstruments[107].carChar5 = 1;
    g_fmInstruments[107].fbConn = 56;    g_fmInstruments[107].percNote = 0;
    
    // GM109: Kalimba
    g_fmInstruments[108].modChar1 = 4;   g_fmInstruments[108].carChar1 = 1;
    g_fmInstruments[108].modChar2 = 79;  g_fmInstruments[108].carChar2 = 0;
    g_fmInstruments[108].modChar3 = 250; g_fmInstruments[108].carChar3 = 194;
    g_fmInstruments[108].modChar4 = 86;  g_fmInstruments[108].carChar4 = 5;
    g_fmInstruments[108].modChar5 = 0;   g_fmInstruments[108].carChar5 = 0;
    g_fmInstruments[108].fbConn = 60;    g_fmInstruments[108].percNote = 0;
    
    // GM110: Bagpipe
    g_fmInstruments[109].modChar1 = 33;  g_fmInstruments[109].carChar1 = 34;
    g_fmInstruments[109].modChar2 = 73;  g_fmInstruments[109].carChar2 = 0;
    g_fmInstruments[109].modChar3 = 124; g_fmInstruments[109].carChar3 = 111;
    g_fmInstruments[109].modChar4 = 32;  g_fmInstruments[109].carChar4 = 12;
    g_fmInstruments[109].modChar5 = 0;   g_fmInstruments[109].carChar5 = 1;
    g_fmInstruments[109].fbConn = 54;    g_fmInstruments[109].percNote = 0;
    
    // GM111: Fiddle
    g_fmInstruments[110].modChar1 = 49;  g_fmInstruments[110].carChar1 = 33;
    g_fmInstruments[110].modChar2 = 133; g_fmInstruments[110].carChar2 = 0;
    g_fmInstruments[110].modChar3 = 221; g_fmInstruments[110].carChar3 = 86;
    g_fmInstruments[110].modChar4 = 51;  g_fmInstruments[110].carChar4 = 22;
    g_fmInstruments[110].modChar5 = 1;   g_fmInstruments[110].carChar5 = 0;
    g_fmInstruments[110].fbConn = 58;    g_fmInstruments[110].percNote = 0;
    
    // GM112: Shanai
    g_fmInstruments[111].modChar1 = 32;  g_fmInstruments[111].carChar1 = 33;
    g_fmInstruments[111].modChar2 = 4;   g_fmInstruments[111].carChar2 = 129;
    g_fmInstruments[111].modChar3 = 218; g_fmInstruments[111].carChar3 = 143;
    g_fmInstruments[111].modChar4 = 5;   g_fmInstruments[111].carChar4 = 11;
    g_fmInstruments[111].modChar5 = 2;   g_fmInstruments[111].carChar5 = 0;
    g_fmInstruments[111].fbConn = 54;    g_fmInstruments[111].percNote = 0;
    
    // GM113: Tinkle Bell
    g_fmInstruments[112].modChar1 = 5;   g_fmInstruments[112].carChar1 = 3;
    g_fmInstruments[112].modChar2 = 106; g_fmInstruments[112].carChar2 = 128;
    g_fmInstruments[112].modChar3 = 241; g_fmInstruments[112].carChar3 = 195;
    g_fmInstruments[112].modChar4 = 229; g_fmInstruments[112].carChar4 = 229;
    g_fmInstruments[112].modChar5 = 0;   g_fmInstruments[112].carChar5 = 0;
    g_fmInstruments[112].fbConn = 54;    g_fmInstruments[112].percNote = 0;
    
    // GM114: Agogo Bells
    g_fmInstruments[113].modChar1 = 7;   g_fmInstruments[113].carChar1 = 2;
    g_fmInstruments[113].modChar2 = 21;  g_fmInstruments[113].carChar2 = 0;
    g_fmInstruments[113].modChar3 = 236; g_fmInstruments[113].carChar3 = 248;
    g_fmInstruments[113].modChar4 = 38;  g_fmInstruments[113].carChar4 = 22;
    g_fmInstruments[113].modChar5 = 0;   g_fmInstruments[113].carChar5 = 0;
    g_fmInstruments[113].fbConn = 58;    g_fmInstruments[113].percNote = 0;
    
    // GM115: Steel Drums
    g_fmInstruments[114].modChar1 = 5;   g_fmInstruments[114].carChar1 = 1;
    g_fmInstruments[114].modChar2 = 157; g_fmInstruments[114].carChar2 = 0;
    g_fmInstruments[114].modChar3 = 103; g_fmInstruments[114].carChar3 = 223;
    g_fmInstruments[114].modChar4 = 53;  g_fmInstruments[114].carChar4 = 5;
    g_fmInstruments[114].modChar5 = 0;   g_fmInstruments[114].carChar5 = 0;
    g_fmInstruments[114].fbConn = 56;    g_fmInstruments[114].percNote = 0;
    
    // GM116: Woodblock
    g_fmInstruments[115].modChar1 = 24;  g_fmInstruments[115].carChar1 = 18;
    g_fmInstruments[115].modChar2 = 150; g_fmInstruments[115].carChar2 = 0;
    g_fmInstruments[115].modChar3 = 250; g_fmInstruments[115].carChar3 = 248;
    g_fmInstruments[115].modChar4 = 40;  g_fmInstruments[115].carChar4 = 229;
    g_fmInstruments[115].modChar5 = 0;   g_fmInstruments[115].carChar5 = 0;
    g_fmInstruments[115].fbConn = 58;    g_fmInstruments[115].percNote = 0;
    
    // GM117: Taiko Drum
    g_fmInstruments[116].modChar1 = 16;  g_fmInstruments[116].carChar1 = 0;
    g_fmInstruments[116].modChar2 = 134; g_fmInstruments[116].carChar2 = 3;
    g_fmInstruments[116].modChar3 = 168; g_fmInstruments[116].carChar3 = 250;
    g_fmInstruments[116].modChar4 = 7;   g_fmInstruments[116].carChar4 = 3;
    g_fmInstruments[116].modChar5 = 0;   g_fmInstruments[116].carChar5 = 0;
    g_fmInstruments[116].fbConn = 54;    g_fmInstruments[116].percNote = 0;
    
    // GM118: Melodic Tom
    g_fmInstruments[117].modChar1 = 17;  g_fmInstruments[117].carChar1 = 16;
    g_fmInstruments[117].modChar2 = 65;  g_fmInstruments[117].carChar2 = 3;
    g_fmInstruments[117].modChar3 = 248; g_fmInstruments[117].carChar3 = 243;
    g_fmInstruments[117].modChar4 = 71;  g_fmInstruments[117].carChar4 = 3;
    g_fmInstruments[117].modChar5 = 2;   g_fmInstruments[117].carChar5 = 0;
    g_fmInstruments[117].fbConn = 52;    g_fmInstruments[117].percNote = 0;
    
    // GM119: Synth Drum
    g_fmInstruments[118].modChar1 = 1;   g_fmInstruments[118].carChar1 = 16;
    g_fmInstruments[118].modChar2 = 142; g_fmInstruments[118].carChar2 = 0;
    g_fmInstruments[118].modChar3 = 241; g_fmInstruments[118].carChar3 = 243;
    g_fmInstruments[118].modChar4 = 6;   g_fmInstruments[118].carChar4 = 2;
    g_fmInstruments[118].modChar5 = 2;   g_fmInstruments[118].carChar5 = 0;
    g_fmInstruments[118].fbConn = 62;    g_fmInstruments[118].percNote = 0;
    
    // GM120: Reverse Cymbal
    g_fmInstruments[119].modChar1 = 14;  g_fmInstruments[119].carChar1 = 192;
    g_fmInstruments[119].modChar2 = 0;   g_fmInstruments[119].carChar2 = 0;
    g_fmInstruments[119].modChar3 = 31;  g_fmInstruments[119].carChar3 = 31;
    g_fmInstruments[119].modChar4 = 0;   g_fmInstruments[119].carChar4 = 255;
    g_fmInstruments[119].modChar5 = 0;   g_fmInstruments[119].carChar5 = 3;
    g_fmInstruments[119].fbConn = 62;    g_fmInstruments[119].percNote = 0;
    
    // GM121: Guitar FretNoise
    g_fmInstruments[120].modChar1 = 6;   g_fmInstruments[120].carChar1 = 3;
    g_fmInstruments[120].modChar2 = 128; g_fmInstruments[120].carChar2 = 136;
    g_fmInstruments[120].modChar3 = 248; g_fmInstruments[120].carChar3 = 86;
    g_fmInstruments[120].modChar4 = 36;  g_fmInstruments[120].carChar4 = 132;
    g_fmInstruments[120].modChar5 = 0;   g_fmInstruments[120].carChar5 = 2;
    g_fmInstruments[120].fbConn = 62;    g_fmInstruments[120].percNote = 0;
    
    // GM122: Breath Noise
    g_fmInstruments[121].modChar1 = 14;  g_fmInstruments[121].carChar1 = 208;
    g_fmInstruments[121].modChar2 = 0;   g_fmInstruments[121].carChar2 = 5;
    g_fmInstruments[121].modChar3 = 248; g_fmInstruments[121].carChar3 = 52;
    g_fmInstruments[121].modChar4 = 0;   g_fmInstruments[121].carChar4 = 4;
    g_fmInstruments[121].modChar5 = 0;   g_fmInstruments[121].carChar5 = 3;
    g_fmInstruments[121].fbConn = 62;    g_fmInstruments[121].percNote = 0;
    
    // GM123: Seashore
    g_fmInstruments[122].modChar1 = 14;  g_fmInstruments[122].carChar1 = 192;
    g_fmInstruments[122].modChar2 = 0;   g_fmInstruments[122].carChar2 = 0;
    g_fmInstruments[122].modChar3 = 246; g_fmInstruments[122].carChar3 = 31;
    g_fmInstruments[122].modChar4 = 0;   g_fmInstruments[122].carChar4 = 2;
    g_fmInstruments[122].modChar5 = 0;   g_fmInstruments[122].carChar5 = 3;
    g_fmInstruments[122].fbConn = 62;    g_fmInstruments[122].percNote = 0;
    
    // GM124: Bird Tweet
    g_fmInstruments[123].modChar1 = 213; g_fmInstruments[123].carChar1 = 218;
    g_fmInstruments[123].modChar2 = 149; g_fmInstruments[123].carChar2 = 64;
    g_fmInstruments[123].modChar3 = 55;  g_fmInstruments[123].carChar3 = 86;
    g_fmInstruments[123].modChar4 = 163; g_fmInstruments[123].carChar4 = 55;
    g_fmInstruments[123].modChar5 = 0;   g_fmInstruments[123].carChar5 = 0;
    g_fmInstruments[123].fbConn = 48;    g_fmInstruments[123].percNote = 0;
    
    // GM125: Telephone
    g_fmInstruments[124].modChar1 = 53;  g_fmInstruments[124].carChar1 = 20;
    g_fmInstruments[124].modChar2 = 92;  g_fmInstruments[124].carChar2 = 8;
    g_fmInstruments[124].modChar3 = 178; g_fmInstruments[124].carChar3 = 244;
    g_fmInstruments[124].modChar4 = 97;  g_fmInstruments[124].carChar4 = 21;
    g_fmInstruments[124].modChar5 = 2;   g_fmInstruments[124].carChar5 = 0;
    g_fmInstruments[124].fbConn = 58;    g_fmInstruments[124].percNote = 0;
    
    // GM126: Helicopter
    g_fmInstruments[125].modChar1 = 14;  g_fmInstruments[125].carChar1 = 208;
    g_fmInstruments[125].modChar2 = 0;   g_fmInstruments[125].carChar2 = 0;
    g_fmInstruments[125].modChar3 = 246; g_fmInstruments[125].carChar3 = 79;
    g_fmInstruments[125].modChar4 = 0;   g_fmInstruments[125].carChar4 = 245;
    g_fmInstruments[125].modChar5 = 0;   g_fmInstruments[125].carChar5 = 3;
    g_fmInstruments[125].fbConn = 62;    g_fmInstruments[125].percNote = 0;
    
    // GM127: Applause/Noise
    g_fmInstruments[126].modChar1 = 38;  g_fmInstruments[126].carChar1 = 228;
    g_fmInstruments[126].modChar2 = 0;   g_fmInstruments[126].carChar2 = 0;
    g_fmInstruments[126].modChar3 = 255; g_fmInstruments[126].carChar3 = 18;
    g_fmInstruments[126].modChar4 = 1;   g_fmInstruments[126].carChar4 = 22;
    g_fmInstruments[126].modChar5 = 0;   g_fmInstruments[126].carChar5 = 1;
    g_fmInstruments[126].fbConn = 62;    g_fmInstruments[126].percNote = 0;
    
    // GM128: Gunshot
    g_fmInstruments[127].modChar1 = 0;   g_fmInstruments[127].carChar1 = 0;
    g_fmInstruments[127].modChar2 = 0;   g_fmInstruments[127].carChar2 = 0;
    g_fmInstruments[127].modChar3 = 243; g_fmInstruments[127].carChar3 = 246;
    g_fmInstruments[127].modChar4 = 240; g_fmInstruments[127].carChar4 = 201;
    g_fmInstruments[127].modChar5 = 0;   g_fmInstruments[127].carChar5 = 2;
    g_fmInstruments[127].fbConn = 62;    g_fmInstruments[127].percNote = 0;
    
    // GP35: Ac Bass Drum
    g_fmInstruments[128].modChar1 = 16;  g_fmInstruments[128].carChar1 = 17;
    g_fmInstruments[128].modChar2 = 68;  g_fmInstruments[128].carChar2 = 0;
    g_fmInstruments[128].modChar3 = 248; g_fmInstruments[128].carChar3 = 243;
    g_fmInstruments[128].modChar4 = 119; g_fmInstruments[128].carChar4 = 6;
    g_fmInstruments[128].modChar5 = 2;   g_fmInstruments[128].carChar5 = 0;
    g_fmInstruments[128].fbConn = 56;    g_fmInstruments[128].percNote = 35;
    
    // GP36: Bass Drum 1
    g_fmInstruments[129].modChar1 = 16;  g_fmInstruments[129].carChar1 = 17;
    g_fmInstruments[129].modChar2 = 68;  g_fmInstruments[129].carChar2 = 0;
    g_fmInstruments[129].modChar3 = 248; g_fmInstruments[129].carChar3 = 243;
    g_fmInstruments[129].modChar4 = 119; g_fmInstruments[129].carChar4 = 6;
    g_fmInstruments[129].modChar5 = 2;   g_fmInstruments[129].carChar5 = 0;
    g_fmInstruments[129].fbConn = 56;    g_fmInstruments[129].percNote = 35;
    
    // GP37: Side Stick
    g_fmInstruments[130].modChar1 = 2;   g_fmInstruments[130].carChar1 = 17;
    g_fmInstruments[130].modChar2 = 7;   g_fmInstruments[130].carChar2 = 0;
    g_fmInstruments[130].modChar3 = 249; g_fmInstruments[130].carChar3 = 248;
    g_fmInstruments[130].modChar4 = 255; g_fmInstruments[130].carChar4 = 255;
    g_fmInstruments[130].modChar5 = 0;   g_fmInstruments[130].carChar5 = 0;
    g_fmInstruments[130].fbConn = 56;    g_fmInstruments[130].percNote = 52;
    
    // GP38: Acoustic Snare
    g_fmInstruments[131].modChar1 = 0;   g_fmInstruments[131].carChar1 = 0;
    g_fmInstruments[131].modChar2 = 0;   g_fmInstruments[131].carChar2 = 0;
    g_fmInstruments[131].modChar3 = 252; g_fmInstruments[131].carChar3 = 250;
    g_fmInstruments[131].modChar4 = 5;   g_fmInstruments[131].carChar4 = 23;
    g_fmInstruments[131].modChar5 = 2;   g_fmInstruments[131].carChar5 = 0;
    g_fmInstruments[131].fbConn = 62;    g_fmInstruments[131].percNote = 48;
    
    // GP39: Hand Clap
    g_fmInstruments[132].modChar1 = 0;   g_fmInstruments[132].carChar1 = 1;
    g_fmInstruments[132].modChar2 = 2;   g_fmInstruments[132].carChar2 = 0;
    g_fmInstruments[132].modChar3 = 255; g_fmInstruments[132].carChar3 = 255;
    g_fmInstruments[132].modChar4 = 7;   g_fmInstruments[132].carChar4 = 8;
    g_fmInstruments[132].modChar5 = 0;   g_fmInstruments[132].carChar5 = 0;
    g_fmInstruments[132].fbConn = 48;    g_fmInstruments[132].percNote = 58;
    
    // GP40: Electric Snare
    g_fmInstruments[133].modChar1 = 0;   g_fmInstruments[133].carChar1 = 0;
    g_fmInstruments[133].modChar2 = 0;   g_fmInstruments[133].carChar2 = 0;
    g_fmInstruments[133].modChar3 = 252; g_fmInstruments[133].carChar3 = 250;
    g_fmInstruments[133].modChar4 = 5;   g_fmInstruments[133].carChar4 = 23;
    g_fmInstruments[133].modChar5 = 2;   g_fmInstruments[133].carChar5 = 0;
    g_fmInstruments[133].fbConn = 62;    g_fmInstruments[133].percNote = 60;
    
    // GP41: Low Floor Tom
    g_fmInstruments[134].modChar1 = 0;   g_fmInstruments[134].carChar1 = 0;
    g_fmInstruments[134].modChar2 = 0;   g_fmInstruments[134].carChar2 = 0;
    g_fmInstruments[134].modChar3 = 246; g_fmInstruments[134].carChar3 = 246;
    g_fmInstruments[134].modChar4 = 12;  g_fmInstruments[134].carChar4 = 6;
    g_fmInstruments[134].modChar5 = 0;   g_fmInstruments[134].carChar5 = 0;
    g_fmInstruments[134].fbConn = 52;    g_fmInstruments[134].percNote = 47;
    
    // GP42: Closed High Hat
    g_fmInstruments[135].modChar1 = 12;  g_fmInstruments[135].carChar1 = 18;
    g_fmInstruments[135].modChar2 = 0;   g_fmInstruments[135].carChar2 = 0;
    g_fmInstruments[135].modChar3 = 246; g_fmInstruments[135].carChar3 = 251;
    g_fmInstruments[135].modChar4 = 8;   g_fmInstruments[135].carChar4 = 71;
    g_fmInstruments[135].modChar5 = 0;   g_fmInstruments[135].carChar5 = 2;
    g_fmInstruments[135].fbConn = 58;    g_fmInstruments[135].percNote = 43;
    
    // GP43: High Floor Tom
    g_fmInstruments[136].modChar1 = 0;   g_fmInstruments[136].carChar1 = 0;
    g_fmInstruments[136].modChar2 = 0;   g_fmInstruments[136].carChar2 = 0;
    g_fmInstruments[136].modChar3 = 246; g_fmInstruments[136].carChar3 = 246;
    g_fmInstruments[136].modChar4 = 12;  g_fmInstruments[136].carChar4 = 6;
    g_fmInstruments[136].modChar5 = 0;   g_fmInstruments[136].carChar5 = 0;
    g_fmInstruments[136].fbConn = 52;    g_fmInstruments[136].percNote = 49;
    
    // GP44: Pedal High Hat
    g_fmInstruments[137].modChar1 = 12;  g_fmInstruments[137].carChar1 = 18;
    g_fmInstruments[137].modChar2 = 0;   g_fmInstruments[137].carChar2 = 5;
    g_fmInstruments[137].modChar3 = 246; g_fmInstruments[137].carChar3 = 123;
    g_fmInstruments[137].modChar4 = 8;   g_fmInstruments[137].carChar4 = 71;
    g_fmInstruments[137].modChar5 = 0;   g_fmInstruments[137].carChar5 = 2;
    g_fmInstruments[137].fbConn = 58;    g_fmInstruments[137].percNote = 43;
    
    // GP45: Low Tom
    g_fmInstruments[138].modChar1 = 0;   g_fmInstruments[138].carChar1 = 0;
    g_fmInstruments[138].modChar2 = 0;   g_fmInstruments[138].carChar2 = 0;
    g_fmInstruments[138].modChar3 = 246; g_fmInstruments[138].carChar3 = 246;
    g_fmInstruments[138].modChar4 = 12;  g_fmInstruments[138].carChar4 = 6;
    g_fmInstruments[138].modChar5 = 0;   g_fmInstruments[138].carChar5 = 0;
    g_fmInstruments[138].fbConn = 52;    g_fmInstruments[138].percNote = 51;
    
    // GP46: Open High Hat
    g_fmInstruments[139].modChar1 = 12;  g_fmInstruments[139].carChar1 = 18;
    g_fmInstruments[139].modChar2 = 0;   g_fmInstruments[139].carChar2 = 0;
    g_fmInstruments[139].modChar3 = 246; g_fmInstruments[139].carChar3 = 203;
    g_fmInstruments[139].modChar4 = 2;   g_fmInstruments[139].carChar4 = 67;
    g_fmInstruments[139].modChar5 = 0;   g_fmInstruments[139].carChar5 = 2;
    g_fmInstruments[139].fbConn = 58;    g_fmInstruments[139].percNote = 43;
    
    // GP47: Low-Mid Tom
    g_fmInstruments[140].modChar1 = 0;   g_fmInstruments[140].carChar1 = 0;
    g_fmInstruments[140].modChar2 = 0;   g_fmInstruments[140].carChar2 = 0;
    g_fmInstruments[140].modChar3 = 246; g_fmInstruments[140].carChar3 = 246;
    g_fmInstruments[140].modChar4 = 12;  g_fmInstruments[140].carChar4 = 6;
    g_fmInstruments[140].modChar5 = 0;   g_fmInstruments[140].carChar5 = 0;
    g_fmInstruments[140].fbConn = 52;    g_fmInstruments[140].percNote = 54;
    
    // GP48: High-Mid Tom
    g_fmInstruments[141].modChar1 = 0;   g_fmInstruments[141].carChar1 = 0;
    g_fmInstruments[141].modChar2 = 0;   g_fmInstruments[141].carChar2 = 0;
    g_fmInstruments[141].modChar3 = 246; g_fmInstruments[141].carChar3 = 246;
    g_fmInstruments[141].modChar4 = 12;  g_fmInstruments[141].carChar4 = 6;
    g_fmInstruments[141].modChar5 = 0;   g_fmInstruments[141].carChar5 = 0;
    g_fmInstruments[141].fbConn = 52;    g_fmInstruments[141].percNote = 57;
    
    // GP49: Crash Cymbal 1
    g_fmInstruments[142].modChar1 = 14;  g_fmInstruments[142].carChar1 = 208;
    g_fmInstruments[142].modChar2 = 0;   g_fmInstruments[142].carChar2 = 0;
    g_fmInstruments[142].modChar3 = 246; g_fmInstruments[142].carChar3 = 159;
    g_fmInstruments[142].modChar4 = 0;   g_fmInstruments[142].carChar4 = 2;
    g_fmInstruments[142].modChar5 = 0;   g_fmInstruments[142].carChar5 = 3;
    g_fmInstruments[142].fbConn = 62;    g_fmInstruments[142].percNote = 72;
    
    // GP50: High Tom
    g_fmInstruments[143].modChar1 = 0;   g_fmInstruments[143].carChar1 = 0;
    g_fmInstruments[143].modChar2 = 0;   g_fmInstruments[143].carChar2 = 0;
    g_fmInstruments[143].modChar3 = 246; g_fmInstruments[143].carChar3 = 246;
    g_fmInstruments[143].modChar4 = 12;  g_fmInstruments[143].carChar4 = 6;
    g_fmInstruments[143].modChar5 = 0;   g_fmInstruments[143].carChar5 = 0;
    g_fmInstruments[143].fbConn = 52;    g_fmInstruments[143].percNote = 60;
    
    // GP51: Ride Cymbal 1
    g_fmInstruments[144].modChar1 = 14;  g_fmInstruments[144].carChar1 = 7;
    g_fmInstruments[144].modChar2 = 8;   g_fmInstruments[144].carChar2 = 74;
    g_fmInstruments[144].modChar3 = 248; g_fmInstruments[144].carChar3 = 244;
    g_fmInstruments[144].modChar4 = 66;  g_fmInstruments[144].carChar4 = 228;
    g_fmInstruments[144].modChar5 = 0;   g_fmInstruments[144].carChar5 = 3;
    g_fmInstruments[144].fbConn = 62;    g_fmInstruments[144].percNote = 76;
    
    // GP52: Chinese Cymbal
    g_fmInstruments[145].modChar1 = 14;  g_fmInstruments[145].carChar1 = 208;
    g_fmInstruments[145].modChar2 = 0;   g_fmInstruments[145].carChar2 = 10;
    g_fmInstruments[145].modChar3 = 245; g_fmInstruments[145].carChar3 = 159;
    g_fmInstruments[145].modChar4 = 48;  g_fmInstruments[145].carChar4 = 2;
    g_fmInstruments[145].modChar5 = 0;   g_fmInstruments[145].carChar5 = 0;
    g_fmInstruments[145].fbConn = 62;    g_fmInstruments[145].percNote = 84;
    
    // GP53: Ride Bell
    g_fmInstruments[146].modChar1 = 14;  g_fmInstruments[146].carChar1 = 7;
    g_fmInstruments[146].modChar2 = 10;  g_fmInstruments[146].carChar2 = 93;
    g_fmInstruments[146].modChar3 = 228; g_fmInstruments[146].carChar3 = 245;
    g_fmInstruments[146].modChar4 = 228; g_fmInstruments[146].carChar4 = 229;
    g_fmInstruments[146].modChar5 = 3;   g_fmInstruments[146].carChar5 = 1;
    g_fmInstruments[146].fbConn = 54;    g_fmInstruments[146].percNote = 36;
    
    // GP54: Tambourine
    g_fmInstruments[147].modChar1 = 2;   g_fmInstruments[147].carChar1 = 5;
    g_fmInstruments[147].modChar2 = 3;   g_fmInstruments[147].carChar2 = 10;
    g_fmInstruments[147].modChar3 = 180; g_fmInstruments[147].carChar3 = 151;
    g_fmInstruments[147].modChar4 = 4;   g_fmInstruments[147].carChar4 = 247;
    g_fmInstruments[147].modChar5 = 0;   g_fmInstruments[147].carChar5 = 0;
    g_fmInstruments[147].fbConn = 62;    g_fmInstruments[147].percNote = 65;
    
    // GP55: Splash Cymbal
    g_fmInstruments[148].modChar1 = 78;  g_fmInstruments[148].carChar1 = 158;
    g_fmInstruments[148].modChar2 = 0;   g_fmInstruments[148].carChar2 = 0;
    g_fmInstruments[148].modChar3 = 246; g_fmInstruments[148].carChar3 = 159;
    g_fmInstruments[148].modChar4 = 0;   g_fmInstruments[148].carChar4 = 2;
    g_fmInstruments[148].modChar5 = 0;   g_fmInstruments[148].carChar5 = 3;
    g_fmInstruments[148].fbConn = 62;    g_fmInstruments[148].percNote = 84;
    
    // GP56: Cow Bell
    g_fmInstruments[149].modChar1 = 17;  g_fmInstruments[149].carChar1 = 16;
    g_fmInstruments[149].modChar2 = 69;  g_fmInstruments[149].carChar2 = 8;
    g_fmInstruments[149].modChar3 = 248; g_fmInstruments[149].carChar3 = 243;
    g_fmInstruments[149].modChar4 = 55;  g_fmInstruments[149].carChar4 = 5;
    g_fmInstruments[149].modChar5 = 2;   g_fmInstruments[149].carChar5 = 0;
    g_fmInstruments[149].fbConn = 56;    g_fmInstruments[149].percNote = 83;
    
    // GP57: Crash Cymbal 2
    g_fmInstruments[150].modChar1 = 14;  g_fmInstruments[150].carChar1 = 208;
    g_fmInstruments[150].modChar2 = 0;   g_fmInstruments[150].carChar2 = 0;
    g_fmInstruments[150].modChar3 = 246; g_fmInstruments[150].carChar3 = 159;
    g_fmInstruments[150].modChar4 = 0;   g_fmInstruments[150].carChar4 = 2;
    g_fmInstruments[150].modChar5 = 0;   g_fmInstruments[150].carChar5 = 3;
    g_fmInstruments[150].fbConn = 62;    g_fmInstruments[150].percNote = 84;
    
    // GP58: Vibraslap
    g_fmInstruments[151].modChar1 = 128; g_fmInstruments[151].carChar1 = 16;
    g_fmInstruments[151].modChar2 = 0;   g_fmInstruments[151].carChar2 = 13;
    g_fmInstruments[151].modChar3 = 255; g_fmInstruments[151].carChar3 = 255;
    g_fmInstruments[151].modChar4 = 3;   g_fmInstruments[151].carChar4 = 20;
    g_fmInstruments[151].modChar5 = 3;   g_fmInstruments[151].carChar5 = 0;
    g_fmInstruments[151].fbConn = 60;    g_fmInstruments[151].percNote = 24;
    
    // GP59: Ride Cymbal 2
    g_fmInstruments[152].modChar1 = 14;  g_fmInstruments[152].carChar1 = 7;
    g_fmInstruments[152].modChar2 = 8;   g_fmInstruments[152].carChar2 = 74;
    g_fmInstruments[152].modChar3 = 248; g_fmInstruments[152].carChar3 = 244;
    g_fmInstruments[152].modChar4 = 66;  g_fmInstruments[152].carChar4 = 228;
    g_fmInstruments[152].modChar5 = 0;   g_fmInstruments[152].carChar5 = 3;
    g_fmInstruments[152].fbConn = 62;    g_fmInstruments[152].percNote = 77;
    
    // GP60: High Bongo
    g_fmInstruments[153].modChar1 = 6;   g_fmInstruments[153].carChar1 = 2;
    g_fmInstruments[153].modChar2 = 11;  g_fmInstruments[153].carChar2 = 0;
    g_fmInstruments[153].modChar3 = 245; g_fmInstruments[153].carChar3 = 245;
    g_fmInstruments[153].modChar4 = 12;  g_fmInstruments[153].carChar4 = 8;
    g_fmInstruments[153].modChar5 = 0;   g_fmInstruments[153].carChar5 = 0;
    g_fmInstruments[153].fbConn = 54;    g_fmInstruments[153].percNote = 60;
    
    // GP61: Low Bongo
    g_fmInstruments[154].modChar1 = 1;   g_fmInstruments[154].carChar1 = 2;
    g_fmInstruments[154].modChar2 = 0;   g_fmInstruments[154].carChar2 = 0;
    g_fmInstruments[154].modChar3 = 250; g_fmInstruments[154].carChar3 = 200;
    g_fmInstruments[154].modChar4 = 191; g_fmInstruments[154].carChar4 = 151;
    g_fmInstruments[154].modChar5 = 0;   g_fmInstruments[154].carChar5 = 0;
    g_fmInstruments[154].fbConn = 55;    g_fmInstruments[154].percNote = 65;
    
    // GP62: Mute High Conga
    g_fmInstruments[155].modChar1 = 1;   g_fmInstruments[155].carChar1 = 1;
    g_fmInstruments[155].modChar2 = 81;  g_fmInstruments[155].carChar2 = 0;
    g_fmInstruments[155].modChar3 = 250; g_fmInstruments[155].carChar3 = 250;
    g_fmInstruments[155].modChar4 = 135; g_fmInstruments[155].carChar4 = 183;
    g_fmInstruments[155].modChar5 = 0;   g_fmInstruments[155].carChar5 = 0;
    g_fmInstruments[155].fbConn = 54;    g_fmInstruments[155].percNote = 59;

    // GP63: Open High Conga
    g_fmInstruments[156].modChar1 = 1;   g_fmInstruments[156].carChar1 = 2;
    g_fmInstruments[156].modChar2 = 84;  g_fmInstruments[156].carChar2 = 0;
    g_fmInstruments[156].modChar3 = 250; g_fmInstruments[156].carChar3 = 248;
    g_fmInstruments[156].modChar4 = 141; g_fmInstruments[156].carChar4 = 184;
    g_fmInstruments[156].modChar5 = 0;   g_fmInstruments[156].carChar5 = 0;
    g_fmInstruments[156].fbConn = 54;    g_fmInstruments[156].percNote = 51;
    
    // GP64: Low Conga
    g_fmInstruments[157].modChar1 = 1;   g_fmInstruments[157].carChar1 = 2;
    g_fmInstruments[157].modChar2 = 89;  g_fmInstruments[157].carChar2 = 0;
    g_fmInstruments[157].modChar3 = 250; g_fmInstruments[157].carChar3 = 248;
    g_fmInstruments[157].modChar4 = 136; g_fmInstruments[157].carChar4 = 182;
    g_fmInstruments[157].modChar5 = 0;   g_fmInstruments[157].carChar5 = 0;
    g_fmInstruments[157].fbConn = 54;    g_fmInstruments[157].percNote = 45;
    
    // GP65: High Timbale
    g_fmInstruments[158].modChar1 = 1;   g_fmInstruments[158].carChar1 = 0;
    g_fmInstruments[158].modChar2 = 0;   g_fmInstruments[158].carChar2 = 0;
    g_fmInstruments[158].modChar3 = 249; g_fmInstruments[158].carChar3 = 250;
    g_fmInstruments[158].modChar4 = 10;  g_fmInstruments[158].carChar4 = 6;
    g_fmInstruments[158].modChar5 = 3;   g_fmInstruments[158].carChar5 = 0;
    g_fmInstruments[158].fbConn = 62;    g_fmInstruments[158].percNote = 71;

    // GP62: Mute High Conga
    g_fmInstruments[155].modChar1 = 1;   g_fmInstruments[155].carChar1 = 1;
    g_fmInstruments[155].modChar2 = 81;  g_fmInstruments[155].carChar2 = 0;
    g_fmInstruments[155].modChar3 = 250; g_fmInstruments[155].carChar3 = 250;
    g_fmInstruments[155].modChar4 = 135; g_fmInstruments[155].carChar4 = 183;
    g_fmInstruments[155].modChar5 = 0;   g_fmInstruments[155].carChar5 = 0;
    g_fmInstruments[155].fbConn = 54;    g_fmInstruments[155].percNote = 59;
    
    // GP63: Open High Conga
    g_fmInstruments[156].modChar1 = 1;   g_fmInstruments[156].carChar1 = 2;
    g_fmInstruments[156].modChar2 = 84;  g_fmInstruments[156].carChar2 = 0;
    g_fmInstruments[156].modChar3 = 250; g_fmInstruments[156].carChar3 = 248;
    g_fmInstruments[156].modChar4 = 141; g_fmInstruments[156].carChar4 = 184;
    g_fmInstruments[156].modChar5 = 0;   g_fmInstruments[156].carChar5 = 0;
    g_fmInstruments[156].fbConn = 54;    g_fmInstruments[156].percNote = 51;
    
    // GP64: Low Conga
    g_fmInstruments[157].modChar1 = 1;   g_fmInstruments[157].carChar1 = 2;
    g_fmInstruments[157].modChar2 = 89;  g_fmInstruments[157].carChar2 = 0;
    g_fmInstruments[157].modChar3 = 250; g_fmInstruments[157].carChar3 = 248;
    g_fmInstruments[157].modChar4 = 136; g_fmInstruments[157].carChar4 = 182;
    g_fmInstruments[157].modChar5 = 0;   g_fmInstruments[157].carChar5 = 0;
    g_fmInstruments[157].fbConn = 54;    g_fmInstruments[157].percNote = 45;
    
    // GP65: High Timbale
    g_fmInstruments[158].modChar1 = 1;   g_fmInstruments[158].carChar1 = 0;
    g_fmInstruments[158].modChar2 = 0;   g_fmInstruments[158].carChar2 = 0;
    g_fmInstruments[158].modChar3 = 249; g_fmInstruments[158].carChar3 = 250;
    g_fmInstruments[158].modChar4 = 10;  g_fmInstruments[158].carChar4 = 6;
    g_fmInstruments[158].modChar5 = 3;   g_fmInstruments[158].carChar5 = 0;
    g_fmInstruments[158].fbConn = 62;    g_fmInstruments[158].percNote = 71;
    
    // GP66: Low Timbale
    g_fmInstruments[159].modChar1 = 0;   g_fmInstruments[159].carChar1 = 0;
    g_fmInstruments[159].modChar2 = 128; g_fmInstruments[159].carChar2 = 0;
    g_fmInstruments[159].modChar3 = 249; g_fmInstruments[159].carChar3 = 246;
    g_fmInstruments[159].modChar4 = 137; g_fmInstruments[159].carChar4 = 108;
    g_fmInstruments[159].modChar5 = 3;   g_fmInstruments[159].carChar5 = 0;
    g_fmInstruments[159].fbConn = 62;    g_fmInstruments[159].percNote = 60;
    
    // GP67: High Agogo
    g_fmInstruments[160].modChar1 = 3;   g_fmInstruments[160].carChar1 = 12;
    g_fmInstruments[160].modChar2 = 128; g_fmInstruments[160].carChar2 = 8;
    g_fmInstruments[160].modChar3 = 248; g_fmInstruments[160].carChar3 = 246;
    g_fmInstruments[160].modChar4 = 136; g_fmInstruments[160].carChar4 = 182;
    g_fmInstruments[160].modChar5 = 3;   g_fmInstruments[160].carChar5 = 0;
    g_fmInstruments[160].fbConn = 63;    g_fmInstruments[160].percNote = 58;
    
    // GP68: Low Agogo
    g_fmInstruments[161].modChar1 = 3;   g_fmInstruments[161].carChar1 = 12;
    g_fmInstruments[161].modChar2 = 133; g_fmInstruments[161].carChar2 = 0;
    g_fmInstruments[161].modChar3 = 248; g_fmInstruments[161].carChar3 = 246;
    g_fmInstruments[161].modChar4 = 136; g_fmInstruments[161].carChar4 = 182;
    g_fmInstruments[161].modChar5 = 3;   g_fmInstruments[161].carChar5 = 0;
    g_fmInstruments[161].fbConn = 63;    g_fmInstruments[161].percNote = 53;
    
    // GP69: Cabasa
    g_fmInstruments[162].modChar1 = 14;  g_fmInstruments[162].carChar1 = 0;
    g_fmInstruments[162].modChar2 = 64;  g_fmInstruments[162].carChar2 = 8;
    g_fmInstruments[162].modChar3 = 118; g_fmInstruments[162].carChar3 = 119;
    g_fmInstruments[162].modChar4 = 79;  g_fmInstruments[162].carChar4 = 24;
    g_fmInstruments[162].modChar5 = 0;   g_fmInstruments[162].carChar5 = 2;
    g_fmInstruments[162].fbConn = 62;    g_fmInstruments[162].percNote = 64;
    
    // GP70: Maracas
    g_fmInstruments[163].modChar1 = 14;  g_fmInstruments[163].carChar1 = 3;
    g_fmInstruments[163].modChar2 = 64;  g_fmInstruments[163].carChar2 = 0;
    g_fmInstruments[163].modChar3 = 200; g_fmInstruments[163].carChar3 = 155;
    g_fmInstruments[163].modChar4 = 73;  g_fmInstruments[163].carChar4 = 105;
    g_fmInstruments[163].modChar5 = 0;   g_fmInstruments[163].carChar5 = 2;
    g_fmInstruments[163].fbConn = 62;    g_fmInstruments[163].percNote = 71;
    
    // GP71: Short Whistle
    g_fmInstruments[164].modChar1 = 215; g_fmInstruments[164].carChar1 = 199;
    g_fmInstruments[164].modChar2 = 220; g_fmInstruments[164].carChar2 = 0;
    g_fmInstruments[164].modChar3 = 173; g_fmInstruments[164].carChar3 = 141;
    g_fmInstruments[164].modChar4 = 5;   g_fmInstruments[164].carChar4 = 5;
    g_fmInstruments[164].modChar5 = 3;   g_fmInstruments[164].carChar5 = 0;
    g_fmInstruments[164].fbConn = 62;    g_fmInstruments[164].percNote = 61;
    
    // GP72: Long Whistle
    g_fmInstruments[165].modChar1 = 215; g_fmInstruments[165].carChar1 = 199;
    g_fmInstruments[165].modChar2 = 220; g_fmInstruments[165].carChar2 = 0;
    g_fmInstruments[165].modChar3 = 168; g_fmInstruments[165].carChar3 = 136;
    g_fmInstruments[165].modChar4 = 4;   g_fmInstruments[165].carChar4 = 4;
    g_fmInstruments[165].modChar5 = 3;   g_fmInstruments[165].carChar5 = 0;
    g_fmInstruments[165].fbConn = 62;    g_fmInstruments[165].percNote = 61;
    
    // GP73: Short Guiro
    g_fmInstruments[166].modChar1 = 128; g_fmInstruments[166].carChar1 = 17;
    g_fmInstruments[166].modChar2 = 0;   g_fmInstruments[166].carChar2 = 0;
    g_fmInstruments[166].modChar3 = 246; g_fmInstruments[166].carChar3 = 103;
    g_fmInstruments[166].modChar4 = 6;   g_fmInstruments[166].carChar4 = 23;
    g_fmInstruments[166].modChar5 = 3;   g_fmInstruments[166].carChar5 = 3;
    g_fmInstruments[166].fbConn = 62;    g_fmInstruments[166].percNote = 44;
    
    // GP74: Long Guiro
    g_fmInstruments[167].modChar1 = 128; g_fmInstruments[167].carChar1 = 17;
    g_fmInstruments[167].modChar2 = 0;   g_fmInstruments[167].carChar2 = 9;
    g_fmInstruments[167].modChar3 = 245; g_fmInstruments[167].carChar3 = 70;
    g_fmInstruments[167].modChar4 = 5;   g_fmInstruments[167].carChar4 = 22;
    g_fmInstruments[167].modChar5 = 2;   g_fmInstruments[167].carChar5 = 3;
    g_fmInstruments[167].fbConn = 62;    g_fmInstruments[167].percNote = 40;
    
    // GP75: Claves
    g_fmInstruments[168].modChar1 = 6;   g_fmInstruments[168].carChar1 = 21;
    g_fmInstruments[168].modChar2 = 63;  g_fmInstruments[168].carChar2 = 0;
    g_fmInstruments[168].modChar3 = 0;   g_fmInstruments[168].carChar3 = 247;
    g_fmInstruments[168].modChar4 = 244; g_fmInstruments[168].carChar4 = 245;
    g_fmInstruments[168].modChar5 = 0;   g_fmInstruments[168].carChar5 = 0;
    g_fmInstruments[168].fbConn = 49;    g_fmInstruments[168].percNote = 69;
    
    // GP76: High Wood Block
    g_fmInstruments[169].modChar1 = 6;   g_fmInstruments[169].carChar1 = 18;
    g_fmInstruments[169].modChar2 = 63;  g_fmInstruments[169].carChar2 = 0;
    g_fmInstruments[169].modChar3 = 0;   g_fmInstruments[169].carChar3 = 247;
    g_fmInstruments[169].modChar4 = 244; g_fmInstruments[169].carChar4 = 245;
    g_fmInstruments[169].modChar5 = 3;   g_fmInstruments[169].carChar5 = 0;
    g_fmInstruments[169].fbConn = 48;    g_fmInstruments[169].percNote = 68;
    
    // GP77: Low Wood Block
    g_fmInstruments[170].modChar1 = 6;   g_fmInstruments[170].carChar1 = 18;
    g_fmInstruments[170].modChar2 = 63;  g_fmInstruments[170].carChar2 = 0;
    g_fmInstruments[170].modChar3 = 0;   g_fmInstruments[170].carChar3 = 247;
    g_fmInstruments[170].modChar4 = 244; g_fmInstruments[170].carChar4 = 245;
    g_fmInstruments[170].modChar5 = 0;   g_fmInstruments[170].carChar5 = 0;
    g_fmInstruments[170].fbConn = 49;    g_fmInstruments[170].percNote = 63;
    
    // GP78: Mute Cuica
    g_fmInstruments[171].modChar1 = 1;   g_fmInstruments[171].carChar1 = 2;
    g_fmInstruments[171].modChar2 = 88;  g_fmInstruments[171].carChar2 = 0;
    g_fmInstruments[171].modChar3 = 103; g_fmInstruments[171].carChar3 = 117;
    g_fmInstruments[171].modChar4 = 231; g_fmInstruments[171].carChar4 = 7;
    g_fmInstruments[171].modChar5 = 0;   g_fmInstruments[171].carChar5 = 0;
    g_fmInstruments[171].fbConn = 48;    g_fmInstruments[171].percNote = 74;
    
    // GP79: Open Cuica
    g_fmInstruments[172].modChar1 = 65;  g_fmInstruments[172].carChar1 = 66;
    g_fmInstruments[172].modChar2 = 69;  g_fmInstruments[172].carChar2 = 8;
    g_fmInstruments[172].modChar3 = 248; g_fmInstruments[172].carChar3 = 117;
    g_fmInstruments[172].modChar4 = 72;  g_fmInstruments[172].carChar4 = 5;
    g_fmInstruments[172].modChar5 = 0;   g_fmInstruments[172].carChar5 = 0;
    g_fmInstruments[172].fbConn = 48;    g_fmInstruments[172].percNote = 60;
    
    // GP80: Mute Triangle
    g_fmInstruments[173].modChar1 = 10;  g_fmInstruments[173].carChar1 = 30;
    g_fmInstruments[173].modChar2 = 64;  g_fmInstruments[173].carChar2 = 78;
    g_fmInstruments[173].modChar3 = 224; g_fmInstruments[173].carChar3 = 255;
    g_fmInstruments[173].modChar4 = 240; g_fmInstruments[173].carChar4 = 5;
    g_fmInstruments[173].modChar5 = 3;   g_fmInstruments[173].carChar5 = 0;
    g_fmInstruments[173].fbConn = 56;    g_fmInstruments[173].percNote = 80;
    
    // GP81: Open Triangle
    g_fmInstruments[174].modChar1 = 10;  g_fmInstruments[174].carChar1 = 30;
    g_fmInstruments[174].modChar2 = 124; g_fmInstruments[174].carChar2 = 82;
    g_fmInstruments[174].modChar3 = 224; g_fmInstruments[174].carChar3 = 255;
    g_fmInstruments[174].modChar4 = 240; g_fmInstruments[174].carChar4 = 2;
    g_fmInstruments[174].modChar5 = 3;   g_fmInstruments[174].carChar5 = 0;
    g_fmInstruments[174].fbConn = 56;    g_fmInstruments[174].percNote = 64;
    
    // GP82
    g_fmInstruments[175].modChar1 = 14;  g_fmInstruments[175].carChar1 = 0;
    g_fmInstruments[175].modChar2 = 64;  g_fmInstruments[175].carChar2 = 8;
    g_fmInstruments[175].modChar3 = 122; g_fmInstruments[175].carChar3 = 123;
    g_fmInstruments[175].modChar4 = 74;  g_fmInstruments[175].carChar4 = 27;
    g_fmInstruments[175].modChar5 = 0;   g_fmInstruments[175].carChar5 = 2;
    g_fmInstruments[175].fbConn = 62;    g_fmInstruments[175].percNote = 72;
    
    // GP83
    g_fmInstruments[176].modChar1 = 14;  g_fmInstruments[176].carChar1 = 7;
    g_fmInstruments[176].modChar2 = 10;  g_fmInstruments[176].carChar2 = 64;
    g_fmInstruments[176].modChar3 = 228; g_fmInstruments[176].carChar3 = 85;
    g_fmInstruments[176].modChar4 = 228; g_fmInstruments[176].carChar4 = 57;
    g_fmInstruments[176].modChar5 = 3;   g_fmInstruments[176].carChar5 = 1;
    g_fmInstruments[176].fbConn = 54;    g_fmInstruments[176].percNote = 73;
    
    // GP84
    g_fmInstruments[177].modChar1 = 5;   g_fmInstruments[177].carChar1 = 4;
    g_fmInstruments[177].modChar2 = 5;   g_fmInstruments[177].carChar2 = 64;
    g_fmInstruments[177].modChar3 = 249; g_fmInstruments[177].carChar3 = 214;
    g_fmInstruments[177].modChar4 = 50;  g_fmInstruments[177].carChar4 = 165;
    g_fmInstruments[177].modChar5 = 3;   g_fmInstruments[177].carChar5 = 0;
    g_fmInstruments[177].fbConn = 62;    g_fmInstruments[177].percNote = 70;
    
    // GP85
    g_fmInstruments[178].modChar1 = 2;   g_fmInstruments[178].carChar1 = 21;
    g_fmInstruments[178].modChar2 = 63;  g_fmInstruments[178].carChar2 = 0;
    g_fmInstruments[178].modChar3 = 0;   g_fmInstruments[178].carChar3 = 247;
    g_fmInstruments[178].modChar4 = 243; g_fmInstruments[178].carChar4 = 245;
    g_fmInstruments[178].modChar5 = 3;   g_fmInstruments[178].carChar5 = 0;
    g_fmInstruments[178].fbConn = 56;    g_fmInstruments[178].percNote = 68;
    
    // GP86
    g_fmInstruments[179].modChar1 = 1;   g_fmInstruments[179].carChar1 = 2;
    g_fmInstruments[179].modChar2 = 79;  g_fmInstruments[179].carChar2 = 0;
    g_fmInstruments[179].modChar3 = 250; g_fmInstruments[179].carChar3 = 248;
    g_fmInstruments[179].modChar4 = 141; g_fmInstruments[179].carChar4 = 181;
    g_fmInstruments[179].modChar5 = 0;   g_fmInstruments[179].carChar5 = 0;
    g_fmInstruments[179].fbConn = 55;    g_fmInstruments[179].percNote = 48;
    
    // GP87
    g_fmInstruments[180].modChar1 = 0;   g_fmInstruments[180].carChar1 = 0;
    g_fmInstruments[180].modChar2 = 0;   g_fmInstruments[180].carChar2 = 0;
    g_fmInstruments[180].modChar3 = 246; g_fmInstruments[180].carChar3 = 246;
    g_fmInstruments[180].modChar4 = 12;  g_fmInstruments[180].carChar4 = 6;
    g_fmInstruments[180].modChar5 = 0;   g_fmInstruments[180].carChar5 = 0;
    g_fmInstruments[180].fbConn = 52;    g_fmInstruments[180].percNote = 53;
}
