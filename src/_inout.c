#include <windows.h>

#if (_WIN64)
    #define INPOUT_LIB "inpout64.dll"
#else
    #define INPOUT_LIB "inpout32.dll"
#endif

void    (__stdcall *Out32)(short PortAddress, short data);
short   (__stdcall *Inp32)(short PortAddress);
UCHAR   (__stdcall *DlPortReadPortUchar) (USHORT port);
void    (__stdcall *DlPortWritePortUchar)(USHORT port, UCHAR Value);
USHORT  (__stdcall *DlPortReadPortUshort) (USHORT port);
void    (__stdcall *DlPortWritePortUshort)(USHORT port, USHORT Value);
ULONG	(__stdcall *DlPortReadPortUlong) (ULONG port);
void	(__stdcall *DlPortWritePortUlong)(ULONG port, ULONG Value);
BOOL    (__stdcall *IsInpOutDriverOpen)(void);
BOOL    (__stdcall *IsXP64Bit)(void);

HINSTANCE hInOut;

BOOL initInOut() {
    #define CHK(x) if(!(x)) return FALSE
    #define GPA(x) (x = (void*)GetProcAddress(hInOut, #x))
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