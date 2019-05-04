#include <windows.h>
#include <stdio.h>
#include "support.h"
#include "file.h"

//extracts filename from a path. Length includes \0
void exPartFromPath(char* dest, char* src, int max, EXPTH_T type) {
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

static BOOL songAlloc(Song* song, int estimate) {
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

static BOOL songReallocIfNeeded(Song* song, int index, int amount) {
    if (index >= song->allocSize) {
        song->allocSize += amount;
        song->rows = realloc(song->rows, song->allocSize * sizeof(SongRow));
        if (!song->rows) return FALSE;
    }
    return TRUE;
}

BOOL loadDosboxDro(Song* song, char* path) {
    FILE* fin;
    int fileSize;
    int crudeEstimate;
    int index = 0;
    int delayNum = 0;
    BOOL isChipHigh = FALSE;
    BYTE dro_magic[8];
    DWORD dro_ver;
    
    if (!(fin = fopen(path, "rb"))) goto _ERR;
    fileSize = getFileSize(fin);
    
    fread((void*)&dro_magic, 8, 1, fin);
    if (memcmp(dro_magic, "DBRAWOPL", 8)) goto _ERR;
    fread((void*)&dro_ver, 4, 1, fin);
    
    switch (dro_ver) {
        case 0x00000001:
        case 0x00010000: {
            DWORD dro_lengthMS;
            DWORD dro_lengthBytes;
            DWORD dro_hardwareType;
            
            fread((void*)&dro_lengthMS, 4, 1, fin);
            fread((void*)&dro_lengthBytes, 4, 1, fin);
            fread((void*)&dro_hardwareType, 4, 1, fin);
            
            if (dro_hardwareType & 0xFFFFFF00) fseek(fin, -3, SEEK_CUR);
            dro_hardwareType &= 0xFF;
            
            crudeEstimate = (fileSize - ftell(fin)) / 2;
            if (!songAlloc(song, crudeEstimate)) goto _ERR;
            
            for (int i=0; i < dro_lengthBytes; i++) {
                BYTE reg, val;
                BYTE delShort; USHORT delLong;
                BOOL doWrite = FALSE;
                BYTE code;
                
                fread((void*)&code, 1, 1, fin);
                
                switch(code) {
                    case 0x00:  //delay short
                        fread((void*)&delShort, 1, 1, fin);
                        delayNum += delShort+1;
                        i += 1;
                        break;
                    case 0x01:  //delay long
                        fread((void*)&delLong, 2, 1, fin);
                        delayNum += delLong+1;
                        i += 2;
                        break;
                    case 0x02:  //switch low
                        isChipHigh = FALSE;
                        break;
                    case 0x03:  //switch high
                        isChipHigh = TRUE;
                        break;
                    case 0x04:  //escape
                        doWrite = TRUE;
                        fread((void*)&reg, 1, 1, fin);
                        fread((void*)&val, 1, 1, fin);
                        i += 2;
                        break;
                    default:    //normal register
                        doWrite = TRUE;
                        reg = code;
                        fread((void*)&val, 1, 1, fin);
                        i += 2;
                        break;
                }
                if (ferror(fin)) break;
                
                if (doWrite) {
                    if (index > 0) {
                        song->rows[index-1].duration = (float)delayNum / 1000;
                    }
                    delayNum = 0;
                    
                    //one extra for two SongRows
                    if (!songReallocIfNeeded(song, index + 1, 64)) goto _ERR;
                    
                    song->rows[index].port = (isChipHigh ? 2 : 0);
                    song->rows[index].data = reg;
                    song->rows[index++].duration = 0.0;
                    song->rows[index].port = (isChipHigh ? 3 : 1);
                    song->rows[index].data = val;
                    song->rows[index++].duration = 0.0;
                    song->dataSize = index;
                }
            }
            break;
        }
        case 0x00000002: {
            DWORD dro_lengthPairs;
            DWORD dro_lengthMS;
            BYTE  dro_hardwareType;
            BYTE  dro_format;
            BYTE  dro_compression;
            BYTE  dro_shortDelayCode;
            BYTE  dro_longDelayCode;
            BYTE  dro_codemapLength;
            BYTE  dro_codemap[128];
            
            fread((void*)&dro_lengthPairs, 4, 1, fin);
            fread((void*)&dro_lengthMS, 4, 1, fin);
            fread((void*)&dro_hardwareType, 1, 1, fin);
            fread((void*)&dro_format, 1, 1, fin);
            fread((void*)&dro_compression, 1, 1, fin);
            fread((void*)&dro_shortDelayCode, 1, 1, fin);
            fread((void*)&dro_longDelayCode, 1, 1, fin);
            fread((void*)&dro_codemapLength, 1, 1, fin);
            fread((void*)dro_codemap, 1, dro_codemapLength, fin);
            
            if (ferror(fin)) goto _ERR;
            if (dro_format != 0) goto _ERR;
            if (dro_compression != 0) goto _ERR;
            
            crudeEstimate = dro_lengthPairs;
            if (!songAlloc(song, crudeEstimate)) goto _ERR;
            
            for (int i=0; i < dro_lengthPairs; i++) {
                BYTE reg, val;
                
                fread((void*)&reg, 1, 1, fin);
                fread((void*)&val, 1, 1, fin);
                
                if (ferror(fin)) break;
                if        (reg == dro_shortDelayCode) {
                    delayNum +=  val+1;
                } else if (reg == dro_longDelayCode) {
                    delayNum += (val+1)<<8;
                } else { //normal register
                    int isHigh = reg&0x80;
                    if (index > 0) {
                        song->rows[index-1].duration = (float)delayNum / 1000;
                    }
                    delayNum = 0;
                    
                    //one extra for two SongRows
                    if (!songReallocIfNeeded(song, index + 1, 64)) goto _ERR;
                    
                    song->rows[index].port = (isHigh ? 2 : 0);
                    song->rows[index].data = dro_codemap[reg & 0x7F];
                    song->rows[index++].duration = 0.0;
                    song->rows[index].port = (isHigh ? 3 : 1);
                    song->rows[index].data = val;
                    song->rows[index++].duration = 0.0;
                    song->dataSize = index;
                }
            }
            break;
        }
        default:
            goto _ERR;
    }
    
    song->type = SNG_OPLX;
    fclose(fin);
    
    return TRUE;
    _ERR:
        if (fin) fclose(fin);
        return FALSE;
}

BOOL loadRdosRawOpl(Song* song, char* path) {
    FILE* fin;
    int fileSize;
    int crudeEstimate;
    int index = 0;
    int delayNum = 0;
    BYTE raw_magic[8];
    BOOL isChipHigh = FALSE;
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
        
        if (!raw_clock) raw_clock = 0xFFFF;
        fread((void*)&val, 1, 1, fin);
        fread((void*)&reg, 1, 1, fin);
        
        if (val == 0xFF && reg == 0xFF) break; //EOF
        if (ferror(fin)) break;
        
        switch(reg) {
            case 0x00: //delay
                delayNum += val;
                break;
            case 0x02: //control
                switch (val) {
                    case 0x00: //clock change
                        fread((void*)&raw_clock, 2, 1, fin);
                        break;
                    case 0x01: //write low
                        isChipHigh = FALSE;
                        break;
                    case 0x02: //write high
                        isChipHigh = TRUE;
                        break;
                }
                break;
            default: //normal register
                if (index > 0) {
                    song->rows[index-1].duration = (float)(delayNum * raw_clock) / 1193180.0;
                }
                delayNum = 0;
                
                //one extra for two SongRows
                if (!songReallocIfNeeded(song, index + 1, 64)) goto _ERR;
                
                song->rows[index].port = (isChipHigh ? 2 : 0);
                song->rows[index].data = reg;
                song->rows[index++].duration = 0.0;
                song->rows[index].port = (isChipHigh ? 3 : 1);
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
    #define DELSENS 0.001 //one millisecond
    FILE* fin;
    int fileSize;
    int index = 0;
    double delay = 0.0;
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
        
        //DAT: timestamp -> duration
        //one step behind the read
        if (!(pch = strtok(NULL, split))) continue;
        sRow.duration = strtof(pch, NULL);
        if (index > 0) {
            SongRow* prev = &song->rows[index-1];
            float diff = sRow.duration - prev->duration;
            
            if (diff < DELSENS) {
                prev->duration = 0.0;
                delay += diff;
                //Beep(1000, 20);
            } else {
                prev->duration = diff + delay;
                delay = 0.0;
            }
        }
        
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
        
        song->rows[index].duration = sRow.duration; //read current timestamp
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
    #undef DELSENS
}

//-------------------------------------

BOOL loadConfig(char* path, Config* conf) {
    return TRUE;
}