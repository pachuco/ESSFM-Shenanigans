#include <stdio.h>
#include <windows.h>
#include "_file.c"
#include "_inout.c"
#include "_console.c"
#include "_tests.c"

//Base address of soundcard's FM ports
//I believe this will be BAR 1
//Ports are 0x00 - 0x0F
//Define this very carefully!
#define FMBASE 0xDE00
#define INNACURATE_USEC_WAIT 15

//Some entries to describe our data file
#define DAT_BANKOFF 0x8C40
#define DAT_INSLEN 0x1C

void IO_write8(UCHAR port, UCHAR data) {
    DlPortWritePortUchar(FMBASE + port, data);
    QPCuWait(INNACURATE_USEC_WAIT);
}
void IO_writeLogRow8(SongRow* sRow) {
    DlPortWritePortUchar(FMBASE + sRow->port, sRow->data);
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


void drawTUI(ScreenBuffer* sBuf, Song* song, int index) {
    static int vertDelim = 2;
    static WORD bckgAttr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    
    paintAttributeRect(sBuf, (SMALL_RECT){0, 0, sBuf->wndSize.X, sBuf->wndSize.Y}, bckgAttr);
    
    {
        //log
        SHORT vertMiddle = (sBuf->wndSize.Y - vertDelim) / 2;
        WORD pointAttr = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;
        
        paintAttributeRect(sBuf, (SMALL_RECT){0, vertDelim+vertMiddle, sBuf->wndSize.X-1, vertDelim+vertMiddle+1}, pointAttr);
        for (int i=0; i < sBuf->wndSize.Y - vertDelim; i++) {
            int off = index + i - vertMiddle;
            
            if (song && off >= 0 && off < song->dataSize) {
                SongRow* sRow = &song->rows[off];
                
                writeTextLine(sBuf, (COORD){0, vertDelim + i}, sBuf->wndSize.X - 1, "%07d\xB3%11.8f\xB3h%1X\xB3h%02X\xB3", off, sRow->duration, sRow->port, sRow->data);
            } else {
                writeTextLine(sBuf, (COORD){0, vertDelim + i}, sBuf->wndSize.X - 1, ".......\xB3...........\xB3..\xB3...\xB3");
            }
        }
    }
    updateRegion(sBuf, (SMALL_RECT){0, 0, sBuf->wndSize.X, sBuf->wndSize.Y});
}

BOOL loadSong(PCHAR inPath, PCHAR outFileName, Song* outSong) {
    char ext[8];
    BOOL success = TRUE;
    
    exPartFromPath(ext, inPath, 8, EXPTH_EXTENSION);
    strupr(ext);
    if        (!strcmp(ext, "LOG")) {
        if (!loadDbgViewLog(outSong, inPath)) success = FALSE;
        //if (success) TEST_ESS_doesPort3WriteOnly012(outSong);
    } else if (!strcmp(ext, "RAW")) {
        if (!loadRdosRawOpl(outSong, inPath)) success = FALSE;
    } else {
        success = FALSE;
    }
    if (success && outFileName) {
        exPartFromPath(outFileName, inPath, 256, EXPTH_FNAME);
        SetConsoleTitleA(outFileName);
    }
        
    return success;
}


typedef enum {
    ACT_REDRAW  = 1<<0,
    ACT_LOAD    = 1<<1,
    ACT_PAUSE   = 1<<2,
    ACT_PLAY    = 1<<3,
    ACT_EXIT    = 1<<4,
    ACT_PGUP    = 1<<5,
    ACT_PGDOWN  = 1<<6,
    ACT_UP      = 1<<7,
    ACT_DOWN    = 1<<8,
} ACTIONS;

int main(int argc, char *argv[]) {
    static char filePath[2048];
    static char fileName[256];
    static MemFile essdat;
    static Song song;
    static Song* pSong = NULL;
    
    if (!initInOut()) {
        printf("Failed to init InpOut!\n");
        return 1;
    }
    //if (!loadFileToMem(&essdat, "ess.dat")) {
    //    printf("Cannot load ESS data file!\n");
    //    return 1;
    //}
    
    { //song player mode
        BOOL isProgActive=TRUE, isPlaying=FALSE;
        ScreenBuffer screen; memset(&screen, 0, sizeof(ScreenBuffer));
        int index=0;
        LONGLONG baseClock;
        LONGLONG lastMusTime, curMusWait;
        LONGLONG lastGuiTime, guiWait;
        BOOL isCtrlDown = FALSE;
        
        QueryPerformanceFrequency((PLARGE_INTEGER)&baseClock);
        lastMusTime = curMusWait = 0;
        lastGuiTime = 0;
        guiWait = (LONGLONG)baseClock * 0.1;
        
        initTUIConsole(&screen, KEY_EVENT);
        
        if (argc >= 2) {
            if (loadSong(argv[1], fileName, &song)) {
                pSong = &song;
            }
        }
        
        FM_startSynth();
        while(isProgActive) {
            WORD vk; KSTATE ks;
            SongRow* sRow = NULL;
            LONGLONG curTime;
            BOOL doIncrement;
            DWORD action;
            
            
            if (isPlaying) {
                QueryPerformanceCounter((PLARGE_INTEGER)&curTime);
                //playback
                if(curTime - lastMusTime > curMusWait) {
                    do {
                        if (doIncrement) {
                            index++;
                            if (index >= pSong->dataSize) {
                                isPlaying = FALSE;
                                index = pSong->dataSize - 1;
                                action |= ACT_REDRAW;
                            }
                            doIncrement = FALSE;
                        }
                        sRow = &pSong->rows[index];
                        curMusWait = (LONGLONG)((double)sRow->duration * baseClock);
                        IO_writeLogRow8(sRow);
                        QPCuWait(INNACURATE_USEC_WAIT);
                        lastMusTime = curTime;
                        
                        doIncrement = TRUE;
                    } while (curMusWait == 0.0 && isPlaying);
                }
                
                //gui
                if(curTime - lastGuiTime > guiWait) {
                    consumeEvents();
                    ks = getKeyVK(&vk);
                    
                    validateScreenBuf(&screen);
                    
                    action |= ACT_REDRAW;
                    lastGuiTime = curTime;
                }
                SwitchToThread();
                
            } else {
                consumeEvents();
                ks = getKeyVK(&vk);
                if (!validateScreenBuf(&screen)) action |= ACT_REDRAW;
                SleepEx(1, 1);
            }
            
            //key handling
            if (ks & KEY_ACTV) {
                if (ks & KEY_DOWN) {
                    if (ks & KEY_HEAD) {
                        switch (vk) {
                            case VK_ESCAPE:     action |= ACT_EXIT;  break;
                            case VK_SPACE:      action |= isPlaying ? ACT_PAUSE : ACT_PLAY; break;
                            case VK_F3:         if (!isPlaying) action |= ACT_LOAD; break;
                        }
                    }
                    switch (vk) {
                        case VK_PRIOR:      if (!isPlaying && pSong) action |= ACT_PGUP;   break;
                        case VK_NEXT:       if (!isPlaying && pSong) action |= ACT_PGDOWN; break;
                        case VK_UP:         if (!isPlaying && pSong) action |= ACT_UP;     break;
                        case VK_DOWN:       if (!isPlaying && pSong) action |= ACT_DOWN;   break;
                        
                        case VK_CONTROL:    isCtrlDown = TRUE;  break;
                    }
                } else {
                    if (vk == VK_CONTROL) isCtrlDown = FALSE;
                }
            }
            
            //actions
            if (action) {
                if (action & ACT_LOAD) {
                    OPENFILENAMEA ofna;
                    #define F_LOG "DbgView log\0*.LOG\0"
                    #define F_RAW "RDOS raw opl\0*.RAW\0"
                    #define F_ANY "Any file\0*.*\0"
                    
                    ofna.lStructSize        = sizeof(OPENFILENAMEA);
                    ofna.hwndOwner          = GetConsoleWindow();
                    ofna.hInstance          = NULL;
                    ofna.lpstrFilter        = F_ANY F_LOG F_RAW "\0";
                    ofna.lpstrCustomFilter  = NULL;
                    ofna.nMaxCustFilter     = 0;
                    ofna.nFilterIndex       = 1;
                    ofna.lpstrFile          = filePath;
                    ofna.nMaxFile           = 1024;
                    ofna.lpstrFileTitle     = fileName;
                    ofna.nMaxFileTitle      = 0;
                    ofna.lpstrInitialDir    = NULL;
                    ofna.lpstrTitle         = NULL;
                    ofna.Flags              = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
                    ofna.nFileOffset        = 0;
                    ofna.nFileExtension     = 0;
                    ofna.lpstrDefExt        = NULL;
                    ofna.lCustData          = 0;
                    ofna.lpfnHook           = NULL;
                    ofna.lpTemplateName     = NULL;
                    ofna.pvReserved         = NULL;
                    ofna.dwReserved         = 0;
                    ofna.FlagsEx            = 0;
                    
                    if (GetOpenFileNameA(&ofna) && loadSong(ofna.lpstrFile, fileName, &song)) {
                        index = 0;
                        pSong = &song;
                        action |= ACT_REDRAW;
                    }
                }
                
                if (action & ACT_PLAY) {
                    doIncrement = FALSE;
                    if (pSong) isPlaying = TRUE;
                }
                
                if (action & ACT_PAUSE) {
                    isPlaying = FALSE;
                }
                
                if (action & ACT_EXIT) {
                    isProgActive = FALSE;
                }
                
                if (action & ACT_PGUP) {
                    int oldInd = index;
                    
                    index -= screen.wndSize.Y;
                    if (index < 0) index = 0;
                    if (isCtrlDown) for (int i=oldInd; i >= index; i--) {
                        IO_writeLogRow8(&pSong->rows[i]);
                        QPCuWait(INNACURATE_USEC_WAIT);
                    }
                    action |= ACT_REDRAW;
                }
                
                if (action & ACT_PGDOWN) {
                    int oldInd = index;
                    
                    index += screen.wndSize.Y;
                    if (index >= pSong->dataSize) index = pSong->dataSize - 1;
                    if (isCtrlDown) for (int i=oldInd; i < index; i++) {
                        IO_writeLogRow8(&pSong->rows[i]);
                        QPCuWait(INNACURATE_USEC_WAIT);
                    }
                    action |= ACT_REDRAW;
                }
                
                if (action & ACT_UP) {
                    if (isCtrlDown) IO_writeLogRow8(&pSong->rows[index]);
                    if (index > 0) index--;
                    action |= ACT_REDRAW;
                }
                
                if (action & ACT_DOWN) {
                    if (isCtrlDown) IO_writeLogRow8(&pSong->rows[index]);
                    index++;
                    if (index >= pSong->dataSize) index = pSong->dataSize - 1;
                    action |= ACT_REDRAW;
                }
                
                if (action & ACT_REDRAW) {
                    drawTUI(&screen, pSong, index);
                }
                
                action = 0;
            }
        }
        FM_stopSynth();
        clearScreen();
        if (pSong && pSong->rows) free(pSong->rows);
    }
    
    //printf("%d\n", 0b11000);
    
    return 0;
}