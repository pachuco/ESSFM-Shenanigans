#include <windows.h>
#include <stdio.h>

//extracts filename from a path. Length includes \0
typedef enum {EXPTH_FNAME, EXPTH_EXTENSION} EXPTH_T;
static void exPartFromPath(char* dest, char* src, int max, EXPTH_T type) {
    int i   = 0;
    int len = strlen(src);
    char* p = src+len;
    
    if (!len || !max) return;
    if (max > len) max = len;
    switch (type) {
        case EXPTH_FNAME:       while (i<max && *p!='\\') {;p--;i++;} break;
        case EXPTH_EXTENSION:   while (i<max && *p!='.' ) {;p--;i++;} break;
    }
    memcpy(dest, p+1, i);
}

//-------------------------------------

typedef struct MemFile {
    char* data;
    int size;
} MemFile;

int getFileSize(FILE* f) {
    int origPos = ftell(f);
    int size;
    
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, origPos, SEEK_SET);
    return size;
}

BOOL loadFileToMem(MemFile* mf, char* path) {
    FILE* fin;
    
    if (!(fin = fopen(path, "rb"))) return FALSE;
    mf->size = getFileSize(fin);
    
    if (!(mf->data = malloc(mf->size))) {
        mf->size = 0;
        fclose(fin);
    }
    
    fread(mf->data, mf->size, 1, fin);
    fclose(fin);
    
    return TRUE;
}

void closeMemFile(MemFile* mf) {
    if (mf->data) free(mf->data);
    mf->data = NULL;
    mf->size = 0;
}

//-------------------------------------

typedef struct SongRow {
    float duration;
    UCHAR port;
    UCHAR data;
} SongRow;

typedef struct Song {
    SongRow* data;
    //both multiple of sizeof(SongRow)
    int dataSize; 
    int allocSize;
} Song;

BOOL allocLog(Song* song, int size) {
    void* ptr;
    
    if (!song->data) {
        ptr = malloc(size * sizeof(SongRow));
    } else {
        ptr = realloc(song->data, size * sizeof(SongRow));
    }
    if (!ptr) return FALSE;
    song->data = ptr;
    song->dataSize = 0;
    song->allocSize = size;
    
    return TRUE;
}

BOOL loadLog(Song* song, char* path) {
    #define LINEMAX 1024
    FILE* fin;
    int fileSize; int index;
    char lineBuf[LINEMAX]; char split[]="\t ";
    SongRow* prevLr = NULL;
    
    
    if (!(fin = fopen(path, "r"))) return FALSE;
    fileSize = getFileSize(fin);
    allocLog(song, fileSize/44); //44 is size of smallest log entry
    
    index = 0;
    while(fgets(lineBuf, LINEMAX, fin)) {
        int lineLen; char* pch;
        SongRow sRow;
        
        //commented out line
        if (lineBuf[0] == '#') continue;
        
        lineLen = strlen(lineBuf);
        if (lineLen == LINEMAX-1) {
            //I think we have a problem
        }
        //STR: index
        if (!(pch = strtok(lineBuf, split))) continue;
        //ignored
        
        //DAT: timestamp
        if (!(pch = strtok(NULL, split))) continue;
        sRow.duration = strtof(pch, NULL);
        if (index > 0) prevLr->duration = sRow.duration - prevLr->duration;
        
        //STR: "__FAKE_WRITE_PORT_UCHAR:"
        if (!(pch = strtok(NULL, split))) continue;
        if (strcmp(pch, "__FAKE_WRITE_PORT_UCHAR:")) continue;
        
        //STR: "port"
        if (!(pch = strtok(NULL, split))) continue;
        if (strcmp(pch, "port")) continue;
        
        //DAT: port
        if (!(pch = strtok(NULL, split))) continue;
        sRow.port  = strtol(pch, NULL, 0);
        sRow.port &= 0x0F;
        
        //STR: "value"
        if (!(pch = strtok(NULL, split))) continue;
        if (strcmp(pch, "value")) continue;
        
        //DAT: data
        if (!(pch = strtok(NULL, split))) continue;
        sRow.data = strtol(pch, NULL, 0);
        
        if (index >= song->allocSize) allocLog(song, song->allocSize + 512);
        prevLr = &song->data[index];
        song->data[index].duration = sRow.duration;
        song->data[index].port     = sRow.port;
        song->data[index].data     = sRow.data;
        song->dataSize = ++index;
    }
    song->data[song->dataSize-1].duration = 0.0;
    fclose(fin);
    
    return TRUE;
    #undef LINEMAX
}

//-------------------------------------

typedef struct Config {
    USHORT basePort;
} Config;

BOOL loadConfig(char* path, Config* conf) {
    return TRUE;
}