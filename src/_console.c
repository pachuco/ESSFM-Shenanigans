#include <windows.h>

typedef enum {
    KEY_NONE = 0,
    KEY_DOWN = 1<<0,
    KEY_UP   = 1<<1,
    KEY_HEAD = 1<<2
} KSTATE;

typedef struct ScreenBuffer {
    COORD size;
    CHAR_INFO* data;
} ScreenBuffer;

static HANDLE hOut;
static HANDLE hIn;
static COORD originalSize;
static DWORD whiteList;
static BYTE keyStates[256];

void getScreenSize(PCOORD pCord) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    
    if (!GetConsoleScreenBufferInfo(hOut, &csbi)) {
        pCord->X = pCord->Y = 0;
    } else {
        pCord->X = csbi.srWindow.Right  - csbi.srWindow.Left + 1;
        pCord->Y = csbi.srWindow.Bottom - csbi.srWindow.Top  + 1;
    }
}

BOOL resizeScreenBuf(PCOORD pSize) {
    if (!pSize) pSize = &originalSize;
    return SetConsoleScreenBufferSize(hOut, *pSize);
}

BOOL validateScreenBuf(ScreenBuffer* buf) {
    COORD cord;
    
    getScreenSize(&cord);
    if (buf->size.X != cord.X || buf->size.Y != cord.Y) {
        int wipeSize = cord.X * cord.Y * sizeof(CHAR_INFO);
        
        buf->size.X = cord.X;
        buf->size.Y = cord.Y;
        if (buf->data) {
            buf->data = realloc(buf->data, wipeSize);
        } else {
            buf->data = malloc(wipeSize);
        }
        return TRUE;
    }
    return FALSE;
}

void consumeEvents() {
    DWORD iEvNum, dummy;
    INPUT_RECORD ir;
    
    while (PeekConsoleInput(hIn, &ir, 1, &iEvNum) && iEvNum > 0) {
        if (ir.EventType & (MENU_EVENT | FOCUS_EVENT)) {
            //ignore these
            ReadConsoleInput(hIn, &ir, 1, &iEvNum);
        } else if (ir.EventType & WINDOW_BUFFER_SIZE_EVENT) {
            ReadConsoleInput(hIn, &ir, 1, &iEvNum);
        } else if (!(ir.EventType & whiteList)) {
            ReadConsoleInput(hIn, &ir, 1, &iEvNum);
        } else {
            return;
        }
    }
}

KSTATE getKeyVK(WORD* out) {
    DWORD iEvNum, dummy;
    INPUT_RECORD ir;
    KSTATE ks = KEY_NONE;
    
    consumeEvents();
    if (PeekConsoleInput(hIn, &ir, 1, &iEvNum) && iEvNum > 0 && ir.EventType == KEY_EVENT) {
        BOOL isKDown = ir.Event.KeyEvent.bKeyDown;
        WORD vk = ir.Event.KeyEvent.wVirtualKeyCode;
        
        if (vk > 0xFF) return KEY_NONE;
        
        ReadConsoleInput(hIn, &ir, 1, &iEvNum);
        ks |= !(isKDown && keyStates[vk]) ? KEY_HEAD : 0;
        ks |= isKDown ? KEY_DOWN : KEY_UP;
        
        keyStates[vk] = (BYTE)isKDown;
        if (out) *out = vk;
    }
    return ks;
}

void pressAnyKey() {
    while (!getKeyVK(NULL)) SleepEx(1,1);
}

void clearScreen() {
   COORD coordScreen = {0, 0};
   DWORD cCharsWritten;
   CONSOLE_SCREEN_BUFFER_INFO csbi; 
   DWORD dwConSize;
   
   if (!GetConsoleScreenBufferInfo(hOut, &csbi)) return;
   dwConSize = csbi.dwSize.X * csbi.dwSize.Y;
   if (!FillConsoleOutputCharacter(hOut, (TCHAR) ' ', dwConSize, coordScreen, &cCharsWritten)) return;
   if (!GetConsoleScreenBufferInfo(hOut, &csbi)) return;
   if (!FillConsoleOutputAttribute(hOut, csbi.wAttributes, dwConSize, coordScreen, &cCharsWritten)) return;
   SetConsoleCursorPosition(hOut, coordScreen);
}

void initTUIConsole(DWORD wl) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    
    whiteList = wl;
    hIn  = GetStdHandle(STD_INPUT_HANDLE);
    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    memset(&keyStates, 0, sizeof(keyStates));
    
    GetConsoleScreenBufferInfo(hOut, &csbi);
    originalSize.X = csbi.dwSize.X;
    originalSize.Y = csbi.dwSize.Y;
    //SetConsoleMode(hIn, ENABLE_WINDOW_INPUT);
    //SetConsoleMode(hOut, ); //ENABLE_EXTENDED_FLAGS
}