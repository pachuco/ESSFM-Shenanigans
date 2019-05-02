#ifndef ESS_SUPPORT_H
#define ESS_SUPPORT_H

#include <windows.h>

//-------------------------------------inpout

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

BOOL initInOut();
void QPCuWait(DWORD uSecTime);

//-------------------------------------console

typedef enum {
    KEY_ACTV = 1<<0,
    KEY_DOWN = 1<<1,
    KEY_HEAD = 1<<2
} KSTATE;

typedef struct ScreenBuffer {
    COORD bufSize;
    COORD wndSize;
    CHAR_INFO* data;
} ScreenBuffer;

KSTATE getKeyVK(WORD* out);
void   pressAnyKey();
BOOL   validateScreenBuf(ScreenBuffer* sBuf);
void   consumeEvents();
void   clearScreen();
COORD  paintAttributeRect(ScreenBuffer* sBuf, SMALL_RECT rect, WORD attributes);
COORD  writeTextLine(ScreenBuffer* sBuf, COORD cord, int maxY, char* format, ...);
void   updateRegion(ScreenBuffer* sBuf, SMALL_RECT rect);
void   initTUIConsole(ScreenBuffer* sBuf, DWORD wl);

#endif