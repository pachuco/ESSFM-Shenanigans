#include <windows.h>

typedef enum {NONE, KEY_DOWN, KEY_UP} KSTATE;

KSTATE getKeyVK(WORD* out, BOOL isTypomatic) {
    DWORD iEvNum, dummy;
    INPUT_RECORD ir;
    static HANDLE hIn = NULL;
    
    if (!hIn) hIn = GetStdHandle(STD_INPUT_HANDLE);
    
    GetNumberOfConsoleInputEvents(hIn, &iEvNum);
    while (iEvNum--) {
        ReadConsoleInput(hIn, &ir, 1, &dummy);
        if (ir.EventType == KEY_EVENT && (isTypomatic || ir.Event.KeyEvent.wRepeatCount == 0)) {
            if (out) *out = ir.Event.KeyEvent.wVirtualKeyCode;
            return ir.Event.KeyEvent.bKeyDown ? KEY_DOWN : KEY_UP;
        }
    }
    return NONE;
}

void pressAnyKey() {
    while (!getKeyVK(NULL, TRUE)) SleepEx(1,1);
}

void clearScreen() {
   COORD coordScreen = {0, 0};
   DWORD cCharsWritten;
   CONSOLE_SCREEN_BUFFER_INFO csbi; 
   DWORD dwConSize;
   static HANDLE hOut = NULL;
   
   if (!hOut) hOut = GetStdHandle(STD_OUTPUT_HANDLE);
   if (!GetConsoleScreenBufferInfo(hOut, &csbi)) return;
   dwConSize = csbi.dwSize.X * csbi.dwSize.Y;
   if (!FillConsoleOutputCharacter(hOut, (TCHAR) ' ', dwConSize, coordScreen, &cCharsWritten)) return;
   if (!GetConsoleScreenBufferInfo(hOut, &csbi)) return;
   if (!FillConsoleOutputAttribute(hOut, csbi.wAttributes, dwConSize, coordScreen, &cCharsWritten)) return;
   SetConsoleCursorPosition(hOut, coordScreen);
}