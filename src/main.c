#include <stdio.h>
#include <windows.h>
#include "_file.c"
#include "_inout.c"
#include "_console.c"

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
            
            writeTextLine(sBuf, (COORD){0, i}, sBuf->wndSize.X - 1, "%07d\xB3%2.8f\xB3h%1X\xB3h%02X\xB3", off, lr->duration, lr->port, lr->data);
        } else {
            writeTextLine(sBuf, (COORD){0, i}, sBuf->wndSize.X - 1, ".......\xB3..........\xB3..\xB3...\xB3");
        }
    }
    updateRegion(sBuf, (SMALL_RECT){0, 0, sBuf->wndSize.X, sBuf->wndSize.Y});
}

//-----------------------------------------

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
    QPCuWait(10);
}
void IO_writeLogRow8(LogRow* lr) {
    DlPortWritePortUchar(FMBASE + lr->port, lr->data);
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

int main(int argc, char *argv[]) {
    static char filePath[2048];
    static MemFile essdat;
    static Log log;
    static Log* pLog;
    
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
        LONGLONG baseClock;
        LONGLONG lastMusTime, curMusWait;
        LONGLONG lastGuiTime, guiWait;
        BOOL isCtrlDown = FALSE;
        
        QueryPerformanceFrequency((PLARGE_INTEGER)&baseClock);
        lastMusTime = curMusWait = 0;
        lastGuiTime = 0;
        guiWait = (LONGLONG)baseClock * 0.1;
        
        initTUIConsole(&sBuf, KEY_EVENT);
        
        if (argc >= 2) {
            if (loadLog(&log, argv[1])) {
                pLog = &log;
            }
        }
        
        FM_startSynth();
        while(isProgActive) {
            WORD vk; KSTATE ks;
            LogRow* lr = NULL;
            LONGLONG curTime;
            BOOL doIncrement;
            
            
            if (isPlaying) {
                //playback
                QueryPerformanceCounter((PLARGE_INTEGER)&curTime);
                if(curTime - lastMusTime > curMusWait) {
                    if (doIncrement) {
                        index++;
                        if (index >= pLog->dataSize) {
                            isPlaying = FALSE;
                            index = pLog->dataSize - 1;
                            TUI_displayLog(&sBuf, pLog, index);
                        }
                        doIncrement = FALSE;
                    }
                    lr = &pLog->data[index];
                    curMusWait = (LONGLONG)((double)lr->duration * baseClock);
                    lastMusTime = curTime;
                    
                    IO_writeLogRow8(lr);
                    doIncrement = TRUE;
                }
                
                //gui
                QueryPerformanceCounter((PLARGE_INTEGER)&curTime);
                if(curTime - lastGuiTime > guiWait) {
                    ks = getKeyVK(&vk);
                    if (ks & KEY_ACTV && ks & KEY_DOWN) {
                        if (ks & KEY_HEAD) {
                            if (vk == VK_ESCAPE) {
                                isProgActive = FALSE;
                            } else if (vk == VK_SPACE) {
                                isPlaying = FALSE;
                            }
                        }
                    }
                    
                    consumeEvents();
                    validateScreenBuf(&sBuf);
                    if (sBuf.needsRedraw) {
                        clearScreen();
                        sBuf.needsRedraw = FALSE;
                    }
                    
                    TUI_displayLog(&sBuf, pLog, index);
                    lastGuiTime = curTime;
                }
                SwitchToThread();
            } else {
                consumeEvents();
                validateScreenBuf(&sBuf);
                if (sBuf.needsRedraw) {
                    clearScreen();
                    TUI_displayLog(&sBuf, pLog, index);
                    sBuf.needsRedraw = FALSE;
                }
                
                ks = getKeyVK(&vk);
                if (ks & KEY_ACTV) {
                    if (ks & KEY_DOWN) {
                        BOOL invLog = FALSE;
                        
                        //typematic keys
                        if (vk == VK_F1) {
                        } else if (vk == VK_PRIOR) {
                            int oldInd = index;
                            
                            index -= sBuf.wndSize.Y;
                            if (index < 0) index = 0;
                            if (isCtrlDown) for (int i=oldInd; i >= index; i--) IO_writeLogRow8(&pLog->data[i]);
                            invLog = TRUE;
                        } else if (vk == VK_NEXT) {
                            int oldInd = index;
                            
                            index += sBuf.wndSize.Y;
                            if (index >= pLog->dataSize) index = pLog->dataSize - 1;
                            if (isCtrlDown) for (int i=oldInd; i < index; i++) IO_writeLogRow8(&pLog->data[i]);
                            invLog = TRUE;
                        } else if (vk == VK_UP) {
                            if (isCtrlDown) IO_writeLogRow8(&pLog->data[index]);
                            if (index > 0) index--;
                            invLog = TRUE;
                        } else if (vk == VK_DOWN) {
                            if (isCtrlDown) IO_writeLogRow8(&pLog->data[index]);
                            index++;
                            if (index >= pLog->dataSize) index = pLog->dataSize - 1;
                            invLog = TRUE;
                        }
                        //non-repeating keys
                        if (ks & KEY_HEAD) {
                            if (vk == VK_ESCAPE) {
                                isProgActive = FALSE;
                            } else if (vk == VK_SPACE) {
                                doIncrement = FALSE;
                                if (pLog) isPlaying = TRUE;
                            } else if (vk == VK_F3) {
                                OPENFILENAMEA ofna;
                                
                                ofna.lStructSize        = sizeof(OPENFILENAMEA);
                                ofna.hwndOwner          = GetConsoleWindow();
                                ofna.hInstance          = NULL;
                                ofna.lpstrFilter        = "DbgView log\0*.LOG\0Any file\0*.*\0\0";
                                ofna.lpstrCustomFilter  = NULL;
                                ofna.nMaxCustFilter     = 0;
                                ofna.nFilterIndex       = 1;
                                ofna.lpstrFile          = filePath;
                                ofna.nMaxFile           = 1024;
                                ofna.lpstrFileTitle     = NULL;
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
                                if (GetOpenFileNameA(&ofna) && loadLog(&log, ofna.lpstrFile)) {
                                    pLog = &log;
                                    invLog = TRUE;
                                }
                            } else if (vk == VK_CONTROL) {
                                isCtrlDown = TRUE;
                            }
                        }
                        if (invLog) TUI_displayLog(&sBuf, pLog, index);
                    } else {
                        if (ks & KEY_HEAD) {
                            if (vk == VK_CONTROL) {
                                isCtrlDown = FALSE;
                            }
                        }
                    }
                }
                SleepEx(1, 1);
            }
        }
        FM_stopSynth();
        clearScreen();
    }
    
    //printf("%d\n", 0b11000);
    
    return 0;
}