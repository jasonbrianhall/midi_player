#include "midiplayer.h"

// Initialize FM instrument data
void MidiPlayer::initFMInstruments() {
    // This function initializes the FM instrument data array
    // with 180 FM instrument definitions
    
    // GM1: Acoustic Grand Piano
    instruments[0].modChar1 = 1;   instruments[0].carChar1 = 1;
    instruments[0].modChar2 = 143; instruments[0].carChar2 = 6;
    instruments[0].modChar3 = 242; instruments[0].carChar3 = 242;
    instruments[0].modChar4 = 244; instruments[0].carChar4 = 247;
    instruments[0].modChar5 = 0;   instruments[0].carChar5 = 0;
    instruments[0].fbConn = 56;    instruments[0].percNote = 0;
    
    // GM2: Bright Acoustic Grand
    instruments[1].modChar1 = 1;   instruments[1].carChar1 = 1;
    instruments[1].modChar2 = 75;  instruments[1].carChar2 = 0;
    instruments[1].modChar3 = 242; instruments[1].carChar3 = 242;
    instruments[1].modChar4 = 244; instruments[1].carChar4 = 247;
    instruments[1].modChar5 = 0;   instruments[1].carChar5 = 0;
    instruments[1].fbConn = 56;    instruments[1].percNote = 0;
    
    // GM3: Electric Grand Piano
    instruments[2].modChar1 = 1;   instruments[2].carChar1 = 1;
    instruments[2].modChar2 = 73;  instruments[2].carChar2 = 0;
    instruments[2].modChar3 = 242; instruments[2].carChar3 = 242;
    instruments[2].modChar4 = 244; instruments[2].carChar4 = 246;
    instruments[2].modChar5 = 0;   instruments[2].carChar5 = 0;
    instruments[2].fbConn = 56;    instruments[2].percNote = 0;
    
    // GM4: Honky-tonk Piano
    instruments[3].modChar1 = 129; instruments[3].carChar1 = 65;
    instruments[3].modChar2 = 18;  instruments[3].carChar2 = 0;
    instruments[3].modChar3 = 242; instruments[3].carChar3 = 242;
    instruments[3].modChar4 = 247; instruments[3].carChar4 = 247;
    instruments[3].modChar5 = 0;   instruments[3].carChar5 = 0;
    instruments[3].fbConn = 54;    instruments[3].percNote = 0;
    
    // GM5: Rhodes Piano
    instruments[4].modChar1 = 1;   instruments[4].carChar1 = 1;
    instruments[4].modChar2 = 87;  instruments[4].carChar2 = 0;
    instruments[4].modChar3 = 241; instruments[4].carChar3 = 242;
    instruments[4].modChar4 = 247; instruments[4].carChar4 = 247;
    instruments[4].modChar5 = 0;   instruments[4].carChar5 = 0;
    instruments[4].fbConn = 48;    instruments[4].percNote = 0;
    
    // GM6: Chorused Piano
    instruments[5].modChar1 = 1;   instruments[5].carChar1 = 1;
    instruments[5].modChar2 = 147; instruments[5].carChar2 = 0;
    instruments[5].modChar3 = 241; instruments[5].carChar3 = 242;
    instruments[5].modChar4 = 247; instruments[5].carChar4 = 247;
    instruments[5].modChar5 = 0;   instruments[5].carChar5 = 0;
    instruments[5].fbConn = 48;    instruments[5].percNote = 0;
    
    // GM7: Harpsichord
    instruments[6].modChar1 = 1;   instruments[6].carChar1 = 22;
    instruments[6].modChar2 = 128; instruments[6].carChar2 = 14;
    instruments[6].modChar3 = 161; instruments[6].carChar3 = 242;
    instruments[6].modChar4 = 242; instruments[6].carChar4 = 245;
    instruments[6].modChar5 = 0;   instruments[6].carChar5 = 0;
    instruments[6].fbConn = 56;    instruments[6].percNote = 0;
    
    // GM8: Clavinet
    instruments[7].modChar1 = 1;   instruments[7].carChar1 = 1;
    instruments[7].modChar2 = 146; instruments[7].carChar2 = 0;
    instruments[7].modChar3 = 194; instruments[7].carChar3 = 194;
    instruments[7].modChar4 = 248; instruments[7].carChar4 = 248;
    instruments[7].modChar5 = 0;   instruments[7].carChar5 = 0;
    instruments[7].fbConn = 58;    instruments[7].percNote = 0;
    
    // GM9: Celesta
    instruments[8].modChar1 = 12;  instruments[8].carChar1 = 129;
    instruments[8].modChar2 = 92;  instruments[8].carChar2 = 0;
    instruments[8].modChar3 = 246; instruments[8].carChar3 = 243;
    instruments[8].modChar4 = 244; instruments[8].carChar4 = 245;
    instruments[8].modChar5 = 0;   instruments[8].carChar5 = 0;
    instruments[8].fbConn = 48;    instruments[8].percNote = 0;
    
    // GM10: Glockenspiel
    instruments[9].modChar1 = 7;   instruments[9].carChar1 = 17;
    instruments[9].modChar2 = 151; instruments[9].carChar2 = 128;
    instruments[9].modChar3 = 243; instruments[9].carChar3 = 242;
    instruments[9].modChar4 = 242; instruments[9].carChar4 = 241;
    instruments[9].modChar5 = 0;   instruments[9].carChar5 = 0;
    instruments[9].fbConn = 50;    instruments[9].percNote = 0;
    
    // GM11: Music box
    instruments[10].modChar1 = 23;  instruments[10].carChar1 = 1;
    instruments[10].modChar2 = 33;  instruments[10].carChar2 = 0;
    instruments[10].modChar3 = 84;  instruments[10].carChar3 = 244;
    instruments[10].modChar4 = 244; instruments[10].carChar4 = 244;
    instruments[10].modChar5 = 0;   instruments[10].carChar5 = 0;
    instruments[10].fbConn = 50;    instruments[10].percNote = 0;
    
    // GM12: Vibraphone
    instruments[11].modChar1 = 152; instruments[11].carChar1 = 129;
    instruments[11].modChar2 = 98;  instruments[11].carChar2 = 0;
    instruments[11].modChar3 = 243; instruments[11].carChar3 = 242;
    instruments[11].modChar4 = 246; instruments[11].carChar4 = 246;
    instruments[11].modChar5 = 0;   instruments[11].carChar5 = 0;
    instruments[11].fbConn = 48;    instruments[11].percNote = 0;
    
    // GM13: Marimba
    instruments[12].modChar1 = 24;  instruments[12].carChar1 = 1;
    instruments[12].modChar2 = 35;  instruments[12].carChar2 = 0;
    instruments[12].modChar3 = 246; instruments[12].carChar3 = 231;
    instruments[12].modChar4 = 246; instruments[12].carChar4 = 247;
    instruments[12].modChar5 = 0;   instruments[12].carChar5 = 0;
    instruments[12].fbConn = 48;    instruments[12].percNote = 0;
    
    // GM14: Xylophone
    instruments[13].modChar1 = 21;  instruments[13].carChar1 = 1;
    instruments[13].modChar2 = 145; instruments[13].carChar2 = 0;
    instruments[13].modChar3 = 246; instruments[13].carChar3 = 246;
    instruments[13].modChar4 = 246; instruments[13].carChar4 = 246;
    instruments[13].modChar5 = 0;   instruments[13].carChar5 = 0;
    instruments[13].fbConn = 52;    instruments[13].percNote = 0;
    
    // GM15: Tubular Bells
    instruments[14].modChar1 = 69;  instruments[14].carChar1 = 129;
    instruments[14].modChar2 = 89;  instruments[14].carChar2 = 128;
    instruments[14].modChar3 = 211; instruments[14].carChar3 = 163;
    instruments[14].modChar4 = 243; instruments[14].carChar4 = 243;
    instruments[14].modChar5 = 0;   instruments[14].carChar5 = 0;
    instruments[14].fbConn = 60;    instruments[14].percNote = 0;
    
    // GM16: Dulcimer
    instruments[15].modChar1 = 3;   instruments[15].carChar1 = 129;
    instruments[15].modChar2 = 73;  instruments[15].carChar2 = 128;
    instruments[15].modChar3 = 117; instruments[15].carChar3 = 181;
    instruments[15].modChar4 = 245; instruments[15].carChar4 = 245;
    instruments[15].modChar5 = 1;   instruments[15].carChar5 = 0;
    instruments[15].fbConn = 52;    instruments[15].percNote = 0;
    
    // GM17: Hammond Organ
    instruments[16].modChar1 = 113; instruments[16].carChar1 = 49;
    instruments[16].modChar2 = 146; instruments[16].carChar2 = 0;
    instruments[16].modChar3 = 246; instruments[16].carChar3 = 241;
    instruments[16].modChar4 = 20;  instruments[16].carChar4 = 7;
    instruments[16].modChar5 = 0;   instruments[16].carChar5 = 0;
    instruments[16].fbConn = 50;    instruments[16].percNote = 0;
    
    // GM18: Percussive Organ
    instruments[17].modChar1 = 114; instruments[17].carChar1 = 48;
    instruments[17].modChar2 = 20;  instruments[17].carChar2 = 0;
    instruments[17].modChar3 = 199; instruments[17].carChar3 = 199;
    instruments[17].modChar4 = 88;  instruments[17].carChar4 = 8;
    instruments[17].modChar5 = 0;   instruments[17].carChar5 = 0;
    instruments[17].fbConn = 50;    instruments[17].percNote = 0;
    
    // GM19: Rock Organ
    instruments[18].modChar1 = 112; instruments[18].carChar1 = 177;
    instruments[18].modChar2 = 68;  instruments[18].carChar2 = 0;
    instruments[18].modChar3 = 170; instruments[18].carChar3 = 138;
    instruments[18].modChar4 = 24;  instruments[18].carChar4 = 8;
    instruments[18].modChar5 = 0;   instruments[18].carChar5 = 0;
    instruments[18].fbConn = 52;    instruments[18].percNote = 0;
    
    // GM20: Church Organ
    instruments[19].modChar1 = 35;  instruments[19].carChar1 = 177;
    instruments[19].modChar2 = 147; instruments[19].carChar2 = 0;
    instruments[19].modChar3 = 151; instruments[19].carChar3 = 85;
    instruments[19].modChar4 = 35;  instruments[19].carChar4 = 20;
    instruments[19].modChar5 = 1;   instruments[19].carChar5 = 0;
    instruments[19].fbConn = 52;    instruments[19].percNote = 0;
    
    // GM21: Reed Organ
    instruments[20].modChar1 = 97;  instruments[20].carChar1 = 177;
    instruments[20].modChar2 = 19;  instruments[20].carChar2 = 128;
    instruments[20].modChar3 = 151; instruments[20].carChar3 = 85;
    instruments[20].modChar4 = 4;   instruments[20].carChar4 = 4;
    instruments[20].modChar5 = 1;   instruments[20].carChar5 = 0;
    instruments[20].fbConn = 48;    instruments[20].percNote = 0;
    
    // GM22: Accordion
    instruments[21].modChar1 = 36;  instruments[21].carChar1 = 177;
    instruments[21].modChar2 = 72;  instruments[21].carChar2 = 0;
    instruments[21].modChar3 = 152; instruments[21].carChar3 = 70;
    instruments[21].modChar4 = 42;  instruments[21].carChar4 = 26;
    instruments[21].modChar5 = 1;   instruments[21].carChar5 = 0;
    instruments[21].fbConn = 60;    instruments[21].percNote = 0;
    
    // GM23: Harmonica
    instruments[22].modChar1 = 97;  instruments[22].carChar1 = 33;
    instruments[22].modChar2 = 19;  instruments[22].carChar2 = 0;
    instruments[22].modChar3 = 145; instruments[22].carChar3 = 97;
    instruments[22].modChar4 = 6;   instruments[22].carChar4 = 7;
    instruments[22].modChar5 = 1;   instruments[22].carChar5 = 0;
    instruments[22].fbConn = 58;    instruments[22].percNote = 0;
    
    // GM24: Tango Accordion
    instruments[23].modChar1 = 33;  instruments[23].carChar1 = 161;
    instruments[23].modChar2 = 19;  instruments[23].carChar2 = 137;
    instruments[23].modChar3 = 113; instruments[23].carChar3 = 97;
    instruments[23].modChar4 = 6;   instruments[23].carChar4 = 7;
    instruments[23].modChar5 = 0;   instruments[23].carChar5 = 0;
    instruments[23].fbConn = 54;    instruments[23].percNote = 0;
    
    // GM25: Acoustic Guitar1
    instruments[24].modChar1 = 2;   instruments[24].carChar1 = 65;
    instruments[24].modChar2 = 156; instruments[24].carChar2 = 128;
    instruments[24].modChar3 = 243; instruments[24].carChar3 = 243;
    instruments[24].modChar4 = 148; instruments[24].carChar4 = 200;
    instruments[24].modChar5 = 1;   instruments[24].carChar5 = 0;
    instruments[24].fbConn = 60;    instruments[24].percNote = 0;
    
    // GM26: Acoustic Guitar2
    instruments[25].modChar1 = 3;   instruments[25].carChar1 = 17;
    instruments[25].modChar2 = 84;  instruments[25].carChar2 = 0;
    instruments[25].modChar3 = 243; instruments[25].carChar3 = 241;
    instruments[25].modChar4 = 154; instruments[25].carChar4 = 231;
    instruments[25].modChar5 = 1;   instruments[25].carChar5 = 0;
    instruments[25].fbConn = 60;    instruments[25].percNote = 0;
    
    // GM27: Electric Guitar1
    instruments[26].modChar1 = 35;  instruments[26].carChar1 = 33;
    instruments[26].modChar2 = 95;  instruments[26].carChar2 = 0;
    instruments[26].modChar3 = 241; instruments[26].carChar3 = 242;
    instruments[26].modChar4 = 58;  instruments[26].carChar4 = 248;
    instruments[26].modChar5 = 0;   instruments[26].carChar5 = 0;
    instruments[26].fbConn = 48;    instruments[26].percNote = 0;
    
    // GM28: Electric Guitar2
    instruments[27].modChar1 = 3;   instruments[27].carChar1 = 33;
    instruments[27].modChar2 = 135; instruments[27].carChar2 = 128;
    instruments[27].modChar3 = 246; instruments[27].carChar3 = 243;
    instruments[27].modChar4 = 34;  instruments[27].carChar4 = 248;
    instruments[27].modChar5 = 1;   instruments[27].carChar5 = 0;
    instruments[27].fbConn = 54;    instruments[27].percNote = 0;
    
    // GM29: Electric Guitar3
    instruments[28].modChar1 = 3;   instruments[28].carChar1 = 33;
    instruments[28].modChar2 = 71;  instruments[28].carChar2 = 0;
    instruments[28].modChar3 = 249; instruments[28].carChar3 = 246;
    instruments[28].modChar4 = 84;  instruments[28].carChar4 = 58;
    instruments[28].modChar5 = 0;   instruments[28].carChar5 = 0;
    instruments[28].fbConn = 48;    instruments[28].percNote = 0;
    
    // GM30: Overdrive Guitar
    instruments[29].modChar1 = 35;  instruments[29].carChar1 = 33;
    instruments[29].modChar2 = 74;  instruments[29].carChar2 = 5;
    instruments[29].modChar3 = 145; instruments[29].carChar3 = 132;
    instruments[29].modChar4 = 65;  instruments[29].carChar4 = 25;
    instruments[29].modChar5 = 1;   instruments[29].carChar5 = 0;
    instruments[29].fbConn = 56;    instruments[29].percNote = 0;
    
    // GM31: Distortion Guitar
    instruments[30].modChar1 = 35;  instruments[30].carChar1 = 33;
    instruments[30].modChar2 = 74;  instruments[30].carChar2 = 0;
    instruments[30].modChar3 = 149; instruments[30].carChar3 = 148;
    instruments[30].modChar4 = 25;  instruments[30].carChar4 = 25;
    instruments[30].modChar5 = 1;   instruments[30].carChar5 = 0;
    instruments[30].fbConn = 56;    instruments[30].percNote = 0;
    
    // GM32: Guitar Harmonics
    instruments[31].modChar1 = 9;   instruments[31].carChar1 = 132;
    instruments[31].modChar2 = 161; instruments[31].carChar2 = 128;
    instruments[31].modChar3 = 32;  instruments[31].carChar3 = 209;
    instruments[31].modChar4 = 79;  instruments[31].carChar4 = 248;
    instruments[31].modChar5 = 0;   instruments[31].carChar5 = 0;
    instruments[31].fbConn = 56;    instruments[31].percNote = 0;
    
    // GM33: Acoustic Bass
    instruments[32].modChar1 = 33;  instruments[32].carChar1 = 162;
    instruments[32].modChar2 = 30;  instruments[32].carChar2 = 0;
    instruments[32].modChar3 = 148; instruments[32].carChar3 = 195;
    instruments[32].modChar4 = 6;   instruments[32].carChar4 = 166;
    instruments[32].modChar5 = 0;   instruments[32].carChar5 = 0;
    instruments[32].fbConn = 50;    instruments[32].percNote = 0;
    
    // GM34: Electric Bass 1
    instruments[33].modChar1 = 49;  instruments[33].carChar1 = 49;
    instruments[33].modChar2 = 18;  instruments[33].carChar2 = 0;
    instruments[33].modChar3 = 241; instruments[33].carChar3 = 241;
    instruments[33].modChar4 = 40;  instruments[33].carChar4 = 24;
    instruments[33].modChar5 = 0;   instruments[33].carChar5 = 0;
    instruments[33].fbConn = 58;    instruments[33].percNote = 0;
    
    // GM35: Electric Bass 2
    instruments[34].modChar1 = 49;  instruments[34].carChar1 = 49;
    instruments[34].modChar2 = 141; instruments[34].carChar2 = 0;
    instruments[34].modChar3 = 241; instruments[34].carChar3 = 241;
    instruments[34].modChar4 = 232; instruments[34].carChar4 = 120;
    instruments[34].modChar5 = 0;   instruments[34].carChar5 = 0;
    instruments[34].fbConn = 58;    instruments[34].percNote = 0;
    
    // GM36: Fretless Bass
    instruments[35].modChar1 = 49;  instruments[35].carChar1 = 50;
    instruments[35].modChar2 = 91;  instruments[35].carChar2 = 0;
    instruments[35].modChar3 = 81;  instruments[35].carChar3 = 113;
    instruments[35].modChar4 = 40;  instruments[35].carChar4 = 72;
    instruments[35].modChar5 = 0;   instruments[35].carChar5 = 0;
    instruments[35].fbConn = 60;    instruments[35].percNote = 0;
    
    // GM37: Slap Bass 1
    instruments[36].modChar1 = 1;   instruments[36].carChar1 = 33;
    instruments[36].modChar2 = 139; instruments[36].carChar2 = 64;
    instruments[36].modChar3 = 161; instruments[36].carChar3 = 242;
    instruments[36].modChar4 = 154; instruments[36].carChar4 = 223;
    instruments[36].modChar5 = 0;   instruments[36].carChar5 = 0;
    instruments[36].fbConn = 56;    instruments[36].percNote = 0;
    
    // GM38: Slap Bass 2
    instruments[37].modChar1 = 33;  instruments[37].carChar1 = 33;
    instruments[37].modChar2 = 139; instruments[37].carChar2 = 8;
    instruments[37].modChar3 = 162; instruments[37].carChar3 = 161;
    instruments[37].modChar4 = 22;  instruments[37].carChar4 = 223;
    instruments[37].modChar5 = 0;   instruments[37].carChar5 = 0;
    instruments[37].fbConn = 56;    instruments[37].percNote = 0;
    
    // GM39: Synth Bass 1
    instruments[38].modChar1 = 49;  instruments[38].carChar1 = 49;
    instruments[38].modChar2 = 139; instruments[38].carChar2 = 0;
    instruments[38].modChar3 = 244; instruments[38].carChar3 = 241;
    instruments[38].modChar4 = 232; instruments[38].carChar4 = 120;
    instruments[38].modChar5 = 0;   instruments[38].carChar5 = 0;
    instruments[38].fbConn = 58;    instruments[38].percNote = 0;
    
    // GM40: Synth Bass 2
    instruments[39].modChar1 = 49;  instruments[39].carChar1 = 49;
    instruments[39].modChar2 = 18;  instruments[39].carChar2 = 0;
    instruments[39].modChar3 = 241; instruments[39].carChar3 = 241;
    instruments[39].modChar4 = 40;  instruments[39].carChar4 = 24;
    instruments[39].modChar5 = 0;   instruments[39].carChar5 = 0;
    instruments[39].fbConn = 58;    instruments[39].percNote = 0;
    
    // GM41: Violin
    instruments[40].modChar1 = 49;  instruments[40].carChar1 = 33;
    instruments[40].modChar2 = 21;  instruments[40].carChar2 = 0;
    instruments[40].modChar3 = 221; instruments[40].carChar3 = 86;
    instruments[40].modChar4 = 19;  instruments[40].carChar4 = 38;
    instruments[40].modChar5 = 1;   instruments[40].carChar5 = 0;
    instruments[40].fbConn = 56;    instruments[40].percNote = 0;
    
    // GM42: Viola
    instruments[41].modChar1 = 49;  instruments[41].carChar1 = 33;
    instruments[41].modChar2 = 22;  instruments[41].carChar2 = 0;
    instruments[41].modChar3 = 221; instruments[41].carChar3 = 102;
    instruments[41].modChar4 = 19;  instruments[41].carChar4 = 6;
    instruments[41].modChar5 = 1;   instruments[41].carChar5 = 0;
    instruments[41].fbConn = 56;    instruments[41].percNote = 0;
    
    // GM43: Cello
    instruments[42].modChar1 = 113; instruments[42].carChar1 = 49;
    instruments[42].modChar2 = 73;  instruments[42].carChar2 = 0;
    instruments[42].modChar3 = 209; instruments[42].carChar3 = 97;
    instruments[42].modChar4 = 28;  instruments[42].carChar4 = 12;
    instruments[42].modChar5 = 1;   instruments[42].carChar5 = 0;
    instruments[42].fbConn = 56;    instruments[42].percNote = 0;
    
    // GM44: Contrabass
    instruments[43].modChar1 = 33;  instruments[43].carChar1 = 35;
    instruments[43].modChar2 = 77;  instruments[43].carChar2 = 128;
    instruments[43].modChar3 = 113; instruments[43].carChar3 = 114;
    instruments[43].modChar4 = 18;  instruments[43].carChar4 = 6;
    instruments[43].modChar5 = 1;   instruments[43].carChar5 = 0;
    instruments[43].fbConn = 50;    instruments[43].percNote = 0;
    
    // GM45: Tremulo Strings
    instruments[44].modChar1 = 241; instruments[44].carChar1 = 225;
    instruments[44].modChar2 = 64;  instruments[44].carChar2 = 0;
    instruments[44].modChar3 = 241; instruments[44].carChar3 = 111;
    instruments[44].modChar4 = 33;  instruments[44].carChar4 = 22;
    instruments[44].modChar5 = 1;   instruments[44].carChar5 = 0;
    instruments[44].fbConn = 50;    instruments[44].percNote = 0;
    
    // GM46: Pizzicato String
    instruments[45].modChar1 = 2;   instruments[45].carChar1 = 1;
    instruments[45].modChar2 = 26;  instruments[45].carChar2 = 128;
    instruments[45].modChar3 = 245; instruments[45].carChar3 = 133;
    instruments[45].modChar4 = 117; instruments[45].carChar4 = 53;
    instruments[45].modChar5 = 1;   instruments[45].carChar5 = 0;
    instruments[45].fbConn = 48;    instruments[45].percNote = 0;
    
    // GM47: Orchestral Harp
    instruments[46].modChar1 = 2;   instruments[46].carChar1 = 1;
    instruments[46].modChar2 = 29;  instruments[46].carChar2 = 128;
    instruments[46].modChar3 = 245; instruments[46].carChar3 = 243;
    instruments[46].modChar4 = 117; instruments[46].carChar4 = 244;
    instruments[46].modChar5 = 1;   instruments[46].carChar5 = 0;
    instruments[46].fbConn = 48;    instruments[46].percNote = 0;
    
    // GM48: Timpany
    instruments[47].modChar1 = 16;  instruments[47].carChar1 = 17;
    instruments[47].modChar2 = 65;  instruments[47].carChar2 = 0;
    instruments[47].modChar3 = 245; instruments[47].carChar3 = 242;
    instruments[47].modChar4 = 5;   instruments[47].carChar4 = 195;
    instruments[47].modChar5 = 1;   instruments[47].carChar5 = 0;
    instruments[47].fbConn = 50;    instruments[47].percNote = 0;
    
    // GM49: String Ensemble1
    instruments[48].modChar1 = 33;  instruments[48].carChar1 = 162;
    instruments[48].modChar2 = 155; instruments[48].carChar2 = 1;
    instruments[48].modChar3 = 177; instruments[48].carChar3 = 114;
    instruments[48].modChar4 = 37;  instruments[48].carChar4 = 8;
    instruments[48].modChar5 = 1;   instruments[48].carChar5 = 0;
    instruments[48].fbConn = 62;    instruments[48].percNote = 0;
    
    // GM50: String Ensemble2
    instruments[49].modChar1 = 161; instruments[49].carChar1 = 33;
    instruments[49].modChar2 = 152; instruments[49].carChar2 = 0;
    instruments[49].modChar3 = 127; instruments[49].carChar3 = 63;
    instruments[49].modChar4 = 3;   instruments[49].carChar4 = 7;
    instruments[49].modChar5 = 1;   instruments[49].carChar5 = 1;
    instruments[49].fbConn = 48;    instruments[49].percNote = 0;
    
    // GM51: Synth Strings 1
    instruments[50].modChar1 = 161; instruments[50].carChar1 = 97;
    instruments[50].modChar2 = 147; instruments[50].carChar2 = 0;
    instruments[50].modChar3 = 193; instruments[50].carChar3 = 79;
    instruments[50].modChar4 = 18;  instruments[50].carChar4 = 5;
    instruments[50].modChar5 = 0;   instruments[50].carChar5 = 0;
    instruments[50].fbConn = 58;    instruments[50].percNote = 0;
    
    // GM52: SynthStrings 2
    instruments[51].modChar1 = 33;  instruments[51].carChar1 = 97;
    instruments[51].modChar2 = 24;  instruments[51].carChar2 = 0;
    instruments[51].modChar3 = 193; instruments[51].carChar3 = 79;
    instruments[51].modChar4 = 34;  instruments[51].carChar4 = 5;
    instruments[51].modChar5 = 0;   instruments[51].carChar5 = 0;
    instruments[51].fbConn = 60;    instruments[51].percNote = 0;
    
    // GM53: Choir Aahs
    instruments[52].modChar1 = 49;  instruments[52].carChar1 = 114;
    instruments[52].modChar2 = 91;  instruments[52].carChar2 = 131;
    instruments[52].modChar3 = 244; instruments[52].carChar3 = 138;
    instruments[52].modChar4 = 21;  instruments[52].carChar4 = 5;
    instruments[52].modChar5 = 0;   instruments[52].carChar5 = 0;
    instruments[52].fbConn = 48;    instruments[52].percNote = 0;
    
    // GM54: Voice Oohs
    instruments[53].modChar1 = 161; instruments[53].carChar1 = 97;
    instruments[53].modChar2 = 144; instruments[53].carChar2 = 0;
    instruments[53].modChar3 = 116; instruments[53].carChar3 = 113;
    instruments[53].modChar4 = 57;  instruments[53].carChar4 = 103;
    instruments[53].modChar5 = 0;   instruments[53].carChar5 = 0;
    instruments[53].fbConn = 48;    instruments[53].percNote = 0;
    
    // GM55: Synth Voice
    instruments[54].modChar1 = 113; instruments[54].carChar1 = 114;
    instruments[54].modChar2 = 87;  instruments[54].carChar2 = 0;
    instruments[54].modChar3 = 84;  instruments[54].carChar3 = 122;
    instruments[54].modChar4 = 5;   instruments[54].carChar4 = 5;
    instruments[54].modChar5 = 0;   instruments[54].carChar5 = 0;
    instruments[54].fbConn = 60;    instruments[54].percNote = 0;
    
    // GM56: Orchestra Hit
    instruments[55].modChar1 = 144; instruments[55].carChar1 = 65;
    instruments[55].modChar2 = 0;   instruments[55].carChar2 = 0;
    instruments[55].modChar3 = 84;  instruments[55].carChar3 = 165;
    instruments[55].modChar4 = 99;  instruments[55].carChar4 = 69;
    instruments[55].modChar5 = 0;   instruments[55].carChar5 = 0;
    instruments[55].fbConn = 56;    instruments[55].percNote = 0;
    
    // GM57: Trumpet
    instruments[56].modChar1 = 33;  instruments[56].carChar1 = 33;
    instruments[56].modChar2 = 146; instruments[56].carChar2 = 1;
    instruments[56].modChar3 = 133; instruments[56].carChar3 = 143;
    instruments[56].modChar4 = 23;  instruments[56].carChar4 = 9;
    instruments[56].modChar5 = 0;   instruments[56].carChar5 = 0;
    instruments[56].fbConn = 60;    instruments[56].percNote = 0;
    
    // GM58: Trombone
    instruments[57].modChar1 = 33;  instruments[57].carChar1 = 33;
    instruments[57].modChar2 = 148; instruments[57].carChar2 = 5;
    instruments[57].modChar3 = 117; instruments[57].carChar3 = 143;
    instruments[57].modChar4 = 23;  instruments[57].carChar4 = 9;
    instruments[57].modChar5 = 0;   instruments[57].carChar5 = 0;
    instruments[57].fbConn = 60;    instruments[57].percNote = 0;
    
    // GM59: Tuba
    instruments[58].modChar1 = 33;  instruments[58].carChar1 = 97;
    instruments[58].modChar2 = 148; instruments[58].carChar2 = 0;
    instruments[58].modChar3 = 118; instruments[58].carChar3 = 130;
    instruments[58].modChar4 = 21;  instruments[58].carChar4 = 55;
    instruments[58].modChar5 = 0;   instruments[58].carChar5 = 0;
    instruments[58].fbConn = 60;    instruments[58].percNote = 0;
    
    // GM60: Muted Trumpet
    instruments[59].modChar1 = 49;  instruments[59].carChar1 = 33;
    instruments[59].modChar2 = 67;  instruments[59].carChar2 = 0;
    instruments[59].modChar3 = 158; instruments[59].carChar3 = 98;
    instruments[59].modChar4 = 23;  instruments[59].carChar4 = 44;
    instruments[59].modChar5 = 1;   instruments[59].carChar5 = 1;
    instruments[59].fbConn = 50;    instruments[59].percNote = 0;
    
    // GM61: French Horn
    instruments[60].modChar1 = 33;  instruments[60].carChar1 = 33;
    instruments[60].modChar2 = 155; instruments[60].carChar2 = 0;
    instruments[60].modChar3 = 97;  instruments[60].carChar3 = 127;
    instruments[60].modChar4 = 106; instruments[60].carChar4 = 10;
    instruments[60].modChar5 = 0;   instruments[60].carChar5 = 0;
    instruments[60].fbConn = 50;    instruments[60].percNote = 0;
    
    // GM62: Brass Section
    instruments[61].modChar1 = 97;  instruments[61].carChar1 = 34;
    instruments[61].modChar2 = 138; instruments[61].carChar2 = 6;
    instruments[61].modChar3 = 117; instruments[61].carChar3 = 116;
    instruments[61].modChar4 = 31;  instruments[61].carChar4 = 15;
    instruments[61].modChar5 = 0;   instruments[61].carChar5 = 0;
    instruments[61].fbConn = 56;    instruments[61].percNote = 0;
    
    // GM63: Synth Brass 1
    instruments[62].modChar1 = 161; instruments[62].carChar1 = 33;
    instruments[62].modChar2 = 134; instruments[62].carChar2 = 131;
    instruments[62].modChar3 = 114; instruments[62].carChar3 = 113;
    instruments[62].modChar4 = 85;  instruments[62].carChar4 = 24;
    instruments[62].modChar5 = 1;   instruments[62].carChar5 = 0;
    instruments[62].fbConn = 48;    instruments[62].percNote = 0;
    
    // GM64: Synth Brass 2
    instruments[63].modChar1 = 33;  instruments[63].carChar1 = 33;
    instruments[63].modChar2 = 77;  instruments[63].carChar2 = 0;
    instruments[63].modChar3 = 84;  instruments[63].carChar3 = 166;
    instruments[63].modChar4 = 60;  instruments[63].carChar4 = 28;
    instruments[63].modChar5 = 0;   instruments[63].carChar5 = 0;
    instruments[63].fbConn = 56;    instruments[63].percNote = 0;
    
    // GM65: Soprano Sax
    instruments[64].modChar1 = 49;  instruments[64].carChar1 = 97;
    instruments[64].modChar2 = 143; instruments[64].carChar2 = 0;
    instruments[64].modChar3 = 147; instruments[64].carChar3 = 114;
    instruments[64].modChar4 = 2;   instruments[64].carChar4 = 11;
    instruments[64].modChar5 = 1;   instruments[64].carChar5 = 0;
    instruments[64].fbConn = 56;    instruments[64].percNote = 0;
    
    // GM66: Alto Sax
    instruments[65].modChar1 = 49;  instruments[65].carChar1 = 97;
    instruments[65].modChar2 = 142; instruments[65].carChar2 = 0;
    instruments[65].modChar3 = 147; instruments[65].carChar3 = 114;
    instruments[65].modChar4 = 3;   instruments[65].carChar4 = 9;
    instruments[65].modChar5 = 1;   instruments[65].carChar5 = 0;
    instruments[65].fbConn = 56;    instruments[65].percNote = 0;
    
    // GM67: Tenor Sax
    instruments[66].modChar1 = 49;  instruments[66].carChar1 = 97;
    instruments[66].modChar2 = 145; instruments[66].carChar2 = 0;
    instruments[66].modChar3 = 147; instruments[66].carChar3 = 130;
    instruments[66].modChar4 = 3;   instruments[66].carChar4 = 9;
    instruments[66].modChar5 = 1;   instruments[66].carChar5 = 0;
    instruments[66].fbConn = 58;    instruments[66].percNote = 0;
    
    // GM68: Baritone Sax
    instruments[67].modChar1 = 49;  instruments[67].carChar1 = 97;
    instruments[67].modChar2 = 142; instruments[67].carChar2 = 0;
    instruments[67].modChar3 = 147; instruments[67].carChar3 = 114;
    instruments[67].modChar4 = 15;  instruments[67].carChar4 = 15;
    instruments[67].modChar5 = 1;   instruments[67].carChar5 = 0;
    instruments[67].fbConn = 58;    instruments[67].percNote = 0;
    
    // GM69: Oboe
    instruments[68].modChar1 = 33;  instruments[68].carChar1 = 33;
    instruments[68].modChar2 = 75;  instruments[68].carChar2 = 0;
    instruments[68].modChar3 = 170; instruments[68].carChar3 = 143;
    instruments[68].modChar4 = 22;  instruments[68].carChar4 = 10;
    instruments[68].modChar5 = 1;   instruments[68].carChar5 = 0;
    instruments[68].fbConn = 56;    instruments[68].percNote = 0;
    
    // GM70: English Horn
    instruments[69].modChar1 = 49;  instruments[69].carChar1 = 33;
    instruments[69].modChar2 = 144; instruments[69].carChar2 = 0;
    instruments[69].modChar3 = 126; instruments[69].carChar3 = 139;
    instruments[69].modChar4 = 23;  instruments[69].carChar4 = 12;
    instruments[69].modChar5 = 1;   instruments[69].carChar5 = 1;
    instruments[69].fbConn = 54;    instruments[69].percNote = 0;
    
    // GM71: Bassoon
    instruments[70].modChar1 = 49;  instruments[70].carChar1 = 50;
    instruments[70].modChar2 = 129; instruments[70].carChar2 = 0;
    instruments[70].modChar3 = 117; instruments[70].carChar3 = 97;
    instruments[70].modChar4 = 25;  instruments[70].carChar4 = 25;
    instruments[70].modChar5 = 1;   instruments[70].carChar5 = 0;
    instruments[70].fbConn = 48;    instruments[70].percNote = 0;
    
    // GM72: Clarinet
    instruments[71].modChar1 = 50;  instruments[71].carChar1 = 33;
    instruments[71].modChar2 = 144; instruments[71].carChar2 = 0;
    instruments[71].modChar3 = 155; instruments[71].carChar3 = 114;
    instruments[71].modChar4 = 33;  instruments[71].carChar4 = 23;
    instruments[71].modChar5 = 0;   instruments[71].carChar5 = 0;
    instruments[71].fbConn = 52;    instruments[71].percNote = 0;
    
    // GM73: Piccolo
    instruments[72].modChar1 = 225; instruments[72].carChar1 = 225;
    instruments[72].modChar2 = 31;  instruments[72].carChar2 = 0;
    instruments[72].modChar3 = 133; instruments[72].carChar3 = 101;
    instruments[72].modChar4 = 95;  instruments[72].carChar4 = 26;
    instruments[72].modChar5 = 0;   instruments[72].carChar5 = 0;
    instruments[72].fbConn = 48;    instruments[72].percNote = 0;
    
    // GM74: Flute
    instruments[73].modChar1 = 225; instruments[73].carChar1 = 225;
    instruments[73].modChar2 = 70;  instruments[73].carChar2 = 0;
    instruments[73].modChar3 = 136; instruments[73].carChar3 = 101;
    instruments[73].modChar4 = 95;  instruments[73].carChar4 = 26;
    instruments[73].modChar5 = 0;   instruments[73].carChar5 = 0;
    instruments[73].fbConn = 48;    instruments[73].percNote = 0;
    
    // GM75: Recorder
    instruments[74].modChar1 = 161; instruments[74].carChar1 = 33;
    instruments[74].modChar2 = 156; instruments[74].carChar2 = 0;
    instruments[74].modChar3 = 117; instruments[74].carChar3 = 117;
    instruments[74].modChar4 = 31;  instruments[74].carChar4 = 10;
    instruments[74].modChar5 = 0;   instruments[74].carChar5 = 0;
    instruments[74].fbConn = 50;    instruments[74].percNote = 0;
    
    // GM76: Pan Flute
    instruments[75].modChar1 = 49;  instruments[75].carChar1 = 33;
    instruments[75].modChar2 = 139; instruments[75].carChar2 = 0;
    instruments[75].modChar3 = 132; instruments[75].carChar3 = 101;
    instruments[75].modChar4 = 88;  instruments[75].carChar4 = 26;
    instruments[75].modChar5 = 0;   instruments[75].carChar5 = 0;
    instruments[75].fbConn = 48;    instruments[75].percNote = 0;
    
    // GM77: Bottle Blow
    instruments[76].modChar1 = 225; instruments[76].carChar1 = 161;
    instruments[76].modChar2 = 76;  instruments[76].carChar2 = 0;
    instruments[76].modChar3 = 102; instruments[76].carChar3 = 101;
    instruments[76].modChar4 = 86;  instruments[76].carChar4 = 38;
    instruments[76].modChar5 = 0;   instruments[76].carChar5 = 0;
    instruments[76].fbConn = 48;    instruments[76].percNote = 0;
    
    // GM78: Shakuhachi
    instruments[77].modChar1 = 98;  instruments[77].carChar1 = 161;
    instruments[77].modChar2 = 203; instruments[77].carChar2 = 0;
    instruments[77].modChar3 = 118; instruments[77].carChar3 = 85;
    instruments[77].modChar4 = 70;  instruments[77].carChar4 = 54;
    instruments[77].modChar5 = 0;   instruments[77].carChar5 = 0;
    instruments[77].fbConn = 48;    instruments[77].percNote = 0;
    
    // GM79: Whistle
    instruments[78].modChar1 = 98;  instruments[78].carChar1 = 161;
    instruments[78].modChar2 = 153; instruments[78].carChar2 = 0;
    instruments[78].modChar3 = 87;  instruments[78].carChar3 = 86;
    instruments[78].modChar4 = 7;   instruments[78].carChar4 = 7;
    instruments[78].modChar5 = 0;   instruments[78].carChar5 = 0;
    instruments[78].fbConn = 59;    instruments[78].percNote = 0;
    
    // GM80: Ocarina
    instruments[79].modChar1 = 98;  instruments[79].carChar1 = 161;
    instruments[79].modChar2 = 147; instruments[79].carChar2 = 0;
    instruments[79].modChar3 = 119; instruments[79].carChar3 = 118;
    instruments[79].modChar4 = 7;   instruments[79].carChar4 = 7;
    instruments[79].modChar5 = 0;   instruments[79].carChar5 = 0;
    instruments[79].fbConn = 59;    instruments[79].percNote = 0;
    
    // GM81: Lead 1 squareea
    instruments[80].modChar1 = 34;  instruments[80].carChar1 = 33;
    instruments[80].modChar2 = 89;  instruments[80].carChar2 = 0;
    instruments[80].modChar3 = 255; instruments[80].carChar3 = 255;
    instruments[80].modChar4 = 3;   instruments[80].carChar4 = 15;
    instruments[80].modChar5 = 2;   instruments[80].carChar5 = 0;
    instruments[80].fbConn = 48;    instruments[80].percNote = 0;
    
    // GM82: Lead 2 sawtooth
    instruments[81].modChar1 = 33;  instruments[81].carChar1 = 33;
    instruments[81].modChar2 = 14;  instruments[81].carChar2 = 0;
    instruments[81].modChar3 = 255; instruments[81].carChar3 = 255;
    instruments[81].modChar4 = 15;  instruments[81].carChar4 = 15;
    instruments[81].modChar5 = 1;   instruments[81].carChar5 = 1;
    instruments[81].fbConn = 48;    instruments[81].percNote = 0;
    
    // GM83: Lead 3 calliope
    instruments[82].modChar1 = 34;  instruments[82].carChar1 = 33;
    instruments[82].modChar2 = 70;  instruments[82].carChar2 = 128;
    instruments[82].modChar3 = 134; instruments[82].carChar3 = 100;
    instruments[82].modChar4 = 85;  instruments[82].carChar4 = 24;
    instruments[82].modChar5 = 0;   instruments[82].carChar5 = 0;
    instruments[82].fbConn = 48;    instruments[82].percNote = 0;
    
    // GM84: Lead 4 chiff
    instruments[83].modChar1 = 33;  instruments[83].carChar1 = 161;
    instruments[83].modChar2 = 69;  instruments[83].carChar2 = 0;
    instruments[83].modChar3 = 102; instruments[83].carChar3 = 150;
    instruments[83].modChar4 = 18;  instruments[83].carChar4 = 10;
    instruments[83].modChar5 = 0;   instruments[83].carChar5 = 0;
    instruments[83].fbConn = 48;    instruments[83].percNote = 0;

    // GM85: Lead 5 charang
    instruments[84].modChar1 = 33;  instruments[84].carChar1 = 34;
    instruments[84].modChar2 = 139; instruments[84].carChar2 = 0;
    instruments[84].modChar3 = 146; instruments[84].carChar3 = 145;
    instruments[84].modChar4 = 42;  instruments[84].carChar4 = 42;
    instruments[84].modChar5 = 1;   instruments[84].carChar5 = 0;
    instruments[84].fbConn = 48;    instruments[84].percNote = 0;
    
    // GM86: Lead 6 voice
    instruments[85].modChar1 = 162; instruments[85].carChar1 = 97;
    instruments[85].modChar2 = 158; instruments[85].carChar2 = 64;
    instruments[85].modChar3 = 223; instruments[85].carChar3 = 111;
    instruments[85].modChar4 = 5;   instruments[85].carChar4 = 7;
    instruments[85].modChar5 = 0;   instruments[85].carChar5 = 0;
    instruments[85].fbConn = 50;    instruments[85].percNote = 0;
    
    // GM87: Lead 7 fifths
    instruments[86].modChar1 = 32;  instruments[86].carChar1 = 96;
    instruments[86].modChar2 = 26;  instruments[86].carChar2 = 0;
    instruments[86].modChar3 = 239; instruments[86].carChar3 = 143;
    instruments[86].modChar4 = 1;   instruments[86].carChar4 = 6;
    instruments[86].modChar5 = 0;   instruments[86].carChar5 = 2;
    instruments[86].fbConn = 48;    instruments[86].percNote = 0;
    
    // GM88: Lead 8 brass
    instruments[87].modChar1 = 33;  instruments[87].carChar1 = 33;
    instruments[87].modChar2 = 143; instruments[87].carChar2 = 128;
    instruments[87].modChar3 = 241; instruments[87].carChar3 = 244;
    instruments[87].modChar4 = 41;  instruments[87].carChar4 = 9;
    instruments[87].modChar5 = 0;   instruments[87].carChar5 = 0;
    instruments[87].fbConn = 58;    instruments[87].percNote = 0;
    
    // GM89: Pad 1 new age
    instruments[88].modChar1 = 119; instruments[88].carChar1 = 161;
    instruments[88].modChar2 = 165; instruments[88].carChar2 = 0;
    instruments[88].modChar3 = 83;  instruments[88].carChar3 = 160;
    instruments[88].modChar4 = 148; instruments[88].carChar4 = 5;
    instruments[88].modChar5 = 0;   instruments[88].carChar5 = 0;
    instruments[88].fbConn = 50;    instruments[88].percNote = 0;
    
    // GM90: Pad 2 warm
    instruments[89].modChar1 = 97;  instruments[89].carChar1 = 177;
    instruments[89].modChar2 = 31;  instruments[89].carChar2 = 128;
    instruments[89].modChar3 = 168; instruments[89].carChar3 = 37;
    instruments[89].modChar4 = 17;  instruments[89].carChar4 = 3;
    instruments[89].modChar5 = 0;   instruments[89].carChar5 = 0;
    instruments[89].fbConn = 58;    instruments[89].percNote = 0;
    
    // GM91: Pad 3 polysynth
    instruments[90].modChar1 = 97;  instruments[90].carChar1 = 97;
    instruments[90].modChar2 = 23;  instruments[90].carChar2 = 0;
    instruments[90].modChar3 = 145; instruments[90].carChar3 = 85;
    instruments[90].modChar4 = 52;  instruments[90].carChar4 = 22;
    instruments[90].modChar5 = 0;   instruments[90].carChar5 = 0;
    instruments[90].fbConn = 60;    instruments[90].percNote = 0;
    
    // GM92: Pad 4 choir
    instruments[91].modChar1 = 113; instruments[91].carChar1 = 114;
    instruments[91].modChar2 = 93;  instruments[91].carChar2 = 0;
    instruments[91].modChar3 = 84;  instruments[91].carChar3 = 106;
    instruments[91].modChar4 = 1;   instruments[91].carChar4 = 3;
    instruments[91].modChar5 = 0;   instruments[91].carChar5 = 0;
    instruments[91].fbConn = 48;    instruments[91].percNote = 0;
    
    // GM93: Pad 5 bowedpad
    instruments[92].modChar1 = 33;  instruments[92].carChar1 = 162;
    instruments[92].modChar2 = 151; instruments[92].carChar2 = 0;
    instruments[92].modChar3 = 33;  instruments[92].carChar3 = 66;
    instruments[92].modChar4 = 67;  instruments[92].carChar4 = 53;
    instruments[92].modChar5 = 0;   instruments[92].carChar5 = 0;
    instruments[92].fbConn = 56;    instruments[92].percNote = 0;
    
    // GM94: Pad 6 metallic
    instruments[93].modChar1 = 161; instruments[93].carChar1 = 33;
    instruments[93].modChar2 = 28;  instruments[93].carChar2 = 0;
    instruments[93].modChar3 = 161; instruments[93].carChar3 = 49;
    instruments[93].modChar4 = 119; instruments[93].carChar4 = 71;
    instruments[93].modChar5 = 1;   instruments[93].carChar5 = 1;
    instruments[93].fbConn = 48;    instruments[93].percNote = 0;
    
    // GM95: Pad 7 halo
    instruments[94].modChar1 = 33;  instruments[94].carChar1 = 97;
    instruments[94].modChar2 = 137; instruments[94].carChar2 = 3;
    instruments[94].modChar3 = 17;  instruments[94].carChar3 = 66;
    instruments[94].modChar4 = 51;  instruments[94].carChar4 = 37;
    instruments[94].modChar5 = 0;   instruments[94].carChar5 = 0;
    instruments[94].fbConn = 58;    instruments[94].percNote = 0;
    
    // GM96: Pad 8 sweep
    instruments[95].modChar1 = 161; instruments[95].carChar1 = 33;
    instruments[95].modChar2 = 21;  instruments[95].carChar2 = 0;
    instruments[95].modChar3 = 17;  instruments[95].carChar3 = 207;
    instruments[95].modChar4 = 71;  instruments[95].carChar4 = 7;
    instruments[95].modChar5 = 1;   instruments[95].carChar5 = 0;
    instruments[95].fbConn = 48;    instruments[95].percNote = 0;
    
    // GM97: FX 1 rain
    instruments[96].modChar1 = 58;  instruments[96].carChar1 = 81;
    instruments[96].modChar2 = 206; instruments[96].carChar2 = 0;
    instruments[96].modChar3 = 248; instruments[96].carChar3 = 134;
    instruments[96].modChar4 = 246; instruments[96].carChar4 = 2;
    instruments[96].modChar5 = 0;   instruments[96].carChar5 = 0;
    instruments[96].fbConn = 50;    instruments[96].percNote = 0;
    
    // GM98: FX 2 soundtrack
    instruments[97].modChar1 = 33;  instruments[97].carChar1 = 33;
    instruments[97].modChar2 = 21;  instruments[97].carChar2 = 0;
    instruments[97].modChar3 = 33;  instruments[97].carChar3 = 65;
    instruments[97].modChar4 = 35;  instruments[97].carChar4 = 19;
    instruments[97].modChar5 = 1;   instruments[97].carChar5 = 0;
    instruments[97].fbConn = 48;    instruments[97].percNote = 0;
    
    // GM99: FX 3 crystal
    instruments[98].modChar1 = 6;   instruments[98].carChar1 = 1;
    instruments[98].modChar2 = 91;  instruments[98].carChar2 = 0;
    instruments[98].modChar3 = 116; instruments[98].carChar3 = 165;
    instruments[98].modChar4 = 149; instruments[98].carChar4 = 114;
    instruments[98].modChar5 = 0;   instruments[98].carChar5 = 0;
    instruments[98].fbConn = 48;    instruments[98].percNote = 0;
    
    // GM100: FX 4 atmosphere
    instruments[99].modChar1 = 34;  instruments[99].carChar1 = 97;
    instruments[99].modChar2 = 146; instruments[99].carChar2 = 131;
    instruments[99].modChar3 = 177; instruments[99].carChar3 = 242;
    instruments[99].modChar4 = 129; instruments[99].carChar4 = 38;
    instruments[99].modChar5 = 0;   instruments[99].carChar5 = 0;
    instruments[99].fbConn = 60;    instruments[99].percNote = 0;
    
    // GM101: FX 5 brightness
    instruments[100].modChar1 = 65;  instruments[100].carChar1 = 66;
    instruments[100].modChar2 = 77;  instruments[100].carChar2 = 0;
    instruments[100].modChar3 = 241; instruments[100].carChar3 = 242;
    instruments[100].modChar4 = 81;  instruments[100].carChar4 = 245;
    instruments[100].modChar5 = 1;   instruments[100].carChar5 = 0;
    instruments[100].fbConn = 48;    instruments[100].percNote = 0;
    
    // GM102: FX 6 goblins
    instruments[101].modChar1 = 97;  instruments[101].carChar1 = 163;
    instruments[101].modChar2 = 148; instruments[101].carChar2 = 128;
    instruments[101].modChar3 = 17;  instruments[101].carChar3 = 17;
    instruments[101].modChar4 = 81;  instruments[101].carChar4 = 19;
    instruments[101].modChar5 = 1;   instruments[101].carChar5 = 0;
    instruments[101].fbConn = 54;    instruments[101].percNote = 0;
    
    // GM103: FX 7 echoes
    instruments[102].modChar1 = 97;  instruments[102].carChar1 = 161;
    instruments[102].modChar2 = 140; instruments[102].carChar2 = 128;
    instruments[102].modChar3 = 17;  instruments[102].carChar3 = 29;
    instruments[102].modChar4 = 49;  instruments[102].carChar4 = 3;
    instruments[102].modChar5 = 0;   instruments[102].carChar5 = 0;
    instruments[102].fbConn = 54;    instruments[102].percNote = 0;
    
    // GM104: FX 8 sci-fi
    instruments[103].modChar1 = 164; instruments[103].carChar1 = 97;
    instruments[103].modChar2 = 76;  instruments[103].carChar2 = 0;
    instruments[103].modChar3 = 243; instruments[103].carChar3 = 129;
    instruments[103].modChar4 = 115; instruments[103].carChar4 = 35;
    instruments[103].modChar5 = 1;   instruments[103].carChar5 = 0;
    instruments[103].fbConn = 52;    instruments[103].percNote = 0;
    
    // GM105: Sitar
    instruments[104].modChar1 = 2;   instruments[104].carChar1 = 7;
    instruments[104].modChar2 = 133; instruments[104].carChar2 = 3;
    instruments[104].modChar3 = 210; instruments[104].carChar3 = 242;
    instruments[104].modChar4 = 83;  instruments[104].carChar4 = 246;
    instruments[104].modChar5 = 0;   instruments[104].carChar5 = 1;
    instruments[104].fbConn = 48;    instruments[104].percNote = 0;
    
    // GM106: Banjo
    instruments[105].modChar1 = 17;  instruments[105].carChar1 = 19;
    instruments[105].modChar2 = 12;  instruments[105].carChar2 = 128;
    instruments[105].modChar3 = 163; instruments[105].carChar3 = 162;
    instruments[105].modChar4 = 17;  instruments[105].carChar4 = 229;
    instruments[105].modChar5 = 1;   instruments[105].carChar5 = 0;
    instruments[105].fbConn = 48;    instruments[105].percNote = 0;
    
    // GM107: Shamisen
    instruments[106].modChar1 = 17;  instruments[106].carChar1 = 17;
    instruments[106].modChar2 = 6;   instruments[106].carChar2 = 0;
    instruments[106].modChar3 = 246; instruments[106].carChar3 = 242;
    instruments[106].modChar4 = 65;  instruments[106].carChar4 = 230;
    instruments[106].modChar5 = 1;   instruments[106].carChar5 = 2;
    instruments[106].fbConn = 52;    instruments[106].percNote = 0;
    
    // GM108: Koto
    instruments[107].modChar1 = 147; instruments[107].carChar1 = 145;
    instruments[107].modChar2 = 145; instruments[107].carChar2 = 0;
    instruments[107].modChar3 = 212; instruments[107].carChar3 = 235;
    instruments[107].modChar4 = 50;  instruments[107].carChar4 = 17;
    instruments[107].modChar5 = 0;   instruments[107].carChar5 = 1;
    instruments[107].fbConn = 56;    instruments[107].percNote = 0;
    
    // GM109: Kalimba
    instruments[108].modChar1 = 4;   instruments[108].carChar1 = 1;
    instruments[108].modChar2 = 79;  instruments[108].carChar2 = 0;
    instruments[108].modChar3 = 250; instruments[108].carChar3 = 194;
    instruments[108].modChar4 = 86;  instruments[108].carChar4 = 5;
    instruments[108].modChar5 = 0;   instruments[108].carChar5 = 0;
    instruments[108].fbConn = 60;    instruments[108].percNote = 0;
    
    // GM110: Bagpipe
    instruments[109].modChar1 = 33;  instruments[109].carChar1 = 34;
    instruments[109].modChar2 = 73;  instruments[109].carChar2 = 0;
    instruments[109].modChar3 = 124; instruments[109].carChar3 = 111;
    instruments[109].modChar4 = 32;  instruments[109].carChar4 = 12;
    instruments[109].modChar5 = 0;   instruments[109].carChar5 = 1;
    instruments[109].fbConn = 54;    instruments[109].percNote = 0;
    
    // GM111: Fiddle
    instruments[110].modChar1 = 49;  instruments[110].carChar1 = 33;
    instruments[110].modChar2 = 133; instruments[110].carChar2 = 0;
    instruments[110].modChar3 = 221; instruments[110].carChar3 = 86;
    instruments[110].modChar4 = 51;  instruments[110].carChar4 = 22;
    instruments[110].modChar5 = 1;   instruments[110].carChar5 = 0;
    instruments[110].fbConn = 58;    instruments[110].percNote = 0;
    
    // GM112: Shanai
    instruments[111].modChar1 = 32;  instruments[111].carChar1 = 33;
    instruments[111].modChar2 = 4;   instruments[111].carChar2 = 129;
    instruments[111].modChar3 = 218; instruments[111].carChar3 = 143;
    instruments[111].modChar4 = 5;   instruments[111].carChar4 = 11;
    instruments[111].modChar5 = 2;   instruments[111].carChar5 = 0;
    instruments[111].fbConn = 54;    instruments[111].percNote = 0;
    
    // GM113: Tinkle Bell
    instruments[112].modChar1 = 5;   instruments[112].carChar1 = 3;
    instruments[112].modChar2 = 106; instruments[112].carChar2 = 128;
    instruments[112].modChar3 = 241; instruments[112].carChar3 = 195;
    instruments[112].modChar4 = 229; instruments[112].carChar4 = 229;
    instruments[112].modChar5 = 0;   instruments[112].carChar5 = 0;
    instruments[112].fbConn = 54;    instruments[112].percNote = 0;
    
    // GM114: Agogo Bells
    instruments[113].modChar1 = 7;   instruments[113].carChar1 = 2;
    instruments[113].modChar2 = 21;  instruments[113].carChar2 = 0;
    instruments[113].modChar3 = 236; instruments[113].carChar3 = 248;
    instruments[113].modChar4 = 38;  instruments[113].carChar4 = 22;
    instruments[113].modChar5 = 0;   instruments[113].carChar5 = 0;
    instruments[113].fbConn = 58;    instruments[113].percNote = 0;
    
    // GM115: Steel Drums
    instruments[114].modChar1 = 5;   instruments[114].carChar1 = 1;
    instruments[114].modChar2 = 157; instruments[114].carChar2 = 0;
    instruments[114].modChar3 = 103; instruments[114].carChar3 = 223;
    instruments[114].modChar4 = 53;  instruments[114].carChar4 = 5;
    instruments[114].modChar5 = 0;   instruments[114].carChar5 = 0;
    instruments[114].fbConn = 56;    instruments[114].percNote = 0;
    
    // GM116: Woodblock
    instruments[115].modChar1 = 24;  instruments[115].carChar1 = 18;
    instruments[115].modChar2 = 150; instruments[115].carChar2 = 0;
    instruments[115].modChar3 = 250; instruments[115].carChar3 = 248;
    instruments[115].modChar4 = 40;  instruments[115].carChar4 = 229;
    instruments[115].modChar5 = 0;   instruments[115].carChar5 = 0;
    instruments[115].fbConn = 58;    instruments[115].percNote = 0;
    
    // GM117: Taiko Drum
    instruments[116].modChar1 = 16;  instruments[116].carChar1 = 0;
    instruments[116].modChar2 = 134; instruments[116].carChar2 = 3;
    instruments[116].modChar3 = 168; instruments[116].carChar3 = 250;
    instruments[116].modChar4 = 7;   instruments[116].carChar4 = 3;
    instruments[116].modChar5 = 0;   instruments[116].carChar5 = 0;
    instruments[116].fbConn = 54;    instruments[116].percNote = 0;
    
    // GM118: Melodic Tom
    instruments[117].modChar1 = 17;  instruments[117].carChar1 = 16;
    instruments[117].modChar2 = 65;  instruments[117].carChar2 = 3;
    instruments[117].modChar3 = 248; instruments[117].carChar3 = 243;
    instruments[117].modChar4 = 71;  instruments[117].carChar4 = 3;
    instruments[117].modChar5 = 2;   instruments[117].carChar5 = 0;
    instruments[117].fbConn = 52;    instruments[117].percNote = 0;
    
    // GM119: Synth Drum
    instruments[118].modChar1 = 1;   instruments[118].carChar1 = 16;
    instruments[118].modChar2 = 142; instruments[118].carChar2 = 0;
    instruments[118].modChar3 = 241; instruments[118].carChar3 = 243;
    instruments[118].modChar4 = 6;   instruments[118].carChar4 = 2;
    instruments[118].modChar5 = 2;   instruments[118].carChar5 = 0;
    instruments[118].fbConn = 62;    instruments[118].percNote = 0;
    
    // GM120: Reverse Cymbal
    instruments[119].modChar1 = 14;  instruments[119].carChar1 = 192;
    instruments[119].modChar2 = 0;   instruments[119].carChar2 = 0;
    instruments[119].modChar3 = 31;  instruments[119].carChar3 = 31;
    instruments[119].modChar4 = 0;   instruments[119].carChar4 = 255;
    instruments[119].modChar5 = 0;   instruments[119].carChar5 = 3;
    instruments[119].fbConn = 62;    instruments[119].percNote = 0;
    
    // GM121: Guitar FretNoise
    instruments[120].modChar1 = 6;   instruments[120].carChar1 = 3;
    instruments[120].modChar2 = 128; instruments[120].carChar2 = 136;
    instruments[120].modChar3 = 248; instruments[120].carChar3 = 86;
    instruments[120].modChar4 = 36;  instruments[120].carChar4 = 132;
    instruments[120].modChar5 = 0;   instruments[120].carChar5 = 2;
    instruments[120].fbConn = 62;    instruments[120].percNote = 0;
    
    // GM122: Breath Noise
    instruments[121].modChar1 = 14;  instruments[121].carChar1 = 208;
    instruments[121].modChar2 = 0;   instruments[121].carChar2 = 5;
    instruments[121].modChar3 = 248; instruments[121].carChar3 = 52;
    instruments[121].modChar4 = 0;   instruments[121].carChar4 = 4;
    instruments[121].modChar5 = 0;   instruments[121].carChar5 = 3;
    instruments[121].fbConn = 62;    instruments[121].percNote = 0;
    
    // GM123: Seashore
    instruments[122].modChar1 = 14;  instruments[122].carChar1 = 192;
    instruments[122].modChar2 = 0;   instruments[122].carChar2 = 0;
    instruments[122].modChar3 = 246; instruments[122].carChar3 = 31;
    instruments[122].modChar4 = 0;   instruments[122].carChar4 = 2;
    instruments[122].modChar5 = 0;   instruments[122].carChar5 = 3;
    instruments[122].fbConn = 62;    instruments[122].percNote = 0;
    
    // GM124: Bird Tweet
    instruments[123].modChar1 = 213; instruments[123].carChar1 = 218;
    instruments[123].modChar2 = 149; instruments[123].carChar2 = 64;
    instruments[123].modChar3 = 55;  instruments[123].carChar3 = 86;
    instruments[123].modChar4 = 163; instruments[123].carChar4 = 55;
    instruments[123].modChar5 = 0;   instruments[123].carChar5 = 0;
    instruments[123].fbConn = 48;    instruments[123].percNote = 0;
    
    // GM125: Telephone
    instruments[124].modChar1 = 53;  instruments[124].carChar1 = 20;
    instruments[124].modChar2 = 92;  instruments[124].carChar2 = 8;
    instruments[124].modChar3 = 178; instruments[124].carChar3 = 244;
    instruments[124].modChar4 = 97;  instruments[124].carChar4 = 21;
    instruments[124].modChar5 = 2;   instruments[124].carChar5 = 0;
    instruments[124].fbConn = 58;    instruments[124].percNote = 0;
    
    // GM126: Helicopter
    instruments[125].modChar1 = 14;  instruments[125].carChar1 = 208;
    instruments[125].modChar2 = 0;   instruments[125].carChar2 = 0;
    instruments[125].modChar3 = 246; instruments[125].carChar3 = 79;
    instruments[125].modChar4 = 0;   instruments[125].carChar4 = 245;
    instruments[125].modChar5 = 0;   instruments[125].carChar5 = 3;
    instruments[125].fbConn = 62;    instruments[125].percNote = 0;
    
    // GM127: Applause/Noise
    instruments[126].modChar1 = 38;  instruments[126].carChar1 = 228;
    instruments[126].modChar2 = 0;   instruments[126].carChar2 = 0;
    instruments[126].modChar3 = 255; instruments[126].carChar3 = 18;
    instruments[126].modChar4 = 1;   instruments[126].carChar4 = 22;
    instruments[126].modChar5 = 0;   instruments[126].carChar5 = 1;
    instruments[126].fbConn = 62;    instruments[126].percNote = 0;
    
    // GM128: Gunshot
    instruments[127].modChar1 = 0;   instruments[127].carChar1 = 0;
    instruments[127].modChar2 = 0;   instruments[127].carChar2 = 0;
    instruments[127].modChar3 = 243; instruments[127].carChar3 = 246;
    instruments[127].modChar4 = 240; instruments[127].carChar4 = 201;
    instruments[127].modChar5 = 0;   instruments[127].carChar5 = 2;
    instruments[127].fbConn = 62;    instruments[127].percNote = 0;
    
    // GP35: Ac Bass Drum
    instruments[128].modChar1 = 16;  instruments[128].carChar1 = 17;
    instruments[128].modChar2 = 68;  instruments[128].carChar2 = 0;
    instruments[128].modChar3 = 248; instruments[128].carChar3 = 243;
    instruments[128].modChar4 = 119; instruments[128].carChar4 = 6;
    instruments[128].modChar5 = 2;   instruments[128].carChar5 = 0;
    instruments[128].fbConn = 56;    instruments[128].percNote = 35;
    
    // GP36: Bass Drum 1
    instruments[129].modChar1 = 16;  instruments[129].carChar1 = 17;
    instruments[129].modChar2 = 68;  instruments[129].carChar2 = 0;
    instruments[129].modChar3 = 248; instruments[129].carChar3 = 243;
    instruments[129].modChar4 = 119; instruments[129].carChar4 = 6;
    instruments[129].modChar5 = 2;   instruments[129].carChar5 = 0;
    instruments[129].fbConn = 56;    instruments[129].percNote = 35;
    
    // GP37: Side Stick
    instruments[130].modChar1 = 2;   instruments[130].carChar1 = 17;
    instruments[130].modChar2 = 7;   instruments[130].carChar2 = 0;
    instruments[130].modChar3 = 249; instruments[130].carChar3 = 248;
    instruments[130].modChar4 = 255; instruments[130].carChar4 = 255;
    instruments[130].modChar5 = 0;   instruments[130].carChar5 = 0;
    instruments[130].fbConn = 56;    instruments[130].percNote = 52;
    
    // GP38: Acoustic Snare
    instruments[131].modChar1 = 0;   instruments[131].carChar1 = 0;
    instruments[131].modChar2 = 0;   instruments[131].carChar2 = 0;
    instruments[131].modChar3 = 252; instruments[131].carChar3 = 250;
    instruments[131].modChar4 = 5;   instruments[131].carChar4 = 23;
    instruments[131].modChar5 = 2;   instruments[131].carChar5 = 0;
    instruments[131].fbConn = 62;    instruments[131].percNote = 48;
    
    // GP39: Hand Clap
    instruments[132].modChar1 = 0;   instruments[132].carChar1 = 1;
    instruments[132].modChar2 = 2;   instruments[132].carChar2 = 0;
    instruments[132].modChar3 = 255; instruments[132].carChar3 = 255;
    instruments[132].modChar4 = 7;   instruments[132].carChar4 = 8;
    instruments[132].modChar5 = 0;   instruments[132].carChar5 = 0;
    instruments[132].fbConn = 48;    instruments[132].percNote = 58;
    
    // GP40: Electric Snare
    instruments[133].modChar1 = 0;   instruments[133].carChar1 = 0;
    instruments[133].modChar2 = 0;   instruments[133].carChar2 = 0;
    instruments[133].modChar3 = 252; instruments[133].carChar3 = 250;
    instruments[133].modChar4 = 5;   instruments[133].carChar4 = 23;
    instruments[133].modChar5 = 2;   instruments[133].carChar5 = 0;
    instruments[133].fbConn = 62;    instruments[133].percNote = 60;
    
    // GP41: Low Floor Tom
    instruments[134].modChar1 = 0;   instruments[134].carChar1 = 0;
    instruments[134].modChar2 = 0;   instruments[134].carChar2 = 0;
    instruments[134].modChar3 = 246; instruments[134].carChar3 = 246;
    instruments[134].modChar4 = 12;  instruments[134].carChar4 = 6;
    instruments[134].modChar5 = 0;   instruments[134].carChar5 = 0;
    instruments[134].fbConn = 52;    instruments[134].percNote = 47;
    
    // GP42: Closed High Hat
    instruments[135].modChar1 = 12;  instruments[135].carChar1 = 18;
    instruments[135].modChar2 = 0;   instruments[135].carChar2 = 0;
    instruments[135].modChar3 = 246; instruments[135].carChar3 = 251;
    instruments[135].modChar4 = 8;   instruments[135].carChar4 = 71;
    instruments[135].modChar5 = 0;   instruments[135].carChar5 = 2;
    instruments[135].fbConn = 58;    instruments[135].percNote = 43;
    
    // GP43: High Floor Tom
    instruments[136].modChar1 = 0;   instruments[136].carChar1 = 0;
    instruments[136].modChar2 = 0;   instruments[136].carChar2 = 0;
    instruments[136].modChar3 = 246; instruments[136].carChar3 = 246;
    instruments[136].modChar4 = 12;  instruments[136].carChar4 = 6;
    instruments[136].modChar5 = 0;   instruments[136].carChar5 = 0;
    instruments[136].fbConn = 52;    instruments[136].percNote = 49;
    
    // GP44: Pedal High Hat
    instruments[137].modChar1 = 12;  instruments[137].carChar1 = 18;
    instruments[137].modChar2 = 0;   instruments[137].carChar2 = 5;
    instruments[137].modChar3 = 246; instruments[137].carChar3 = 123;
    instruments[137].modChar4 = 8;   instruments[137].carChar4 = 71;
    instruments[137].modChar5 = 0;   instruments[137].carChar5 = 2;
    instruments[137].fbConn = 58;    instruments[137].percNote = 43;
    
    // GP45: Low Tom
    instruments[138].modChar1 = 0;   instruments[138].carChar1 = 0;
    instruments[138].modChar2 = 0;   instruments[138].carChar2 = 0;
    instruments[138].modChar3 = 246; instruments[138].carChar3 = 246;
    instruments[138].modChar4 = 12;  instruments[138].carChar4 = 6;
    instruments[138].modChar5 = 0;   instruments[138].carChar5 = 0;
    instruments[138].fbConn = 52;    instruments[138].percNote = 51;
    
    // GP46: Open High Hat
    instruments[139].modChar1 = 12;  instruments[139].carChar1 = 18;
    instruments[139].modChar2 = 0;   instruments[139].carChar2 = 0;
    instruments[139].modChar3 = 246; instruments[139].carChar3 = 203;
    instruments[139].modChar4 = 2;   instruments[139].carChar4 = 67;
    instruments[139].modChar5 = 0;   instruments[139].carChar5 = 2;
    instruments[139].fbConn = 58;    instruments[139].percNote = 43;
    
    // GP47: Low-Mid Tom
    instruments[140].modChar1 = 0;   instruments[140].carChar1 = 0;
    instruments[140].modChar2 = 0;   instruments[140].carChar2 = 0;
    instruments[140].modChar3 = 246; instruments[140].carChar3 = 246;
    instruments[140].modChar4 = 12;  instruments[140].carChar4 = 6;
    instruments[140].modChar5 = 0;   instruments[140].carChar5 = 0;
    instruments[140].fbConn = 52;    instruments[140].percNote = 54;
    
    // GP48: High-Mid Tom
    instruments[141].modChar1 = 0;   instruments[141].carChar1 = 0;
    instruments[141].modChar2 = 0;   instruments[141].carChar2 = 0;
    instruments[141].modChar3 = 246; instruments[141].carChar3 = 246;
    instruments[141].modChar4 = 12;  instruments[141].carChar4 = 6;
    instruments[141].modChar5 = 0;   instruments[141].carChar5 = 0;
    instruments[141].fbConn = 52;    instruments[141].percNote = 57;
    
    // GP49: Crash Cymbal 1
    instruments[142].modChar1 = 14;  instruments[142].carChar1 = 208;
    instruments[142].modChar2 = 0;   instruments[142].carChar2 = 0;
    instruments[142].modChar3 = 246; instruments[142].carChar3 = 159;
    instruments[142].modChar4 = 0;   instruments[142].carChar4 = 2;
    instruments[142].modChar5 = 0;   instruments[142].carChar5 = 3;
    instruments[142].fbConn = 62;    instruments[142].percNote = 72;
    
    // GP50: High Tom
    instruments[143].modChar1 = 0;   instruments[143].carChar1 = 0;
    instruments[143].modChar2 = 0;   instruments[143].carChar2 = 0;
    instruments[143].modChar3 = 246; instruments[143].carChar3 = 246;
    instruments[143].modChar4 = 12;  instruments[143].carChar4 = 6;
    instruments[143].modChar5 = 0;   instruments[143].carChar5 = 0;
    instruments[143].fbConn = 52;    instruments[143].percNote = 60;
    
    // GP51: Ride Cymbal 1
    instruments[144].modChar1 = 14;  instruments[144].carChar1 = 7;
    instruments[144].modChar2 = 8;   instruments[144].carChar2 = 74;
    instruments[144].modChar3 = 248; instruments[144].carChar3 = 244;
    instruments[144].modChar4 = 66;  instruments[144].carChar4 = 228;
    instruments[144].modChar5 = 0;   instruments[144].carChar5 = 3;
    instruments[144].fbConn = 62;    instruments[144].percNote = 76;
    
    // GP52: Chinese Cymbal
    instruments[145].modChar1 = 14;  instruments[145].carChar1 = 208;
    instruments[145].modChar2 = 0;   instruments[145].carChar2 = 10;
    instruments[145].modChar3 = 245; instruments[145].carChar3 = 159;
    instruments[145].modChar4 = 48;  instruments[145].carChar4 = 2;
    instruments[145].modChar5 = 0;   instruments[145].carChar5 = 0;
    instruments[145].fbConn = 62;    instruments[145].percNote = 84;
    
    // GP53: Ride Bell
    instruments[146].modChar1 = 14;  instruments[146].carChar1 = 7;
    instruments[146].modChar2 = 10;  instruments[146].carChar2 = 93;
    instruments[146].modChar3 = 228; instruments[146].carChar3 = 245;
    instruments[146].modChar4 = 228; instruments[146].carChar4 = 229;
    instruments[146].modChar5 = 3;   instruments[146].carChar5 = 1;
    instruments[146].fbConn = 54;    instruments[146].percNote = 36;
    
    // GP54: Tambourine
    instruments[147].modChar1 = 2;   instruments[147].carChar1 = 5;
    instruments[147].modChar2 = 3;   instruments[147].carChar2 = 10;
    instruments[147].modChar3 = 180; instruments[147].carChar3 = 151;
    instruments[147].modChar4 = 4;   instruments[147].carChar4 = 247;
    instruments[147].modChar5 = 0;   instruments[147].carChar5 = 0;
    instruments[147].fbConn = 62;    instruments[147].percNote = 65;
    
    // GP55: Splash Cymbal
    instruments[148].modChar1 = 78;  instruments[148].carChar1 = 158;
    instruments[148].modChar2 = 0;   instruments[148].carChar2 = 0;
    instruments[148].modChar3 = 246; instruments[148].carChar3 = 159;
    instruments[148].modChar4 = 0;   instruments[148].carChar4 = 2;
    instruments[148].modChar5 = 0;   instruments[148].carChar5 = 3;
    instruments[148].fbConn = 62;    instruments[148].percNote = 84;
    
    // GP56: Cow Bell
    instruments[149].modChar1 = 17;  instruments[149].carChar1 = 16;
    instruments[149].modChar2 = 69;  instruments[149].carChar2 = 8;
    instruments[149].modChar3 = 248; instruments[149].carChar3 = 243;
    instruments[149].modChar4 = 55;  instruments[149].carChar4 = 5;
    instruments[149].modChar5 = 2;   instruments[149].carChar5 = 0;
    instruments[149].fbConn = 56;    instruments[149].percNote = 83;
    
    // GP57: Crash Cymbal 2
    instruments[150].modChar1 = 14;  instruments[150].carChar1 = 208;
    instruments[150].modChar2 = 0;   instruments[150].carChar2 = 0;
    instruments[150].modChar3 = 246; instruments[150].carChar3 = 159;
    instruments[150].modChar4 = 0;   instruments[150].carChar4 = 2;
    instruments[150].modChar5 = 0;   instruments[150].carChar5 = 3;
    instruments[150].fbConn = 62;    instruments[150].percNote = 84;
    
    // GP58: Vibraslap
    instruments[151].modChar1 = 128; instruments[151].carChar1 = 16;
    instruments[151].modChar2 = 0;   instruments[151].carChar2 = 13;
    instruments[151].modChar3 = 255; instruments[151].carChar3 = 255;
    instruments[151].modChar4 = 3;   instruments[151].carChar4 = 20;
    instruments[151].modChar5 = 3;   instruments[151].carChar5 = 0;
    instruments[151].fbConn = 60;    instruments[151].percNote = 24;
    
    // GP59: Ride Cymbal 2
    instruments[152].modChar1 = 14;  instruments[152].carChar1 = 7;
    instruments[152].modChar2 = 8;   instruments[152].carChar2 = 74;
    instruments[152].modChar3 = 248; instruments[152].carChar3 = 244;
    instruments[152].modChar4 = 66;  instruments[152].carChar4 = 228;
    instruments[152].modChar5 = 0;   instruments[152].carChar5 = 3;
    instruments[152].fbConn = 62;    instruments[152].percNote = 77;
    
    // GP60: High Bongo
    instruments[153].modChar1 = 6;   instruments[153].carChar1 = 2;
    instruments[153].modChar2 = 11;  instruments[153].carChar2 = 0;
    instruments[153].modChar3 = 245; instruments[153].carChar3 = 245;
    instruments[153].modChar4 = 12;  instruments[153].carChar4 = 8;
    instruments[153].modChar5 = 0;   instruments[153].carChar5 = 0;
    instruments[153].fbConn = 54;    instruments[153].percNote = 60;
    
    // GP61: Low Bongo
    instruments[154].modChar1 = 1;   instruments[154].carChar1 = 2;
    instruments[154].modChar2 = 0;   instruments[154].carChar2 = 0;
    instruments[154].modChar3 = 250; instruments[154].carChar3 = 200;
    instruments[154].modChar4 = 191; instruments[154].carChar4 = 151;
    instruments[154].modChar5 = 0;   instruments[154].carChar5 = 0;
    instruments[154].fbConn = 55;    instruments[154].percNote = 65;
    
    // GP62: Mute High Conga
    instruments[155].modChar1 = 1;   instruments[155].carChar1 = 1;
    instruments[155].modChar2 = 81;  instruments[155].carChar2 = 0;
    instruments[155].modChar3 = 250; instruments[155].carChar3 = 250;
    instruments[155].modChar4 = 135; instruments[155].carChar4 = 183;
    instruments[155].modChar5 = 0;   instruments[155].carChar5 = 0;
    instruments[155].fbConn = 54;    instruments[155].percNote = 59;

    // GP63: Open High Conga
    instruments[156].modChar1 = 1;   instruments[156].carChar1 = 2;
    instruments[156].modChar2 = 84;  instruments[156].carChar2 = 0;
    instruments[156].modChar3 = 250; instruments[156].carChar3 = 248;
    instruments[156].modChar4 = 141; instruments[156].carChar4 = 184;
    instruments[156].modChar5 = 0;   instruments[156].carChar5 = 0;
    instruments[156].fbConn = 54;    instruments[156].percNote = 51;
    
    // GP64: Low Conga
    instruments[157].modChar1 = 1;   instruments[157].carChar1 = 2;
    instruments[157].modChar2 = 89;  instruments[157].carChar2 = 0;
    instruments[157].modChar3 = 250; instruments[157].carChar3 = 248;
    instruments[157].modChar4 = 136; instruments[157].carChar4 = 182;
    instruments[157].modChar5 = 0;   instruments[157].carChar5 = 0;
    instruments[157].fbConn = 54;    instruments[157].percNote = 45;
    
    // GP65: High Timbale
    instruments[158].modChar1 = 1;   instruments[158].carChar1 = 0;
    instruments[158].modChar2 = 0;   instruments[158].carChar2 = 0;
    instruments[158].modChar3 = 249; instruments[158].carChar3 = 250;
    instruments[158].modChar4 = 10;  instruments[158].carChar4 = 6;
    instruments[158].modChar5 = 3;   instruments[158].carChar5 = 0;
    instruments[158].fbConn = 62;    instruments[158].percNote = 71;

    // GP62: Mute High Conga
    instruments[155].modChar1 = 1;   instruments[155].carChar1 = 1;
    instruments[155].modChar2 = 81;  instruments[155].carChar2 = 0;
    instruments[155].modChar3 = 250; instruments[155].carChar3 = 250;
    instruments[155].modChar4 = 135; instruments[155].carChar4 = 183;
    instruments[155].modChar5 = 0;   instruments[155].carChar5 = 0;
    instruments[155].fbConn = 54;    instruments[155].percNote = 59;
    
    // GP63: Open High Conga
    instruments[156].modChar1 = 1;   instruments[156].carChar1 = 2;
    instruments[156].modChar2 = 84;  instruments[156].carChar2 = 0;
    instruments[156].modChar3 = 250; instruments[156].carChar3 = 248;
    instruments[156].modChar4 = 141; instruments[156].carChar4 = 184;
    instruments[156].modChar5 = 0;   instruments[156].carChar5 = 0;
    instruments[156].fbConn = 54;    instruments[156].percNote = 51;
    
    // GP64: Low Conga
    instruments[157].modChar1 = 1;   instruments[157].carChar1 = 2;
    instruments[157].modChar2 = 89;  instruments[157].carChar2 = 0;
    instruments[157].modChar3 = 250; instruments[157].carChar3 = 248;
    instruments[157].modChar4 = 136; instruments[157].carChar4 = 182;
    instruments[157].modChar5 = 0;   instruments[157].carChar5 = 0;
    instruments[157].fbConn = 54;    instruments[157].percNote = 45;
    
    // GP65: High Timbale
    instruments[158].modChar1 = 1;   instruments[158].carChar1 = 0;
    instruments[158].modChar2 = 0;   instruments[158].carChar2 = 0;
    instruments[158].modChar3 = 249; instruments[158].carChar3 = 250;
    instruments[158].modChar4 = 10;  instruments[158].carChar4 = 6;
    instruments[158].modChar5 = 3;   instruments[158].carChar5 = 0;
    instruments[158].fbConn = 62;    instruments[158].percNote = 71;
    
    // GP66: Low Timbale
    instruments[159].modChar1 = 0;   instruments[159].carChar1 = 0;
    instruments[159].modChar2 = 128; instruments[159].carChar2 = 0;
    instruments[159].modChar3 = 249; instruments[159].carChar3 = 246;
    instruments[159].modChar4 = 137; instruments[159].carChar4 = 108;
    instruments[159].modChar5 = 3;   instruments[159].carChar5 = 0;
    instruments[159].fbConn = 62;    instruments[159].percNote = 60;
    
    // GP67: High Agogo
    instruments[160].modChar1 = 3;   instruments[160].carChar1 = 12;
    instruments[160].modChar2 = 128; instruments[160].carChar2 = 8;
    instruments[160].modChar3 = 248; instruments[160].carChar3 = 246;
    instruments[160].modChar4 = 136; instruments[160].carChar4 = 182;
    instruments[160].modChar5 = 3;   instruments[160].carChar5 = 0;
    instruments[160].fbConn = 63;    instruments[160].percNote = 58;
    
    // GP68: Low Agogo
    instruments[161].modChar1 = 3;   instruments[161].carChar1 = 12;
    instruments[161].modChar2 = 133; instruments[161].carChar2 = 0;
    instruments[161].modChar3 = 248; instruments[161].carChar3 = 246;
    instruments[161].modChar4 = 136; instruments[161].carChar4 = 182;
    instruments[161].modChar5 = 3;   instruments[161].carChar5 = 0;
    instruments[161].fbConn = 63;    instruments[161].percNote = 53;
    
    // GP69: Cabasa
    instruments[162].modChar1 = 14;  instruments[162].carChar1 = 0;
    instruments[162].modChar2 = 64;  instruments[162].carChar2 = 8;
    instruments[162].modChar3 = 118; instruments[162].carChar3 = 119;
    instruments[162].modChar4 = 79;  instruments[162].carChar4 = 24;
    instruments[162].modChar5 = 0;   instruments[162].carChar5 = 2;
    instruments[162].fbConn = 62;    instruments[162].percNote = 64;
    
    // GP70: Maracas
    instruments[163].modChar1 = 14;  instruments[163].carChar1 = 3;
    instruments[163].modChar2 = 64;  instruments[163].carChar2 = 0;
    instruments[163].modChar3 = 200; instruments[163].carChar3 = 155;
    instruments[163].modChar4 = 73;  instruments[163].carChar4 = 105;
    instruments[163].modChar5 = 0;   instruments[163].carChar5 = 2;
    instruments[163].fbConn = 62;    instruments[163].percNote = 71;
    
    // GP71: Short Whistle
    instruments[164].modChar1 = 215; instruments[164].carChar1 = 199;
    instruments[164].modChar2 = 220; instruments[164].carChar2 = 0;
    instruments[164].modChar3 = 173; instruments[164].carChar3 = 141;
    instruments[164].modChar4 = 5;   instruments[164].carChar4 = 5;
    instruments[164].modChar5 = 3;   instruments[164].carChar5 = 0;
    instruments[164].fbConn = 62;    instruments[164].percNote = 61;
    
    // GP72: Long Whistle
    instruments[165].modChar1 = 215; instruments[165].carChar1 = 199;
    instruments[165].modChar2 = 220; instruments[165].carChar2 = 0;
    instruments[165].modChar3 = 168; instruments[165].carChar3 = 136;
    instruments[165].modChar4 = 4;   instruments[165].carChar4 = 4;
    instruments[165].modChar5 = 3;   instruments[165].carChar5 = 0;
    instruments[165].fbConn = 62;    instruments[165].percNote = 61;
    
    // GP73: Short Guiro
    instruments[166].modChar1 = 128; instruments[166].carChar1 = 17;
    instruments[166].modChar2 = 0;   instruments[166].carChar2 = 0;
    instruments[166].modChar3 = 246; instruments[166].carChar3 = 103;
    instruments[166].modChar4 = 6;   instruments[166].carChar4 = 23;
    instruments[166].modChar5 = 3;   instruments[166].carChar5 = 3;
    instruments[166].fbConn = 62;    instruments[166].percNote = 44;
    
    // GP74: Long Guiro
    instruments[167].modChar1 = 128; instruments[167].carChar1 = 17;
    instruments[167].modChar2 = 0;   instruments[167].carChar2 = 9;
    instruments[167].modChar3 = 245; instruments[167].carChar3 = 70;
    instruments[167].modChar4 = 5;   instruments[167].carChar4 = 22;
    instruments[167].modChar5 = 2;   instruments[167].carChar5 = 3;
    instruments[167].fbConn = 62;    instruments[167].percNote = 40;
    
    // GP75: Claves
    instruments[168].modChar1 = 6;   instruments[168].carChar1 = 21;
    instruments[168].modChar2 = 63;  instruments[168].carChar2 = 0;
    instruments[168].modChar3 = 0;   instruments[168].carChar3 = 247;
    instruments[168].modChar4 = 244; instruments[168].carChar4 = 245;
    instruments[168].modChar5 = 0;   instruments[168].carChar5 = 0;
    instruments[168].fbConn = 49;    instruments[168].percNote = 69;
    
    // GP76: High Wood Block
    instruments[169].modChar1 = 6;   instruments[169].carChar1 = 18;
    instruments[169].modChar2 = 63;  instruments[169].carChar2 = 0;
    instruments[169].modChar3 = 0;   instruments[169].carChar3 = 247;
    instruments[169].modChar4 = 244; instruments[169].carChar4 = 245;
    instruments[169].modChar5 = 3;   instruments[169].carChar5 = 0;
    instruments[169].fbConn = 48;    instruments[169].percNote = 68;
    
    // GP77: Low Wood Block
    instruments[170].modChar1 = 6;   instruments[170].carChar1 = 18;
    instruments[170].modChar2 = 63;  instruments[170].carChar2 = 0;
    instruments[170].modChar3 = 0;   instruments[170].carChar3 = 247;
    instruments[170].modChar4 = 244; instruments[170].carChar4 = 245;
    instruments[170].modChar5 = 0;   instruments[170].carChar5 = 0;
    instruments[170].fbConn = 49;    instruments[170].percNote = 63;
    
    // GP78: Mute Cuica
    instruments[171].modChar1 = 1;   instruments[171].carChar1 = 2;
    instruments[171].modChar2 = 88;  instruments[171].carChar2 = 0;
    instruments[171].modChar3 = 103; instruments[171].carChar3 = 117;
    instruments[171].modChar4 = 231; instruments[171].carChar4 = 7;
    instruments[171].modChar5 = 0;   instruments[171].carChar5 = 0;
    instruments[171].fbConn = 48;    instruments[171].percNote = 74;
    
    // GP79: Open Cuica
    instruments[172].modChar1 = 65;  instruments[172].carChar1 = 66;
    instruments[172].modChar2 = 69;  instruments[172].carChar2 = 8;
    instruments[172].modChar3 = 248; instruments[172].carChar3 = 117;
    instruments[172].modChar4 = 72;  instruments[172].carChar4 = 5;
    instruments[172].modChar5 = 0;   instruments[172].carChar5 = 0;
    instruments[172].fbConn = 48;    instruments[172].percNote = 60;
    
    // GP80: Mute Triangle
    instruments[173].modChar1 = 10;  instruments[173].carChar1 = 30;
    instruments[173].modChar2 = 64;  instruments[173].carChar2 = 78;
    instruments[173].modChar3 = 224; instruments[173].carChar3 = 255;
    instruments[173].modChar4 = 240; instruments[173].carChar4 = 5;
    instruments[173].modChar5 = 3;   instruments[173].carChar5 = 0;
    instruments[173].fbConn = 56;    instruments[173].percNote = 80;
    
    // GP81: Open Triangle
    instruments[174].modChar1 = 10;  instruments[174].carChar1 = 30;
    instruments[174].modChar2 = 124; instruments[174].carChar2 = 82;
    instruments[174].modChar3 = 224; instruments[174].carChar3 = 255;
    instruments[174].modChar4 = 240; instruments[174].carChar4 = 2;
    instruments[174].modChar5 = 3;   instruments[174].carChar5 = 0;
    instruments[174].fbConn = 56;    instruments[174].percNote = 64;
    
    // GP82
    instruments[175].modChar1 = 14;  instruments[175].carChar1 = 0;
    instruments[175].modChar2 = 64;  instruments[175].carChar2 = 8;
    instruments[175].modChar3 = 122; instruments[175].carChar3 = 123;
    instruments[175].modChar4 = 74;  instruments[175].carChar4 = 27;
    instruments[175].modChar5 = 0;   instruments[175].carChar5 = 2;
    instruments[175].fbConn = 62;    instruments[175].percNote = 72;
    
    // GP83
    instruments[176].modChar1 = 14;  instruments[176].carChar1 = 7;
    instruments[176].modChar2 = 10;  instruments[176].carChar2 = 64;
    instruments[176].modChar3 = 228; instruments[176].carChar3 = 85;
    instruments[176].modChar4 = 228; instruments[176].carChar4 = 57;
    instruments[176].modChar5 = 3;   instruments[176].carChar5 = 1;
    instruments[176].fbConn = 54;    instruments[176].percNote = 73;
    
    // GP84
    instruments[177].modChar1 = 5;   instruments[177].carChar1 = 4;
    instruments[177].modChar2 = 5;   instruments[177].carChar2 = 64;
    instruments[177].modChar3 = 249; instruments[177].carChar3 = 214;
    instruments[177].modChar4 = 50;  instruments[177].carChar4 = 165;
    instruments[177].modChar5 = 3;   instruments[177].carChar5 = 0;
    instruments[177].fbConn = 62;    instruments[177].percNote = 70;
    
    // GP85
    instruments[178].modChar1 = 2;   instruments[178].carChar1 = 21;
    instruments[178].modChar2 = 63;  instruments[178].carChar2 = 0;
    instruments[178].modChar3 = 0;   instruments[178].carChar3 = 247;
    instruments[178].modChar4 = 243; instruments[178].carChar4 = 245;
    instruments[178].modChar5 = 3;   instruments[178].carChar5 = 0;
    instruments[178].fbConn = 56;    instruments[178].percNote = 68;
    
    // GP86
    instruments[179].modChar1 = 1;   instruments[179].carChar1 = 2;
    instruments[179].modChar2 = 79;  instruments[179].carChar2 = 0;
    instruments[179].modChar3 = 250; instruments[179].carChar3 = 248;
    instruments[179].modChar4 = 141; instruments[179].carChar4 = 181;
    instruments[179].modChar5 = 0;   instruments[179].carChar5 = 0;
    instruments[179].fbConn = 55;    instruments[179].percNote = 48;
    
    // GP87
    instruments[180].modChar1 = 0;   instruments[180].carChar1 = 0;
    instruments[180].modChar2 = 0;   instruments[180].carChar2 = 0;
    instruments[180].modChar3 = 246; instruments[180].carChar3 = 246;
    instruments[180].modChar4 = 12;  instruments[180].carChar4 = 6;
    instruments[180].modChar5 = 0;   instruments[180].carChar5 = 0;
    instruments[180].fbConn = 52;    instruments[180].percNote = 53;
}
