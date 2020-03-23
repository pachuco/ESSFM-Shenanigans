#include <windows.h>
#include "support.h"

#if (_WIN64)
    #define INPOUT_LIB "inpout64.dll"
#else
    #define INPOUT_LIB "inpout32.dll"
#endif

HINSTANCE hInOut;

BOOL initInOut() {
    #define CHK(x) if(!(x)) return FALSE
    #define GPA(x) (x = (void*)GetProcAddress(hInOut, #x))
    #ifdef ENABLE_HARDWARE_ACCESS
        CHK(hInOut = LoadLibrary(INPOUT_LIB));
        //orig
        CHK(GPA(Out32));
        CHK(GPA(Inp32));
        //DLLPortIO
        CHK(GPA(DlPortReadPortUchar));
        CHK(GPA(DlPortWritePortUchar));
        CHK(GPA(DlPortReadPortUshort));
        CHK(GPA(DlPortWritePortUshort));
        CHK(GPA(DlPortReadPortUlong));
        CHK(GPA(DlPortWritePortUlong));
        //helper
        CHK(GPA(IsInpOutDriverOpen));
        CHK(GPA(IsXP64Bit));
        
        
        CHK(IsInpOutDriverOpen());
    #endif
    
    return TRUE;
    #undef GPA
    #undef CHK
}

//not very accurate
void QPCuWait(DWORD uSecTime) { //KeStallExecutionProcessor
    static LONGLONG freq=0;
    LONGLONG start=0, cur=0, wait=0;
    
    if (freq == 0) QueryPerformanceFrequency((PLARGE_INTEGER)&freq);
    if (freq != 0) {
        wait = ((LONGLONG)uSecTime * freq)/(LONGLONG)1000000;
        QueryPerformanceCounter((PLARGE_INTEGER)&start);
        while (cur < (start + wait)) {
            __asm__("pause");
            QueryPerformanceCounter((PLARGE_INTEGER)&cur);
        }
    } else {
        //TODO: alternate timing mechanism
    }
}