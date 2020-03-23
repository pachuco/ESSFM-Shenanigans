#include <windows.h>
#include "support.h"
#include "file.h"

//Base address of soundcard's FM ports
//I believe this will be BAR 1
//Ports are 0x00 - 0x0F
//Define this very carefully!
#define FMBASE 0xDE00
#define INNACURATE_USEC_WAIT 10

//Some entries to describe our data file
#define DAT_BANKOFF 0x8C40
#define DAT_INSLEN 0x1C

void IO_write8(UCHAR port, UCHAR data) {
    #ifdef ENABLE_HARDWARE_ACCESS
        DlPortWritePortUchar(FMBASE + port, data);
    #endif
    QPCuWait(INNACURATE_USEC_WAIT);
}

void FM_startSynth() {
    //these are probably not right
    IO_write8(0x04, 72);
    IO_write8(0x04, 72);
    IO_write8(0x05, 0);
    IO_write8(0x04, 127);
    IO_write8(0x04, 127);
    IO_write8(0x05, 0);
    IO_write8(0x04, 54);
    IO_write8(0x05, 153);//119
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

