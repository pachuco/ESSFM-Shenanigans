#include <windows.h>
#include <stdio.h>

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

typedef struct LogRow {
    float duration;
    UCHAR port;
    UCHAR data;
} LogRow;

typedef struct Log {
    LogRow* data;
    //both multiple of sizeof(LogRow)
    int dataSize; 
    int allocSize;
} Log;

BOOL allocLog(Log* log, int size) {
    void* ptr;
    
    if (!log->data || log->allocSize == 0) {
        ptr = malloc(size * sizeof(LogRow));
    } else {
        ptr = realloc(log->data, size * sizeof(LogRow));
    }
    if (!ptr) return FALSE;
    log->data = ptr;
    log->dataSize = 0;
    log->allocSize = size;
    
    return TRUE;
}

BOOL loadLog(Log* log, char* path) {
    #define LINEMAX 1024
    FILE* fin;
    int fileSize; int index;
    char lineBuf[LINEMAX]; char split[]="\t ";
    LogRow* prevLr = NULL;
    
    
    if (!(fin = fopen(path, "r"))) return FALSE;
    fileSize = getFileSize(fin);
    allocLog(log, fileSize/44); //44 is size of smallest log entry
    
    index = 0;
    while(fgets(lineBuf, LINEMAX, fin)) {
        int lineLen; char* pch;
        LogRow lr;
        
        lineLen = strlen(lineBuf);
        if (lineLen == LINEMAX-1) {
            //I think we have a problem
        }
        //STR: index
        if (!(pch = strtok(lineBuf, split))) continue;
        //ignored
        
        //DAT: timestamp
        if (!(pch = strtok(NULL, split))) continue;
        lr.duration = strtof(pch, NULL);
        if (index > 0) prevLr->duration = lr.duration - prevLr->duration;
        
        //STR: "__FAKE_WRITE_PORT_UCHAR:"
        if (!(pch = strtok(NULL, split))) continue;
        if (strcmp(pch, "__FAKE_WRITE_PORT_UCHAR:")) continue;
        
        //STR: "port"
        if (!(pch = strtok(NULL, split))) continue;
        if (strcmp(pch, "port")) continue;
        
        //DAT: port
        if (!(pch = strtok(NULL, split))) continue;
        lr.port  = strtol(pch, NULL, 0);
        lr.port &= 0x0F;
        
        //STR: "value"
        if (!(pch = strtok(NULL, split))) continue;
        if (strcmp(pch, "value")) continue;
        
        //DAT: data
        if (!(pch = strtok(NULL, split))) continue;
        lr.data = strtol(pch, NULL, 0);
        
        if (index >= log->allocSize) allocLog(log, log->allocSize + 512);
        prevLr = &log->data[index];
        log->data[index].duration = lr.duration;
        log->data[index].port     = lr.port;
        log->data[index].data     = lr.data;
        log->dataSize = ++index;
    }
    log->data[log->dataSize-1].duration = 0.0;
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