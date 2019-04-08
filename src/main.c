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

void TUI_displayLog(ScreenBuffer* sBuf, Log* log, int index) {
    SHORT vertMiddle = sBuf->wndSize.Y / 2;
    WORD bckgAttr  = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    WORD pointAttr = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;
    
    paintAttributeRect(sBuf, (SMALL_RECT){0, 0, sBuf->wndSize.X, sBuf->wndSize.Y}, bckgAttr);
    paintAttributeRect(sBuf, (SMALL_RECT){0, vertMiddle, sBuf->wndSize.X, vertMiddle+1}, pointAttr);
    for (int i=0; i < sBuf->wndSize.Y; i++) {
        int off = index + i - vertMiddle;
        if (i != 0) printf("\n");
        if (log && off >= 0 && off < log->dataSize) {
            LogRow* lr = &log->data[off];
            
            writeTextLine(sBuf, (COORD){0, i}, "%07d\xB3%2.8f\xB3h%1X\xB3h%02X\xB3", off, lr->duration, lr->port, lr->data);
        } else {
            writeTextLine(sBuf, (COORD){0, i}, ".......\xB3..........\xB3..\xB3...\xB3");
        }
    }
    updateRegion(sBuf, (SMALL_RECT){0, 0, sBuf->wndSize.X, sBuf->wndSize.Y});
}

int main(int argc, char *argv[]) {
    static MemFile essdat;
    static Log log;
    
    if (!initInOut()) {
        printf("Failed to init InpOut!\n");
        return 1;
    }
    //if (!loadFileToMem(&essdat, "ess.dat")) {
    //    printf("Cannot load ESS data file!\n");
    //    return 1;
    //}
    
    { //log player mode
        BOOL isProgActive=TRUE, isPlaying=FALSE;
        ScreenBuffer sBuf; memset(&sBuf, 0, sizeof(ScreenBuffer));
        int index=0;
        
        initTUIConsole(&sBuf, KEY_EVENT);
        
        if (argc >= 2) {
            if (!loadLog(&log, argv[1])) {
            }
        }
        
        while(isProgActive) {
            WORD vk; KSTATE ks;
            
            consumeEvents();
            validateScreenBuf(&sBuf);
            
            if (sBuf.needsRedraw) {
                clearScreen();
                TUI_displayLog(&sBuf, &log, index);
                sBuf.needsRedraw = FALSE;
            }
            
            if (isPlaying) {
            } else {
                ks = getKeyVK(&vk);
                if (ks & KEY_DOWN) {
                    BOOL invLog = FALSE;
                    
                    if (vk == VK_F1) {
                    } else if (vk == VK_PRIOR) {
                        index -= sBuf.wndSize.Y;
                        if (index < 0) index = 0;
                        invLog = TRUE;
                    } else if (vk == VK_NEXT) {
                        index += sBuf.wndSize.Y;
                        if (index >= log.dataSize) index = log.dataSize;
                        invLog = TRUE;
                    } else if (vk == VK_UP) {
                        if (index > 0) index--;
                        invLog = TRUE;
                    } else if (vk == VK_DOWN) {
                        if (index < log.dataSize) index++;
                        invLog = TRUE;
                    }
                    //non-repeating keys
                    if (ks & KEY_HEAD) { //typematic keys
                        if (vk == VK_ESCAPE) {
                            isProgActive = FALSE;
                        } else if (vk == VK_SPACE) {
                            
                        }
                    }
                    if (invLog) TUI_displayLog(&sBuf, &log, index);
                }
            }
            //if
            SleepEx(1, 1);
        }
        clearScreen();
    }
    
    //printf("%d\n", 0b11000);
    
    return 0;
}

void sgfdsgQPCuWait(DWORD uSecTime) {
    static LONGLONG freq=0;
    LONGLONG start=0, cur=0, llTime=0;
    
    if (freq == 0) QueryPerformanceFrequency((PLARGE_INTEGER)&freq);
    if (freq != 0) {
        llTime = ((LONGLONG)uSecTime * freq)/1000000;
        QueryPerformanceCounter((PLARGE_INTEGER)&start);
        while (cur < (start + llTime)) {
            SwitchToThread();
            QueryPerformanceCounter((PLARGE_INTEGER)&cur);
        }
    } else {
        //TODO: alternate timing mechanism
    }
}