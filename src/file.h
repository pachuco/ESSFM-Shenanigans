#pragma once

#include <windows.h>
#include <stdio.h>

typedef enum {EXPTH_FNAME, EXPTH_EXTENSION} EXPTH_T;

void exPartFromPath(char* dest, char* src, int max, EXPTH_T type);

//-------------------------------------

typedef struct MemFile {
    char* data;
    int size;
} MemFile;

int getFileSize(FILE* f);
BOOL loadFileToMem(MemFile* mf, char* path);
void closeMemFile(MemFile* mf);

//-------------------------------------

#define F_ANY "Any file\0*.*\0"
#define F_LOG "DbgView log\0*.LOG\0"
#define F_RAW "RDOS raw opl\0*.RAW\0"
#define F_DRO "DosBox DRO\0*.DRO\0"
#define FILETYPES F_ANY F_LOG F_RAW F_DRO

typedef enum {
    SNG_OPLX, SNG_ESS
} SONG_TYPE;

typedef enum {
    SR_INACTIVE = 1<<0
} SONGROW_BITFIELD;

typedef struct SongRow {
    float duration;
    UCHAR port;
    UCHAR data;
    UCHAR bitfield;
} SongRow;

typedef struct Song {
    SongRow* rows;
    int type;
    //both multiple of sizeof(SongRow)
    int dataSize; 
    int allocSize;
} Song;

Song* loadSong(char* path);
Song* loadWeirdDosboxDro(char* path);
Song* loadDosboxDro(char* path);
Song* loadRdosRawOpl(char* path);
Song* loadDbgViewLog(char* path);

//-------------------------------------

typedef struct Config {
    USHORT basePort;
} Config;

BOOL loadConfig(char* path, Config* conf);