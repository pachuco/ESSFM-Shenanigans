#include <windows.h>

typedef enum {
    KEY_ACTV = 1<<0,
    KEY_DOWN = 1<<1,
    KEY_HEAD = 1<<2
} KSTATE;

typedef struct ScreenBuffer {
    COORD bufSize;
    COORD wndSize;
    CHAR_INFO* data;
    BOOL needsRedraw;
} ScreenBuffer;

static HANDLE hOut = INVALID_HANDLE_VALUE;
static HANDLE hIn = INVALID_HANDLE_VALUE;
static DWORD whiteList;
static BYTE keyStates[256];

static BOOL trimWriteRegion(ScreenBuffer* sBuf, PSMALL_RECT rect) {
    if (rect->Left   > sBuf->wndSize.X) return FALSE;
    if (rect->Top    > sBuf->wndSize.Y) return FALSE;
    if (rect->Right  > sBuf->wndSize.X) rect->Right = sBuf->wndSize.X - 1;
    if (rect->Bottom > sBuf->wndSize.Y) rect->Right = sBuf->wndSize.Y - 1;
    
    return TRUE;
}

KSTATE getKeyVK(WORD* out) {
    DWORD iEvNum;
    INPUT_RECORD ir;
    KSTATE ks = 0;
    
    if (PeekConsoleInput(hIn, &ir, 1, &iEvNum) && iEvNum > 0 && ir.EventType == KEY_EVENT) {
        BOOL isKDown = ir.Event.KeyEvent.bKeyDown;
        WORD vk = ir.Event.KeyEvent.wVirtualKeyCode;
        
        if (vk > 0xFF) return 0;
        
        ReadConsoleInput(hIn, &ir, 1, &iEvNum);
        ks |= KEY_ACTV;
        ks |= !(isKDown && keyStates[vk]) ? KEY_HEAD : 0;
        ks |= isKDown ? KEY_DOWN : 0;
        
        keyStates[vk] = (BYTE)isKDown;
        if (out) *out = vk;
    }
    return ks;
}

void pressAnyKey() {
    while (!getKeyVK(NULL)) SleepEx(1,1);
}

void validateScreenBuf(ScreenBuffer* sBuf) {
    COORD wndSize;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int wipeSize;
    
    GetConsoleScreenBufferInfo(hOut, &csbi);
    wndSize.X = csbi.srWindow.Right  - csbi.srWindow.Left + 1;
    wndSize.Y = csbi.srWindow.Bottom - csbi.srWindow.Top  + 1;
    if (sBuf->wndSize.X != wndSize.X || sBuf->wndSize.Y != wndSize.Y) {
        sBuf->wndSize.X = wndSize.X;
        sBuf->wndSize.Y = wndSize.Y;
        sBuf->needsRedraw = TRUE;
    }
    if (sBuf->bufSize.X != csbi.dwSize.X || sBuf->bufSize.Y != csbi.dwSize.Y) {
        sBuf->bufSize.X = csbi.dwSize.X;
        sBuf->bufSize.Y = csbi.dwSize.Y;
        sBuf->needsRedraw = TRUE;
        
        wipeSize = sBuf->bufSize.X * sBuf->bufSize.Y * sizeof(CHAR_INFO);
        sBuf->data = !sBuf->data ? malloc(wipeSize) : realloc(sBuf->data, wipeSize);
    }
}

void consumeEvents() {
    DWORD iEvNum;
    INPUT_RECORD ir;
    
    while (PeekConsoleInput(hIn, &ir, 1, &iEvNum) && iEvNum > 0) {
        if (ir.EventType & (MENU_EVENT | FOCUS_EVENT | WINDOW_BUFFER_SIZE_EVENT)) {
            //ignore these
            ReadConsoleInput(hIn, &ir, 1, &iEvNum);
        } else if (!(ir.EventType & whiteList)) {
            ReadConsoleInput(hIn, &ir, 1, &iEvNum);
        } else {
            return;
        }
    }
}

void clearScreen() {
   COORD coordScreen = {0, 0};
   DWORD cCharsWritten;
   CONSOLE_SCREEN_BUFFER_INFO csbi; 
   DWORD dwConSize;
   
   if (!GetConsoleScreenBufferInfo(hOut, &csbi)) return;
   dwConSize = csbi.dwSize.X * csbi.dwSize.Y;
   if (!FillConsoleOutputCharacter(hOut, (CHAR) ' ', dwConSize, coordScreen, &cCharsWritten)) return;
   if (!FillConsoleOutputAttribute(hOut, csbi.wAttributes, dwConSize, coordScreen, &cCharsWritten)) return;
   SetConsoleCursorPosition(hOut, coordScreen);
}

COORD paintAttributeRect(ScreenBuffer* sBuf, SMALL_RECT rect, WORD attributes) {;
    if (!trimWriteRegion(sBuf, &rect)) return (COORD){-1, -1};
    
    for (int i=rect.Top; i < rect.Bottom; i++) {
        for (int j=rect.Left; j < rect.Right; j++) {
            sBuf->data[i*sBuf->bufSize.X + j].Attributes = attributes;
        }
    }
    
    SetConsoleCursorPosition(hOut, (COORD){0,0});
    return (COORD){rect.Right, rect.Bottom};
}

COORD writeTextLine(ScreenBuffer* sBuf, COORD cord, int maxY, char* format, ...) {
    SMALL_RECT wReg;
    char tmp[256];
    
    va_list args;
    va_start(args, format);
    vsprintf(tmp,format, args);
    va_end(args);
    
    wReg = (SMALL_RECT){cord.X, cord.Y, cord.X+lstrlenA(tmp), cord.Y};
    if (!trimWriteRegion(sBuf, &wReg)) return (COORD){-1, -1};
    if (wReg.Right > maxY) wReg.Right = maxY;
    
    for (int j=wReg.Left; j < wReg.Right; j++) {
        sBuf->data[cord.Y * sBuf->bufSize.X + j].Char.AsciiChar = tmp[j - wReg.Left];
    }
    
    return (COORD){cord.Y, wReg.Right};
}

void updateRegion(ScreenBuffer* sBuf, SMALL_RECT rect) {
    WriteConsoleOutputA(hOut, sBuf->data, sBuf->bufSize, (COORD){0,0}, &rect);
    SetConsoleCursorPosition(hOut, (COORD){0,0});
}

void initTUIConsole(ScreenBuffer* sBuf, DWORD wl) {
    whiteList = wl;
    hIn  = GetStdHandle(STD_INPUT_HANDLE);
    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    memset(&keyStates, 0, sizeof(keyStates));
    
    sBuf->wndSize.X = sBuf->wndSize.Y = 0;
    sBuf->bufSize.X = sBuf->bufSize.Y = 0;
}