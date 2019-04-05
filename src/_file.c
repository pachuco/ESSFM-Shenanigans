#include <windows.h>
#include <stdio.h>

typedef struct MemFile {
    char* data;
    int size;
} MemFile;

BOOL loadFileToMem(char* path, MemFile* mf) {
    FILE* fin;
    
    if (!(fin = fopen(path, "rb"))) return FALSE;
    
    fseek(fin, 0, SEEK_END);
    mf->size = ftell(fin);
    fseek(fin, 0, SEEK_SET);
    
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