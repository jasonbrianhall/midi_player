// Initialize FM instrument data
void initFMInstruments() {
    // This function initializes the FM instrument data array
    // with 180 FM instrument definitions from the original QBasic code
    
    // GM1: Acoustic Grand Piano
    adl[0].modChar1 = 1;   adl[0].carChar1 = 1;
    adl[0].modChar2 = 143; adl[0].carChar2 = 6;
    adl[0].modChar3 = 242; adl[0].carChar3 = 242;
    adl[0].modChar4 = 244; adl[0].carChar4 = 247;
    adl[0].modChar5 = 0;   adl[0].carChar5 = 0;
    adl[0].fbConn = 56;    adl[0].percNote = 0;
    
    // GM2: Bright Acoustic Grand
    adl[1].modChar1 = 1;   adl[1].carChar1 = 1;
    adl[1].modChar2 = 75;  adl[1].carChar2 = 0;
    adl[1].modChar3 = 242; adl[1].carChar3 = 242;
    adl[1].modChar4 = 244; adl[1].carChar4 = 247;
    adl[1].modChar5 = 0;   adl[1].carChar5 = 0;
    adl[1].fbConn = 56;    adl[1].percNote = 0;
    
    // GM3: Electric Grand Piano
    adl[2].modChar1 = 1;   adl[2].carChar1 = 1;
    adl[2].modChar2 = 73;  adl[2].carChar2 = 0;
    adl[2].modChar3 = 242; adl[2].carChar3 = 242;
    adl[2].modChar4 = 244; adl[2].carChar4 = 246;
    adl[2].modChar5 = 0;   adl[2].carChar5 = 0;
    adl[2].fbConn = 56;    adl[2].percNote = 0;
    
    // GM4: Honky-tonk Piano
    adl[3].modChar1 = 129; adl[3].carChar1 = 65;
    adl[3].modChar2 = 18;  adl[3].carChar2 = 0;
    adl[3].modChar3 = 242; adl[3].carChar3 = 242;
    adl[3].modChar4 = 247; adl[3].carChar4 = 247;
    adl[3].modChar5 = 0;   adl[3].carChar5 = 0;
    adl[3].fbConn = 54;    adl[3].percNote = 0;
    
    // GM5: Rhodes Piano
    adl[4].modChar1 = 1;   adl[4].carChar1 = 1;
    adl[4].modChar2 = 87;  adl[4].carChar2 = 0;
    adl[4].modChar3 = 241; adl[4].carChar3 = 242;
    adl[4].modChar4 = 247; adl[4].carChar4 = 247;
    adl[4].modChar5 = 0;   adl[4].carChar5 = 0;
    adl[4].fbConn = 48;    adl[4].percNote = 0;
    
    // GM6: Chorused Piano
    adl[5].modChar1 = 1;   adl[5].carChar1 = 1;
    adl[5].modChar2 = 147; adl[5].carChar2 = 0;
    adl[5].modChar3 = 241; adl[5].carChar3 = 242;
    adl[5].modChar4 = 247; adl[5].carChar4 = 247;
    adl[5].modChar5 = 0;   adl[5].carChar5 = 0;
    adl[5].fbConn = 48;    adl[5].percNote = 0;
    
    // GM7: Harpsichord
    adl[6].modChar1 = 1;   adl[6].carChar1 = 22;
    adl[6].modChar2 = 128; adl[6].carChar2 = 14;
    adl[6].modChar3 = 161; adl[6].carChar3 = 242;
    adl[6].modChar4 = 242; adl[6].carChar4 = 245;
    adl[6].modChar5 = 0;   adl[6].carChar5 = 0;
    adl[6].fbConn = 56;    adl[6].percNote = 0;
    
    // GM8: Clavinet
    adl[7].modChar1 = 1;   adl[7].carChar1 = 1;
    adl[7].modChar2 = 146; adl[7].carChar2 = 0;
    adl[7].modChar3 = 194; adl[7].carChar3 = 194;
    adl[7].modChar4 = 248; adl[7].carChar4 = 248;
    adl[7].modChar5 = 0;   adl[7].carChar5 = 0;
    adl[7].fbConn = 58;    adl[7].percNote = 0;
    
    // GM9: Celesta
    adl[8].modChar1 = 12;  adl[8].carChar1 = 129;
    adl[8].modChar2 = 92;  adl[8].carChar2 = 0;
    adl[8].modChar3 = 246; adl[8].carChar3 = 243;
    adl[8].modChar4 = 244; adl[8].carChar4 = 245;
    adl[8].modChar5 = 0;   adl[8].carChar5 = 0;
    adl[8].fbConn = 48;    adl[8].percNote = 0;
    
    // GM10: Glockenspiel
    adl[9].modChar1 = 7;   adl[9].carChar1 = 17;
    adl[9].modChar2 = 151; adl[9].carChar2 = 128;
    adl[9].modChar3 = 243; adl[9].carChar3 = 242;
    adl[9].modChar4 = 242; adl[9].carChar4 = 241;
    adl[9].modChar5 = 0;   adl[9].carChar5 = 0;
    adl[9].fbConn = 50;    adl[9].percNote = 0;
    
    // GM11: Music box
    adl[10].modChar1 = 23;  adl[10].carChar1 = 1;
    adl[10].modChar2 = 33;  adl[10].carChar2 = 0;
    adl[10].modChar3 = 84;  adl[10].carChar3 = 244;
    adl[10].modChar4 = 244; adl[10].carChar4 = 244;
    adl[10].modChar5 = 0;   adl[10].carChar5 = 0;
    adl[10].fbConn = 50;    adl[10].percNote = 0;
    
    // GM12: Vibraphone
    adl[11].modChar1 = 152; adl[11].carChar1 = 129;
    adl[11].modChar2 = 98;  adl[11].carChar2 = 0;
    adl[11].modChar3 = 243; adl[11].carChar3 = 242;
    adl[11].modChar4 = 246; adl[11].carChar4 = 246;
    adl[11].modChar5 = 0;   adl[11].carChar5 = 0;
    adl[11].fbConn = 48;    adl[11].percNote = 0;
    
    // GM13: Marimba
    adl[12].modChar1 = 24;  adl[12].carChar1 = 1;
    adl[12].modChar2 = 35;  adl[12].carChar2 = 0;
    adl[12].modChar3 = 246; adl[12].carChar3 = 231;
    adl[12].modChar4 = 246; adl[12].carChar4 = 247;
    adl[12].modChar5 = 0;   adl[12].carChar5 = 0;
    adl[12].fbConn = 48;    adl[12].percNote = 0;
    
    // GM14: Xylophone
    adl[13].modChar1 = 21;  adl[13].carChar1 = 1;
    adl[13].modChar2 = 145; adl[13].carChar2 = 0;
    adl[13].modChar3 = 246; adl[13].carChar3 = 246;
    adl[13].modChar4 = 246; adl[13].carChar4 = 246;
    adl[13].modChar5 = 0;   adl[13].carChar5 = 0;
    adl[13].fbConn = 52;    adl[13].percNote = 0;
    
    // GM15: Tubular Bells
    adl[14].modChar1 = 69;  adl[14].carChar1 = 129;
    adl[14].modChar2 = 89;  adl[14].carChar2 = 128;
    adl[14].modChar3 = 211; adl[14].carChar3 = 163;
    adl[14].modChar4 = 243; adl[14].carChar4 = 243;
    adl[14].modChar5 = 0;   adl[14].carChar5 = 0;
    adl[14].fbConn = 60;    adl[14].percNote = 0;
    
    // GM16: Dulcimer
    adl[15].modChar1 = 3;   adl[15].carChar1 = 129;
    adl[15].modChar2 = 73;  adl[15].carChar2 = 128;
    adl[15].modChar3 = 117; adl[15].carChar3 = 181;
    adl[15].modChar4 = 245; adl[15].carChar4 = 245;
    adl[15].modChar5 = 1;   adl[15].carChar5 = 0;
    adl[15].fbConn = 52;    adl[15].percNote = 0;
    
    // GM17: Hammond Organ
    adl[16].modChar1 = 113; adl[16].carChar1 = 49;
    adl[16].modChar2 = 146; adl[16].carChar2 = 0;
    adl[16].modChar3 = 246; adl[16].carChar3 = 241;
    adl[16].modChar4 = 20;  adl[16].carChar4 = 7;
    adl[16].modChar5 = 0;   adl[16].carChar5 = 0;
    adl[16].fbConn = 50;    adl[16].percNote = 0;
    
    // GM18: Percussive Organ
    adl[17].modChar1 = 114; adl[17].carChar1 = 48;
    adl[17].modChar2 = 20;  adl[17].carChar2 = 0;
    adl[17].modChar3 = 199; adl[17].carChar3 = 199;
    adl[17].modChar4 = 88;  adl[17].carChar4 = 8;
    adl[17].modChar5 = 0;   adl[17].carChar5 = 0;
    adl[17].fbConn = 50;    adl[17].percNote = 0;
    
    // GM19: Rock Organ
    adl[18].modChar1 = 112; adl[18].carChar1 = 177;
    adl[18].modChar2 = 68;  adl[18].carChar2 = 0;
    adl[18].modChar3 = 170; adl[18].carChar3 = 138;
    adl[18].modChar4 = 24;  adl[18].carChar4 = 8;
    adl[18].modChar5 = 0;   adl[18].carChar5 = 0;
    adl[18].fbConn = 52;    adl[18].percNote = 0;
    
    // GM20: Church Organ
    adl[19].modChar1 = 35;  adl[19].carChar1 = 177;
    adl[19].modChar2 = 147; adl[19].carChar2 = 0;
    adl[19].modChar3 = 151; adl[19].carChar3 = 85;
    adl[19].modChar4 = 35;  adl[19].carChar4 = 20;
    adl[19].modChar5 = 1;   adl[19].carChar5 = 0;
    adl[19].fbConn = 52;    adl[19].percNote = 0;
    
    // GM21: Reed Organ
    adl[20].modChar1 = 97;  adl[20].carChar1 = 177;
    adl[20].modChar2 = 19;  adl[20].carChar2 = 128;
    adl[20].modChar3 = 151; adl[20].carChar3 = 85;
    adl[20].modChar4 = 4;   adl[20].carChar4 = 4;
    adl[20].modChar5 = 1;   adl[20].carChar5 = 0;
    adl[20].fbConn = 48;    adl[20].percNote = 0;
    
    // GM22: Accordion
    adl[21].modChar1 = 36;  adl[21].carChar1 = 177;
    adl[21].modChar2 = 72;  adl[21].carChar2 = 0;
    adl[21].modChar3 = 152; adl[21].carChar3 = 70;
    adl[21].modChar4 = 42;  adl[21].carChar4 = 26;
    adl[21].modChar5 = 1;   adl[21].carChar5 = 0;
    adl[21].fbConn = 60;    adl[21].percNote = 0;
    
    // GM23: Harmonica
    adl[22].modChar1 = 97;  adl[22].carChar1 = 33;
    adl[22].modChar2 = 19;  adl[22].carChar2 = 0;
    adl[22].modChar3 = 145; adl[22].carChar3 = 97;
    adl[22].modChar4 = 6;   adl[22].carChar4 = 7;
    adl[22].modChar5 = 1;   adl[22].carChar5 = 0;
    adl[22].fbConn = 58;    adl[22].percNote = 0;
    
    // GM24: Tango Accordion
    adl[23].modChar1 = 33;  adl[23].carChar1 = 161;
    adl[23].modChar2 = 19;  adl[23].carChar2 = 137;
    adl[23].modChar3 = 113; adl[23].carChar3 = 97;
    adl[23].modChar4 = 6;   adl[23].carChar4 = 7;
    adl[23].modChar5 = 0;   adl[23].carChar5 = 0;
    adl[23].fbConn = 54;    adl[23].percNote = 0;
    
    // GM25: Acoustic Guitar1
    adl[24].modChar1 = 2;   adl[24].carChar1 = 65;
    adl[24].modChar2 = 156; adl[24].carChar2 = 128;
    adl[24].modChar3 = 243; adl[24].carChar3 = 243;
    adl[24].modChar4 = 148; adl[24].carChar4 = 200;
    adl[24].modChar5 = 1;   adl[24].carChar5 = 0;
    adl[24].fbConn = 60;    adl[24].percNote = 0;
    
    // GM26: Acoustic Guitar2
    adl[25].modChar1 = 3;   adl[25].carChar1 = 17;
    adl[25].modChar2 = 84;  adl[25].carChar2 = 0;
    adl[25].modChar3 = 243; adl[25].carChar3 = 241;
    adl[25].modChar4 = 154; adl[25].carChar4 = 231;
    adl[25].modChar5 = 1;   adl[25].carChar5 = 0;
    adl[25].fbConn = 60;    adl[25].percNote = 0;
    
    // GM27: Electric Guitar1
    adl[26].modChar1 = 35;  adl[26].carChar1 = 33;
    adl[26].modChar2 = 95;  adl[26].carChar2 = 0;
    adl[26].modChar3 = 241; adl[26].carChar3 = 242;
    adl[26].modChar4 = 58;  adl[26].carChar4 = 248;
    adl[26].modChar5 = 0;   adl[26].carChar5 = 0;
    adl[26].fbConn = 48;    adl[26].percNote = 0;
    
    // GM28: Electric Guitar2
    adl[27].modChar1 = 3;   adl[27].carChar1 = 33;
    adl[27].modChar2 = 135; adl[27].carChar2 = 128;
    adl[27].modChar3 = 246; adl[27].carChar3 = 243;
    adl[27].modChar4 = 34;  adl[27].carChar4 = 248;
    adl[27].modChar5 = 1;   adl[27].carChar5 = 0;
    adl[27].fbConn = 54;    adl[27].percNote = 0;
    
    // GM29: Electric Guitar3
    adl[28].modChar1 = 3;   adl[28].carChar1 = 33;
    adl[28].modChar2 = 71;  adl[28].carChar2 = 0;
    adl[28].modChar3 = 249; adl[28].carChar3 = 246;
    adl[28].modChar4 = 84;  adl[28].carChar4 = 58;
    adl[28].modChar5 = 0;   adl[28].carChar5 = 0;
    adl[28].fbConn = 48;    adl[28].percNote = 0;
    
    // GM30: Overdrive Guitar
    adl[29].modChar1 = 35;  adl[29].carChar1 = 33;
    adl[29].modChar2 = 74;  adl[29].carChar2 = 5;
    adl[29].modChar3 = 145; adl[29].carChar3 = 132;
    adl[29].modChar4 = 65;  adl[29].carChar4 = 25;
    adl[29].modChar5 = 1;   adl[29].carChar5 = 0;
    adl[29].fbConn = 56;    adl[29].percNote = 0;
    
    // GM31: Distortion Guitar
    adl[30].modChar1 = 35;  adl[30].carChar1 = 33;
    adl[30].modChar2 = 74;  adl[30].carChar2 = 0;
    adl[30].modChar3 = 149; adl[30].carChar3 = 148;
    adl[30].modChar4 = 25;  adl[30].carChar4 = 25;
    adl[30].modChar5 = 1;   adl[30].carChar5 = 0;
    adl[30].fbConn = 56;    adl[30].percNote = 0;
    
    // GM32: Guitar Harmonics
    adl[31].modChar1 = 9;   adl[31].carChar1 = 132;
    adl[31].modChar2 = 161; adl[31].carChar2 = 128;
    adl[31].modChar3 = 32;  adl[31].carChar3 = 209;
    adl[31].modChar4 = 79;  adl[31].carChar4 = 248;
    adl[31].modChar5 = 0;   adl[31].carChar5 = 0;
    adl[31].fbConn = 56;    adl[31].percNote = 0;
    
    // GM33: Acoustic Bass
    adl[32].modChar1 = 33;  adl[32].carChar1 = 162;
    adl[32].modChar2 = 30;  adl[32].carChar2 = 0;
    adl[32].modChar3 = 148; adl[32].carChar3 = 195;
    adl[32].modChar4 = 6;   adl[32].carChar4 = 166;
    adl[32].modChar5 = 0;   adl[32].carChar5 = 0;
    adl[32].fbConn = 50;    adl[32].percNote = 0;
    
    // GM34: Electric Bass 1
    adl[33].modChar1 = 49;  adl[33].carChar1 = 49;
    adl[33].modChar2 = 18;  adl[33].carChar2 = 0;
    adl[33].modChar3 = 241; adl[33].carChar3 = 241;
    adl[33].modChar4 = 40;  adl[33].carChar4 = 24;
    adl[33].modChar5 = 0;   adl[33].carChar5 = 0;
    adl[33].fbConn = 58;    adl[33].percNote = 0;
    
    // GM35: Electric Bass 2
    adl[34].modChar1 = 49;  adl[34].carChar1 = 49;
    adl[34].modChar2 = 141; adl[34].carChar2 = 0;
    adl[34].modChar3 = 241; adl[34].carChar3 = 241;
    adl[34].modChar4 = 232; adl[34].carChar4 = 120;
    adl[34].modChar5 = 0;   adl[34].carChar5 = 0;
    adl[34].fbConn = 58;    adl[34].percNote = 0;
    
    // GM36: Fretless Bass
    adl[35].modChar1 = 49;  adl[35].carChar1 = 50;
    adl[35].modChar2 = 91;  adl[35].carChar2 = 0;
    adl[35].modChar3 = 81;  adl[35].carChar3 = 113;
    adl[35].modChar4 = 40;  adl[35].carChar4 = 72;
    adl[35].modChar5 = 0;   adl[35].carChar5 = 0;
    adl[35].fbConn = 60;    adl[35].percNote = 0;
    
    // GM37: Slap Bass 1
    adl[36].modChar1 = 1;   adl[36].carChar1 = 33;
    adl[36].modChar2 = 139; adl[36].carChar2 = 64;
    adl[36].modChar3 = 161; adl[36].carChar3 = 242;
    adl[36].modChar4 = 154; adl[36].carChar4 = 223;
    adl[36].modChar5 = 0;   adl[36].carChar5 = 0;
    adl[36].fbConn = 56;    adl[36].percNote = 0;
    
    // GM38: Slap Bass 2
    adl[37].modChar1 = 33;  adl[37].carChar1 = 33;
    adl[37].modChar2 = 139; adl[37].carChar2 = 8;
    adl[37].modChar3 = 162; adl[37].carChar3 = 161;
    adl[37].modChar4 = 22;  adl[37].carChar4 = 223;
    adl[37].modChar5 = 0;   adl[37].carChar5 = 0;
    adl[37].fbConn = 56;    adl[37].percNote = 0;
    
    // GM39: Synth Bass 1
    adl[38].modChar1 = 49;  adl[38].carChar1 = 49;
    adl[38].modChar2 = 139; adl[38].carChar2 = 0;
    adl[38].modChar3 = 244; adl[38].carChar3 = 241;
    adl[38].modChar4 = 232; adl[38].carChar4 = 120;
    adl[38].modChar5 = 0;   adl[38].carChar5 = 0;
    adl[38].fbConn = 58;    adl[38].percNote = 0;
    
    // GM40: Synth Bass 2
    adl[39].modChar1 = 49;  adl[39].carChar1 = 49;
    adl[39].modChar2 = 18;  adl[39].carChar2 = 0;
    adl[39].modChar3 = 241; adl[39].carChar3 = 241;
    adl[39].modChar4 = 40;  adl[39].carChar4 = 24;
    adl[39].modChar5 = 0;   adl[39].carChar5 = 0;
    adl[39].fbConn = 58;    adl[39].percNote = 0;
    
    // GM41: Violin
    adl[40].modChar1 = 49;  adl[40].carChar1 = 33;
    adl[40].modChar2 = 21;  adl[40].carChar2 = 0;
    adl[40].modChar3 = 221; adl[40].carChar3 = 86;
    adl[40].modChar4 = 19;  adl[40].carChar4 = 38;
    adl[40].modChar5 = 1;   adl[40].carChar5 = 0;
    adl[40].fbConn = 56;    adl[40].percNote = 0;
    
    // GM42: Viola
    adl[41].modChar1 = 49;  adl[41].carChar1 = 33;
    adl[41].modChar2 = 22;  adl[41].carChar2 = 0;
    adl[41].modChar3 = 221; adl[41].carChar3 = 102;
    adl[41].modChar4 = 19;  adl[41].carChar4 = 6;
    adl[41].modChar5 = 1;   adl[41].carChar5 = 0;
    adl[41].fbConn = 56;    adl[41].percNote = 0;
    
    // GM43: Cello
    adl[42].modChar1 = 113; adl[42].carChar1 = 49;
    adl[42].modChar2 = 73;  adl[42].carChar2 = 0;
    adl[42].modChar3 = 209; adl[42].carChar3 = 97;
    adl[42].modChar4 = 28;  adl[42].carChar4 = 12;
    adl[42].modChar5 = 1;   adl[42].carChar5 = 0;
    adl[42].fbConn = 56;    adl[42].percNote = 0;
    
    // GM44: Contrabass
    adl[43].modChar1 = 33;  adl[43].carChar1 = 35;
    adl[43].modChar2 = 77;  adl[43].carChar2 = 128;
    adl[43].modChar3 = 113; adl[43].carChar3 = 114;
    adl[43].modChar4 = 18;  adl[43].carChar4 = 6;
    adl[43].modChar5 = 1;   adl[43].carChar5 = 0;
    adl[43].fbConn = 50;    adl[43].percNote = 0;
    
    // GM45: Tremulo Strings
    adl[44].modChar1 = 241; adl[44].carChar1 = 225;
    adl[44].modChar2 = 64;  adl[44].carChar2 = 0;
    adl[44].modChar3 = 241; adl[44].carChar3 = 111;
    adl[44].modChar4 = 33;  adl[44].carChar4 = 22;
    adl[44].modChar5 = 1;   adl[44].carChar5 = 0;
    adl[44].fbConn = 50;    adl[44].percNote = 0;
    
    // GM46: Pizzicato String
    adl[45].modChar1 = 2;   adl[45].carChar1 = 1;
    adl[45].modChar2 = 26;  adl[45].carChar2 = 128;
    adl[45].modChar3 = 245; adl[45].carChar3 = 133;
    adl[45].modChar4 = 117; adl[45].carChar4 = 53;
    adl[45].modChar5 = 1;   adl[45].carChar5 = 0;
    adl[45].fbConn = 48;    adl[45].percNote = 0;
    
    // GM47: Orchestral Harp
    adl[46].modChar1 = 2;   adl[46].carChar1 = 1;
    adl[46].modChar2 = 29;  adl[46].carChar2 = 128;
    adl[46].modChar3 = 245; adl[46].carChar3 = 243;
    adl[46].modChar4 = 117; adl[46].carChar4 = 244;
    adl[46].modChar5 = 1;   adl[46].carChar5 = 0;
    adl[46].fbConn = 48;    adl[46].percNote = 0;
    
    // GM48: Timpany
    adl[47].modChar1 = 16;  adl[47].carChar1 = 17;
    adl[47].modChar2 = 65;  adl[47].carChar2 = 0;
    adl[47].modChar3 = 245; adl[47].carChar3 = 242;
    adl[47].modChar4 = 5;   adl[47].carChar4 = 195;
    adl[47].modChar5 = 1;   adl[47].carChar5 = 0;
    adl[47].fbConn = 50;    adl[47].percNote = 0;
    
    // GM49: String Ensemble1
    adl[48].modChar1 = 33;  adl[48].carChar1 = 162;
    adl[48].modChar2 = 155; adl[48].carChar2 = 1;
    adl[48].modChar3 = 177; adl[48].carChar3 = 114;
    adl[48].modChar4 = 37;  adl[48].carChar4 = 8;
    adl[48].modChar5 = 1;   adl[48].carChar5 = 0;
    adl[48].fbConn = 62;    adl[48].percNote = 0;
    
    // GM50: String Ensemble2
    adl[49].modChar1 = 161; adl[49].carChar1 = 33;
    adl[49].modChar2 = 152; adl[49].carChar2 = 0;
    adl[49].modChar3 = 127; adl[49].carChar3 = 63;
    adl[49].modChar4 = 3;   adl[49].carChar4 = 7;
    adl[49].modChar5 = 1;   adl[49].carChar5 = 1;
    adl[49].fbConn = 48;    adl[49].percNote = 0;
    
    // GM51: Synth Strings 1
    adl[50].modChar1 = 161; adl[50].carChar1 = 97;
    adl[50].modChar2 = 147; adl[50].carChar2 = 0;
    adl[50].modChar3 = 193; adl[50].carChar3 = 79;
    adl[50].modChar4 = 18;  adl[50].carChar4 = 5;
    adl[50].modChar5 = 0;   adl[50].carChar5 = 0;
    adl[50].fbConn = 58;    adl[50].percNote = 0;
    
    // GM52: SynthStrings 2
    adl[51].modChar1 = 33;  adl[51].carChar1 = 97;
    adl[51].modChar2 = 24;  adl[51].carChar2 = 0;
    adl[51].modChar3 = 193; adl[51].carChar3 = 79;
    adl[51].modChar4 = 34;  adl[51].carChar4 = 5;
    adl[51].modChar5 = 0;   adl[51].carChar5 = 0;
    adl[51].fbConn = 60;    adl[51].percNote = 0;
    
    // GM53: Choir Aahs
    adl[52].modChar1 = 49;  adl[52].carChar1 = 114;
    adl[52].modChar2 = 91;  adl[52].carChar2 = 131;
    adl[52].modChar3 = 244; adl[52].carChar3 = 138;
    adl[52].modChar4 = 21;  adl[52].carChar4 = 5;
    adl[52].modChar5 = 0;   adl[52].carChar5 = 0;
    adl[52].fbConn = 48;    adl[52].percNote = 0;
    
    // GM54: Voice Oohs
    adl[53].modChar1 = 161; adl[53].carChar1 = 97;
    adl[53].modChar2 = 144; adl[53].carChar2 = 0;
    adl[53].modChar3 = 116; adl[53].carChar3 = 113;
    adl[53].modChar4 = 57;  adl[53].carChar4 = 103;
    adl[53].modChar5 = 0;   adl[53].carChar5 = 0;
    adl[53].fbConn = 48;    adl[53].percNote = 0;
    
    // GM55: Synth Voice
    adl[54].modChar1 = 113; adl[54].carChar1 = 114;
    adl[54].modChar2 = 87;  adl[54].carChar2 = 0;
    adl[54].modChar3 = 84;  adl[54].carChar3 = 122;
    adl[54].modChar4 = 5;   adl[54].carChar4 = 5;
    adl[54].modChar5 = 0;   adl[54].carChar5 = 0;
    adl[54].fbConn = 60;    adl[54].percNote = 0;
    
    // GM56: Orchestra Hit
    adl[55].modChar1 = 144; adl[55].carChar1 = 65;
    adl[55].modChar2 = 0;   adl[55].carChar2 = 0;
    adl[55].modChar3 = 84;  adl[55].carChar3 = 165;
    adl[55].modChar4 = 99;  adl[55].carChar4 = 69;
    adl[55].modChar5 = 0;   adl[55].carChar5 = 0;
    adl[55].fbConn = 56;    adl[55].percNote = 0;
    
    // GM57: Trumpet
    adl[56].modChar1 = 33;  adl[56].carChar1 = 33;
    adl[56].modChar2 = 146; adl[56].carChar2 = 1;
    adl[56].modChar3 = 133; adl[56].carChar3 = 143;
    adl[56].modChar4 = 23;  adl[56].carChar4 = 9;
    adl[56].modChar5 = 0;   adl[56].carChar5 = 0;
    adl[56].fbConn = 60;    adl[56].percNote = 0;
    
    // GM58: Trombone
    adl[57].modChar1 = 33;  adl[57].carChar1 = 33;
    adl[57].modChar2 = 148; adl[57].carChar2 = 5;
    adl[57].modChar3 = 117; adl[57].carChar3 = 143;
    adl[57].modChar4 = 23;  adl[57].carChar4 = 9;
    adl[57].modChar5 = 0;   adl[57].carChar5 = 0;
    adl[57].fbConn = 60;    adl[57].percNote = 0;
    
    // GM59: Tuba
    adl[58].modChar1 = 33;  adl[58].carChar1 = 97;
    adl[58].modChar2 = 148; adl[58].carChar2 = 0;
    adl[58].modChar3 = 118; adl[58].carChar3 = 130;
    adl[58].modChar4 = 21;  adl[58].carChar4 = 55;
    adl[58].modChar5 = 0;   adl[58].carChar5 = 0;
    adl[58].fbConn = 60;    adl[58].percNote = 0;
    
    // GM60: Muted Trumpet
    adl[59].modChar1 = 49;  adl[59].carChar1 = 33;
    adl[59].modChar2 = 67;  adl[59].carChar2 = 0;
    adl[59].modChar3 = 158; adl[59].carChar3 = 98;
    adl[59].modChar4 = 23;  adl[59].carChar4 = 44;
    adl[59].modChar5 = 1;   adl[59].carChar5 = 1;
    adl[59].fbConn = 50;    adl[59].percNote = 0;
    
    // GM61: French Horn
    adl[60].modChar1 = 33;  adl[60].carChar1 = 33;
    adl[60].modChar2 = 155; adl[60].carChar2 = 0;
    adl[60].modChar3 = 97;  adl[60].carChar3 = 127;
    adl[60].modChar4 = 106; adl[60].carChar4 = 10;
    adl[60].modChar5 = 0;   adl[60].carChar5 = 0;
    adl[60].fbConn = 50;    adl[60].percNote = 0;
    
    // GM62: Brass Section
    adl[61].modChar1 = 97;  adl[61].carChar1 = 34;
    adl[61].modChar2 = 138; adl[61].carChar2 = 6;
    adl[61].modChar3 = 117; adl[61].carChar3 = 116;
    adl[61].modChar4 = 31;  adl[61].carChar4 = 15;
    adl[61].modChar5 = 0;   adl[61].carChar5 = 0;
    adl[61].fbConn = 56;    adl[61].percNote = 0;
    
    // GM63: Synth Brass 1
    adl[62].modChar1 = 161; adl[62].carChar1 = 33;
    adl[62].modChar2 = 134; adl[62].carChar2 = 131;
    adl[62].modChar3 = 114; adl[62].carChar3 = 113;
    adl[62].modChar4 = 85;  adl[62].carChar4 = 24;
    adl[62].modChar5 = 1;   adl[62].carChar5 = 0;
    adl[62].fbConn = 48;    adl[62].percNote = 0;
    
    // GM64: Synth Brass 2
    adl[63].modChar1 = 33;  adl[63].carChar1 = 33;
    adl[63].modChar2 = 77;  adl[63].carChar2 = 0;
    adl[63].modChar3 = 84;  adl[63].carChar3 = 166;
    adl[63].modChar4 = 60;  adl[63].carChar4 = 28;
    adl[63].modChar5 = 0;   adl[63].carChar5 = 0;
    adl[63].fbConn = 56;    adl[63].percNote = 0;
    
    // GM65: Soprano Sax
    adl[64].modChar1 = 49;  adl[64].carChar1 = 97;
    adl[64].modChar2 = 143; adl[64].carChar2 = 0;
    adl[64].modChar3 = 147; adl[64].carChar3 = 114;
    adl[64].modChar4 = 2;   adl[64].carChar4 = 11;
    adl[64].modChar5 = 1;   adl[64].carChar5 = 0;
    adl[64].fbConn = 56;    adl[64].percNote = 0;
    
    // GM66: Alto Sax
    adl[65].modChar1 = 49;  adl[65].carChar1 = 97;
    adl[65].modChar2 = 142; adl[65].carChar2 = 0;
    adl[65].modChar3 = 147; adl[65].carChar3 = 114;
    adl[65].modChar4 = 3;   adl[65].carChar4 = 9;
    adl[65].modChar5 = 1;   adl[65].carChar5 = 0;
    adl[65].fbConn = 56;    adl[65].percNote = 0;
    
    // GM67: Tenor Sax
    adl[66].modChar1 = 49;  adl[66].carChar1 = 97;
    adl[66].modChar2 = 145; adl[66].carChar2 = 0;
    adl[66].modChar3 = 147; adl[66].carChar3 = 130;
    adl[66].modChar4 = 3;   adl[66].carChar4 = 9;
    adl[66].modChar5 = 1;   adl[66].carChar5 = 0;
    adl[66].fbConn = 58;    adl[66].percNote = 0;
    
    // GM68: Baritone Sax
    adl[67].modChar1 = 49;  adl[67].carChar1 = 97;
    adl[67].modChar2 = 142; adl[67].carChar2 = 0;
    adl[67].modChar3 = 147; adl[67].carChar3 = 114;
    adl[67].modChar4 = 15;  adl[67].carChar4 = 15;
    adl[67].modChar5 = 1;   adl[67].carChar5 = 0;
    adl[67].fbConn = 58;    adl[67].percNote = 0;
    
    // GM69: Oboe
    adl[68].modChar1 = 33;  adl[68].carChar1 = 33;
    adl[68].modChar2 = 75;  adl[68].carChar2 = 0;
    adl[68].modChar3 = 170; adl[68].carChar3 = 143;
    adl[68].modChar4 = 22;  adl[68].carChar4 = 10;
    adl[68].modChar5 = 1;   adl[68].carChar5 = 0;
    adl[68].fbConn = 56;    adl[68].percNote = 0;
    
    // GM70: English Horn
    adl[69].modChar1 = 49;  adl[69].carChar1 = 33;
    adl[69].modChar2 = 144; adl[69].carChar2 = 0;
    adl[69].modChar3 = 126; adl[69].carChar3 = 139;
    adl[69].modChar4 = 23;  adl[69].carChar4 = 12;
    adl[69].modChar5 = 1;   adl[69].carChar5 = 1;
    adl[69].fbConn = 54;    adl[69].percNote = 0;
    
    // GM71: Bassoon
    adl[70].modChar1 = 49;  adl[70].carChar1 = 50;
    adl[70].modChar2 = 129; adl[70].carChar2 = 0;
    adl[70].modChar3 = 117; adl[70].carChar3 = 97;
    adl[70].modChar4 = 25;  adl[70].carChar4 = 25;
    adl[70].modChar5 = 1;   adl[70].carChar5 = 0;
    adl[70].fbConn = 48;    adl[70].percNote = 0;
    
    // GM72: Clarinet
    adl[71].modChar1 = 50;  adl[71].carChar1 = 33;
    adl[71].modChar2 = 144; adl[71].carChar2 = 0;
    adl[71].modChar3 = 155; adl[71].carChar3 = 114;
    adl[71].modChar4 = 33;  adl[71].carChar4 = 23;
    adl[71].modChar5 = 0;   adl[71].carChar5 = 0;
    adl[71].fbConn = 52;    adl[71].percNote = 0;
    
    // GM73: Piccolo
    adl[72].modChar1 = 225; adl[72].carChar1 = 225;
    adl[72].modChar2 = 31;  adl[72].carChar2 = 0;
    adl[72].modChar3 = 133; adl[72].carChar3 = 101;
    adl[72].modChar4 = 95;  adl[72].carChar4 = 26;
    adl[72].modChar5 = 0;   adl[72].carChar5 = 0;
    adl[72].fbConn = 48;    adl[72].percNote = 0;
    
    // GM74: Flute
    adl[73].modChar1 = 225; adl[73].carChar1 = 225;
    adl[73].modChar2 = 70;  adl[73].carChar2 = 0;
    adl[73].modChar3 = 136; adl[73].carChar3 = 101;
    adl[73].modChar4 = 95;  adl[73].carChar4 = 26;
    adl[73].modChar5 = 0;   adl[73].carChar5 = 0;
    adl[73].fbConn = 48;    adl[73].percNote = 0;
    
    // GM75: Recorder
    adl[74].modChar1 = 161; adl[74].carChar1 = 33;
    adl[74].modChar2 = 156; adl[74].carChar2 = 0;
    adl[74].modChar3 = 117; adl[74].carChar3 = 117;
    adl[74].modChar4 = 31;  adl[74].carChar4 = 10;
    adl[74].modChar5 = 0;   adl[74].carChar5 = 0;
    adl[74].fbConn = 50;    adl[74].percNote = 0;
    
    // GM76: Pan Flute
    adl[75].modChar1 = 49;  adl[75].carChar1 = 33;
    adl[75].modChar2 = 139; adl[75].carChar2 = 0;
    adl[75].modChar3 = 132; adl[75].carChar3 = 101;
    adl[75].modChar4 = 88;  adl[75].carChar4 = 26;
    adl[75].modChar5 = 0;   adl[75].carChar5 = 0;
    adl[75].fbConn = 48;    adl[75].percNote = 0;
    
    // GM77: Bottle Blow
    adl[76].modChar1 = 225; adl[76].carChar1 = 161;
    adl[76].modChar2 = 76;  adl[76].carChar2 = 0;
    adl[76].modChar3 = 102; adl[76].carChar3 = 101;
    adl[76].modChar4 = 86;  adl[76].carChar4 = 38;
    adl[76].modChar5 = 0;   adl[76].carChar5 = 0;
    adl[76].fbConn = 48;    adl[76].percNote = 0;
    
    // GM78: Shakuhachi
    adl[77].modChar1 = 98;  adl[77].carChar1 = 161;
    adl[77].modChar2 = 203; adl[77].carChar2 = 0;
    adl[77].modChar3 = 118; adl[77].carChar3 = 85;
    adl[77].modChar4 = 70;  adl[77].carChar4 = 54;
    adl[77].modChar5 = 0;   adl[77].carChar5 = 0;
    adl[77].fbConn = 48;    adl[77].percNote = 0;
    
    // GM79: Whistle
    adl[78].modChar1 = 98;  adl[78].carChar1 = 161;
    adl[78].modChar2 = 153; adl[78].carChar2 = 0;
    adl[78].modChar3 = 87;  adl[78].carChar3 = 86;
    adl[78].modChar4 = 7;   adl[78].carChar4 = 7;
    adl[78].modChar5 = 0;   adl[78].carChar5 = 0;
    adl[78].fbConn = 59;    adl[78].percNote = 0;
    
    // GM80: Ocarina
    adl[79].modChar1 = 98;  adl[79].carChar1 = 161;
    adl[79].modChar2 = 147; adl[79].carChar2 = 0;
    adl[79].modChar3 = 119; adl[79].carChar3 = 118;
    adl[79].modChar4 = 7;   adl[79].carChar4 = 7;
    adl[79].modChar5 = 0;   adl[79].carChar5 = 0;
    adl[79].fbConn = 59;    adl[79].percNote = 0;
    
    // GM81: Lead 1 squareea
    adl[80].modChar1 = 34;  adl[80].carChar1 = 33;
    adl[80].modChar2 = 89;  adl[80].carChar2 = 0;
    adl[80].modChar3 = 255; adl[80].carChar3 = 255;
    adl[80].modChar4 = 3;   adl[80].carChar4 = 15;
    adl[80].modChar5 = 2;   adl[80].carChar5 = 0;
    adl[80].fbConn = 48;    adl[80].percNote = 0;
    
    // GM82: Lead 2 sawtooth
    adl[81].modChar1 = 33;  adl[81].carChar1 = 33;
    adl[81].modChar2 = 14;  adl[81].carChar2 = 0;
    adl[81].modChar3 = 255; adl[81].carChar3 = 255;
    adl[81].modChar4 = 15;  adl[81].carChar4 = 15;
    adl[81].modChar5 = 1;   adl[81].carChar5 = 1;
    adl[81].fbConn = 48;    adl[81].percNote = 0;
    
    // GM83: Lead 3 calliope
    adl[82].modChar1 = 34;  adl[82].carChar1 = 33;
    adl[82].modChar2 = 70;  adl[82].carChar2 = 128;
    adl[82].modChar3 = 134; adl[82].carChar3 = 100;
    adl[82].modChar4 = 85;  adl[82].carChar4 = 24;
    adl[82].modChar5 = 0;   adl[82].carChar5 = 0;
    adl[82].fbConn = 48;    adl[82].percNote = 0;
    
    // GM84: Lead 4 chiff
    adl[83].modChar1 = 33;  adl[83].carChar1 = 161;
    adl[83].modChar2 = 69;  adl[83].carChar2 = 0;
    adl[83].modChar3 = 102; adl[83].carChar3 = 150;
    adl[83].modChar4 = 18;  adl[83].carChar4 = 10;
    adl[83].modChar5 = 0;   adl[83].carChar5 = 0;
    adl[83].fbConn = 48;    adl[83].percNote = 0;
}
