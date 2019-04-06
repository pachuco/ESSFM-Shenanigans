#include <stdio.h>
#include <windows.h>
#include "_file.c"
#include "_inout.c"
#include "_console.c"

//Base address of soundcard's FM ports
//I believe this will be BAR 1
//Ports are 0x00 - 0x0F
//Define this very carefully!
#define FMBASE 0xDE00

//Some entries to describe our data file
#define DAT_BANKOFF 0x8C40
#define DAT_INSLEN 0x1C

void IO_write8(UCHAR port, UCHAR data) {
    DlPortWritePortUchar(FMBASE + port, data);
    QPCuWait(50);
}

void FM_startSynth() {
    IO_write8(0x04, 72);
    IO_write8(0x04, 72);
    IO_write8(0x05, 0);
    IO_write8(0x04, 127);
    IO_write8(0x04, 127);
    IO_write8(0x05, 0);
    IO_write8(0x04, 54);
    IO_write8(0x05, 119);//153
    IO_write8(0x04, 107);
    IO_write8(0x05, 0);
    IO_write8(0x07, 66);
    IO_write8(0x02, 5);
    IO_write8(0x01, 128);
}

void FM_stopSynth() {
    IO_write8(0x04, 72);
    IO_write8(0x04, 72);
    IO_write8(0x05, 16);
    IO_write8(0x07, 98);
}

int main(int argc, char *argv[]) {
    static MemFile essdat;
    static Log log;
    
    if (!initInOut()) {
        printf("Failed to init InpOut!\n");
        return 1;
    }
    if (!loadFileToMem("ess.dat", &essdat)) {
        printf("Cannot load ESS data file!\n");
        return 1;
    }
    if( argc < 2 ) {
        FM_startSynth();
        pressAnyKey();
        FM_stopSynth();
    } else {
        if (!loadLog(argv[1], &log)) {
            printf("Could not load logfile!\n");
            return 1;
        }
        FM_startSynth();
        pressAnyKey();
        FM_stopSynth();
    }
    
    //printf("%d\n", 0b11000);
    
    return 0;
}