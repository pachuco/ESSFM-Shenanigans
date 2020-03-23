#include <windows.h>
#include "logplayer.h"
#include "file.h"

static HANDLE hThread;
static LogPlayer lpl;
static struct {
    DWORD doFlags;
    Song* newSong;
    BOOL  isPlay;
} cmnd;

enum {
    DF_EXIT     = 1<<0,
    DF_LOADSONG = 1<<1,
    DF_PLAY     = 1<<2
};

static DWORD WINAPI playThread(LPVOID lpParam) {
    LONGLONG baseClock;
    LONGLONG curWait, curTime, lastTime;
    SongRow* sRow = NULL;
    
    curWait = lastTime = 0;
    lpl.isPlaying   = FALSE;
    lpl.curIndex    = 0;
    lpl.song        = NULL;
    
    QueryPerformanceFrequency((PLARGE_INTEGER)&baseClock);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    
    lpl.isInit = TRUE;
    while(lpl.isInit) {
        if (lpl.isPlaying) {
            QueryPerformanceCounter((PLARGE_INTEGER)&curTime);
            if (!sRow) {
                sRow = &lpl.song->rows[lpl.curIndex];
                IO_write8(sRow->port, sRow->data);
                curWait = (LONGLONG)((double)sRow->duration * baseClock);
                lastTime = curTime;
            }
            
            if(curTime - lastTime > curWait) {
                if (lpl.curIndex+1 >= lpl.song->dataSize) {
                    lpl.isPlaying = FALSE;
                    continue;
                } else {
                    lpl.curIndex++;
                }
                sRow = NULL;
            }
            SwitchToThread();
        } else {
            curWait = lastTime = 0;
            sRow = NULL;
            SleepEx(1, TRUE);
        }
        
        if (cmnd.doFlags) {
            if (cmnd.doFlags & DF_LOADSONG) {
                lpl.isPlaying = FALSE;
                lpl.curIndex = 0;
                lpl.song = cmnd.newSong;
                cmnd.doFlags &= ~DF_LOADSONG;
            }
            
            if (cmnd.doFlags & DF_PLAY) {
                if (lpl.song && lpl.curIndex < lpl.song->dataSize) {
                    lpl.isPlaying = cmnd.isPlay;
                }
                cmnd.doFlags &= ~DF_PLAY;
            }
            
            if (cmnd.doFlags & DF_EXIT) {
                lpl.isInit = FALSE;
                cmnd.doFlags &= ~DF_EXIT;
            }
        }
    }
    
    return 0;
}

LogPlayer* esslp_init() {
    DWORD threadID;
    
    if (hThread) return &lpl;
    ZeroMemory(&lpl, sizeof(lpl));
    ZeroMemory(&cmnd, sizeof(cmnd));
    hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)(playThread), NULL, 0, &threadID);
    if (!hThread) goto ERR;
    
    return &lpl;
    
    ERR:
        esslp_destroy();
        return NULL;
}


#define WAITON(MASK) while(cmnd.doFlags & MASK) SleepEx(1, TRUE)

void esslp_destroy() {
    cmnd.doFlags |= DF_EXIT;
    WaitForSingleObject(hThread, INFINITE);
}

void esslp_playCtrl(BOOL isPlay) {
    if (!lpl.song) return;
    
    cmnd.isPlay = isPlay;
    cmnd.doFlags |= DF_PLAY;
    WAITON(DF_PLAY);
}

void esslp_loadSong(Song* song) {
    cmnd.newSong = song;
    cmnd.doFlags |= DF_LOADSONG;
    WAITON(DF_LOADSONG);
}

void esslp_stop() {
    if (!lpl.song) return;
    
    cmnd.isPlay = FALSE;
    cmnd.doFlags |= DF_PLAY;
    WAITON(DF_PLAY);
    lpl.curIndex  = 0;
}
    
void esslp_seek(DWORD index) {
    /*if (!lpl.song)
    if (index > lpl.song->dataSize) {
        lpl.curIndex = lpl.song->dataSize - 1;
    } else {
        lpl.curIndex = index;
    }*/
}