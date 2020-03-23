#pragma once

#include <windows.h>
#include "file.h"

typedef struct {
    Song* song;
    BOOL  isInit;
    BOOL  isPlaying;
    DWORD curIndex;
} LogPlayer;

LogPlayer* esslp_init();
void       esslp_destroy();
void       esslp_loadSong(Song* song);
void       esslp_playCtrl(BOOL isPlay);
void       esslp_stop();
void       esslp_seek(DWORD index);