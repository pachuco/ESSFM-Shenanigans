#include <windows.h>
#include <assert.h>
#include "support.h"
#include "file.h"
#include <buttio.h>

//Base address of soundcard's FM ports
//I believe this will be BAR 1
//Ports are 0x00 - 0x0F
//Define this very carefully!
#define FMBASE 0xDE00
#define INNACURATE_USEC_WAIT 10

//Some entries to describe our data file
#define DAT_BANKOFF 0x8C40
#define DAT_INSLEN 0x1C

IOHandler g_ioHand = {0};

BOOL initInOut() {
    if (!buttio_init(&g_ioHand, NULL, BUTTIO_MET_IOPM)) return FALSE;
    
    if (1) {
        ALLOWIO(g_ioHand.iopm, FMBASE + 0);
        iopm_fillRange(g_ioHand.iopm, FMBASE + 1, FMBASE + 0xF, TRUE);
    } else {
        iopm_fillRange(g_ioHand.iopm, FMBASE + 0, FMBASE + 0xF, TRUE);
        //ALLOWIO(g_ioHand.iopm, FMBASE + 0);
        //ALLOWIO(g_ioHand.iopm, FMBASE + 1);
        //ALLOWIO(g_ioHand.iopm, FMBASE + 2);
        //ALLOWIO(g_ioHand.iopm, FMBASE + 3);
        //ALLOWIO(g_ioHand.iopm, FMBASE + 4);
        //ALLOWIO(g_ioHand.iopm, FMBASE + 5);
        //ALLOWIO(g_ioHand.iopm, FMBASE + 6);
        //ALLOWIO(g_ioHand.iopm, FMBASE + 7);
        //ALLOWIO(g_ioHand.iopm, FMBASE + 8);
        //ALLOWIO(g_ioHand.iopm, FMBASE + 9);
        //ALLOWIO(g_ioHand.iopm, FMBASE + 10);
        //ALLOWIO(g_ioHand.iopm, FMBASE + 11);
        //ALLOWIO(g_ioHand.iopm, FMBASE + 12);
        //ALLOWIO(g_ioHand.iopm, FMBASE + 13);
        //ALLOWIO(g_ioHand.iopm, FMBASE + 14);
        //ALLOWIO(g_ioHand.iopm, FMBASE + 15);
        //BLOCKIO(g_ioHand.iopm, FMBASE + 15);
    }
    assert(CHECKIO(g_ioHand.iopm, FMBASE + 0));
    assert(CHECKIO(g_ioHand.iopm, FMBASE + 1));
    assert(CHECKIO(g_ioHand.iopm, FMBASE + 2));
    assert(CHECKIO(g_ioHand.iopm, FMBASE + 3));
    assert(CHECKIO(g_ioHand.iopm, FMBASE + 4));
    assert(CHECKIO(g_ioHand.iopm, FMBASE + 5));
    assert(CHECKIO(g_ioHand.iopm, FMBASE + 6));
    assert(CHECKIO(g_ioHand.iopm, FMBASE + 7));
    assert(CHECKIO(g_ioHand.iopm, FMBASE + 8));
    assert(CHECKIO(g_ioHand.iopm, FMBASE + 9));
    assert(CHECKIO(g_ioHand.iopm, FMBASE + 10));
    assert(CHECKIO(g_ioHand.iopm, FMBASE + 11));
    assert(CHECKIO(g_ioHand.iopm, FMBASE + 12));
    assert(CHECKIO(g_ioHand.iopm, FMBASE + 13));
    assert(CHECKIO(g_ioHand.iopm, FMBASE + 14));
    assert(CHECKIO(g_ioHand.iopm, FMBASE + 15));
    buttio_flushIOPMChanges(&g_ioHand);
    
    return TRUE;
}

void shutdownInOut(void) {
    buttio_shutdown(&g_ioHand);
}

//not very accurate
void QPCuWait(DWORD uSecTime) { //KeStallExecutionProcessor
    static LONGLONG freq=0;
    LONGLONG start=0, cur=0, wait=0;
    
    if (freq == 0) QueryPerformanceFrequency((PLARGE_INTEGER)&freq);
    if (freq != 0) {
        wait = ((LONGLONG)uSecTime * freq)/(LONGLONG)1000000;
        QueryPerformanceCounter((PLARGE_INTEGER)&start);
        while (cur < (start + wait)) {
            __asm__("pause");
            QueryPerformanceCounter((PLARGE_INTEGER)&cur);
        }
    } else {
        //TODO: alternate timing mechanism
    }
}

void IO_write8(UCHAR port, UCHAR data) {
    #ifdef ENABLE_HARDWARE_ACCESS
        buttio_wu8(&g_ioHand, FMBASE + port, data);
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

