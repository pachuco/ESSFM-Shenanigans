#ifndef ESS_FILE_H
#define ESS_FILE_H

#include <windows.h>

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

BOOL loadRdosRawOpl(Song* song, char* path);
BOOL loadDbgViewLog(Song* song, char* path);

//-------------------------------------

typedef struct Config {
    USHORT basePort;
} Config;

BOOL loadConfig(char* path, Config* conf);

#endif