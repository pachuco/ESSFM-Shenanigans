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

void TUI_displayLog(Log* log, int index) {
    COORD scrSize;
    int vertMiddle;
    //return;
    getScreenSize(&scrSize);
    vertMiddle = scrSize.Y / 2;
    clearScreen();
    for (int i=0; i < scrSize.Y; i++) {
        int off = index + i - vertMiddle;
        if (i != 0) printf("\n");
        if (off >= 0 && off < log->dataSize) {
            LogRow* lr = &log->data[off];
            printf("%07d³%2.8f³h%1X³", off, lr->duration, lr->port);
        } else {
            printf(".......³..........³..³");
        }
    }
}

int main(int argc, char *argv[]) {
    static MemFile essdat;
    static Log log;
    
    if (!initInOut()) {
        printf("Failed to init InpOut!\n");
        return 1;
    }
    if (!loadFileToMem(&essdat, "ess.dat")) {
        printf("Cannot load ESS data file!\n");
        return 1;
    }
    
    initTUIConsole(KEY_EVENT);
    
    if( argc < 2 ) { //piano mode
        FM_startSynth();
        pressAnyKey();
        FM_stopSynth();
    } else { //log player mode
        BOOL isProgActive=TRUE, isPlaying=FALSE;
        ScreenBuffer sBuf; memset(&sBuf, 0, sizeof(ScreenBuffer));
        int index=0;
        COORD screenSize = {128, 128};
        
        if (!loadLog(&log, argv[1])) {
            printf("Could not load logfile!\n");
            return 1;
        }
        resizeScreenBuf(&screenSize);
        
        
        while(isProgActive) {
            WORD vk; KSTATE ks;
            
            if (validateScreenBuf(&sBuf)) {
                TUI_displayLog(&log, index);
            }
            
            if (isPlaying) {
            } else {
                ks = getKeyVK(&vk);
                if (ks & KEY_DOWN) {
                    if (vk == VK_F1) {
                    } else if (vk == VK_PRIOR) {
                        
                    } else if (vk == VK_NEXT) {
                        
                    } else if (vk == VK_UP) {
                        if (index > 0) index--;
                        TUI_displayLog(&log, index);
                    } else if (vk == VK_DOWN) {
                        if (index < log.dataSize) index++;
                        TUI_displayLog(&log, index);
                    }
                    //non-repeating keys
                    if (ks & KEY_HEAD) { //typematic keys
                        if (vk == VK_ESCAPE) {
                            isProgActive = FALSE;
                            clearScreen();
                        } else if (vk == VK_SPACE) {
                        }
                    } 
                }
            }
            //if
            resizeScreenBuf(NULL);
            SleepEx(1, 1);
        }
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