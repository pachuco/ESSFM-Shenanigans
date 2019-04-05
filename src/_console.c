#include <windows.h>

static BOOL getKeydownVK(DWORD* out) {
    DWORD iEvNum, dummy;
    INPUT_RECORD ir;
    static HANDLE hIn = NULL;
    
    if (!hIn) hIn = GetStdHandle(STD_INPUT_HANDLE);
    
    GetNumberOfConsoleInputEvents(hIn, &iEvNum);
    while (iEvNum--) {
        ReadConsoleInput(hIn, &ir, 1, &dummy);
        if (ir.EventType == KEY_EVENT && ir.Event.KeyEvent.bKeyDown) {
            if (out) *out = ir.Event.KeyEvent.wVirtualKeyCode;
            return TRUE;
        }
    }
    return FALSE;
}

static void pressAnyKey() {
    while (!getKeydownVK(NULL)) SleepEx(1,1);
}

static void clearScreen() {
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