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

BOOL songAlloc(Song* song, int estimate) {
    if (!song->rows) {
        song->rows = malloc(estimate * sizeof(SongRow));
    } else {
        song->rows = realloc(song->rows, estimate * sizeof(SongRow));
    }
    if (!song->rows) return FALSE;
    
    song->dataSize = 0;
    song->allocSize = estimate;
    
    return TRUE;
}

BOOL songReallocIfNeeded(Song* song, int index, int amount) {
    if (index >= song->allocSize) {
        song->allocSize += amount;
        song->rows = realloc(song->rows, song->allocSize * sizeof(SongRow));
        if (!song->rows) return FALSE;
    }
    return TRUE;
}

BOOL loadRdosRawOpl(Song* song, char* path) {
    FILE* fin;
    int fileSize;
    int index = 0;
    int crudeEstimate;
    BYTE raw_magic[8];
    BYTE raw_delayNum = 0;
    BOOL raw_isChipHigh = FALSE;
    USHORT raw_clock;
    
    if (!(fin = fopen(path, "rb"))) goto _ERR;
    fileSize = getFileSize(fin);
    
    fread((void*)&raw_magic, 8, 1, fin);
    if (memcmp(raw_magic, "RAWADATA", 8)) goto _ERR;
    fread((void*)&raw_clock, 2, 1, fin);
    
    crudeEstimate = (fileSize - ftell(fin));
    if (!songAlloc(song, crudeEstimate)) goto _ERR;
    
    while(!feof(fin)) {
        BYTE val, reg;
        
        fread((void*)&val, 1, 1, fin);
        fread((void*)&reg, 1, 1, fin);
        
        if (val == 0xFF && reg == 0xFF) break; //EOF
        switch(reg) {
            case 0x00: //delay
                raw_delayNum += val;
                break;
            case 0x02: //control
                switch (val) {
                    case 0x00: //clock change
                        fread((void*)&raw_clock, 2, 1, fin);
                        break;
                    case 0x01: //write low
                        raw_isChipHigh = FALSE;
                        break;
                    case 0x02: //write high
                        raw_isChipHigh = TRUE;
                        break;
                }
                break;
            default: //normal register
                if (index > 0) {
                    song->rows[index-1].duration = (float)((raw_delayNum) * raw_clock) / 1193180.0;
                }
                raw_delayNum = 0;
                
                if (!songReallocIfNeeded(song, index, 64)) goto _ERR;
                song->rows[index].port = (raw_isChipHigh ? 2 : 0);
                song->rows[index].data = reg;
                song->rows[index++].duration = 0.0;
                
                if (!songReallocIfNeeded(song, index, 64)) goto _ERR;
                song->rows[index].port = (raw_isChipHigh ? 3 : 1);
                song->rows[index].data = val;
                song->rows[index++].duration = 0.0;
                song->dataSize = index;
                break;
        }
    }
    song->type = SNG_OPLX;
    fclose(fin);
    
    return TRUE;
    _ERR:
        if (fin) fclose(fin);
        return FALSE;
}

BOOL loadDbgViewLog(Song* song, char* path) {
    #define LINEMAX 1024
    FILE* fin;
    int fileSize;
    int index = 0;
    char lineBuf[LINEMAX]; char split[]="\t ";
    int crudeEstimate; 
    
    if (!(fin = fopen(path, "r"))) goto _ERR;
    fileSize = getFileSize(fin);
    crudeEstimate = fileSize/44; //44 is size of smallest log entry
    if (!songAlloc(song, crudeEstimate)) goto _ERR;
    
    while(fgets(lineBuf, LINEMAX, fin)) {
        int lineLen; char* pch;
        SongRow sRow;
        
        //commented out line
        //DbgView treats these as invalid, but we don't!
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
        if (index > 0) song->rows[index-1].duration = sRow.duration - song->rows[index-1].duration;
        
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
        
        if (!songReallocIfNeeded(song, index, 512)) goto _ERR;
        
        song->rows[index].duration = sRow.duration;
        song->rows[index].port     = sRow.port;
        song->rows[index].data     = sRow.data;
        song->dataSize = ++index;
    }
    song->rows[song->dataSize-1].duration = 0.0;
    song->type = SNG_ESS;
    fclose(fin);
    
    return TRUE;
    _ERR:
        if (fin) fclose(fin);
        return FALSE;
    #undef LINEMAX
}

//-------------------------------------

typedef struct Config {
    USHORT basePort;
} Config;

BOOL loadConfig(char* path, Config* conf) {
    return TRUE;
}