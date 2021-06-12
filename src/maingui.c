#include <windows.h>
#include <commctrl.h>

#include "guiresources.h"
#include "support.h"
#include "logplayer.h"
#include "file.h"
#include "_fm.c"

#define MAXBUF 2048
void odsprintf(const CHAR* fmt, ...) {
    va_list args;
    char buf[MAXBUF];
    
    va_start(args, fmt);
    vsnprintf(buf, MAXBUF, fmt, args);
    va_end(args);
    OutputDebugStringA(buf);
}

char wndClassName[] = "cls_logThinghieWindow";
static HWND hMain;
static HFONT hFont;
static HWND hList;;
static HACCEL hAccel;
static char filePath[2048];
static char fileName[256];
static DWORD numRows;

static LogPlayer* plpl;
static Song* song;

#define TIMER_PLAY 1
#define DELAY_ANIM 100 //ms
#define MAX_LVROWS 512

enum {
    RANGE_ACC = 10000,
    ACC_OPENFILE,
    ACC_PLAYTOGGLE
};

enum {
    COL_INDICATOR,
    COL_INDEX,
    COL_DURAT,
    COL_PORT,
    COL_DATA,
    COL_DESCR,
    MAX_COL
};



static ACCEL tabAccel[] = {
    {FVIRTKEY, VK_F3,       ACC_OPENFILE},
    {FVIRTKEY, VK_SPACE,    ACC_PLAYTOGGLE}
};

static void redrawList() {
    char tBuf[256];
    
    if (!hList) return;
    
    odsprintf("%d", numRows);
    SetScrollPos(hMain, SB_VERT, plpl->curIndex, TRUE);
    InvalidateRect(hList, NULL, FALSE);
    //ListView_RedrawItems(hList, 0, numRows);
    UpdateWindow(hList);
}

static void openFileDialog() {
    OPENFILENAMEA ofna = {};
    
    ofna.lStructSize        = sizeof(OPENFILENAMEA);
    ofna.hwndOwner          = GetConsoleWindow();
    ofna.lpstrFilter        = FILETYPES "\0";
    ofna.nFilterIndex       = 1;
    ofna.lpstrFile          = filePath;
    ofna.nMaxFile           = 1024;
    ofna.lpstrFileTitle     = fileName;
    ofna.Flags              = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    
    if (GetOpenFileNameA(&ofna)) {
        Song* newSong;
        if (newSong = loadSong(filePath)) {
            exPartFromPath(fileName, filePath, sizeof(fileName), EXPTH_FNAME);
            SetWindowTextA(hMain, fileName);
            
            free(song);
            song = newSong;
            FM_stopSynth();
            FM_startSynth();
            esslp_loadSong(song);
            SetScrollRange(hList, SB_VERT, 0, song->dataSize, TRUE);
            redrawList();
        }
    }
}



static LRESULT proc_listView(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    NMLVDISPINFOA* plvdi;
    switch (msg) {
        case WM_SIZE:
            return 0;
        case WM_VSCROLL:
            if (plpl->isPlaying) return 0;
            plpl->curIndex = GetScrollPos(hList, SB_VERT);
            SetScrollRange(hList, SB_VERT, 0, song ? song->dataSize : 0, TRUE);
            return 0;
        case WM_HSCROLL:
            return 0;
    }
    return CallWindowProc((void*)GetWindowLong(hwnd, GWL_USERDATA), hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK proc_mainWnd(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	LVCOLUMN col = {};
	LVITEMA item = {};

	switch (msg) {
        case WM_PAINT:
            //redrawList();
            break;
        case WM_DESTROY:
            esslp_destroy();
            FM_stopSynth();
            shutdownInOut();
            if (song) free(song);
            PostQuitMessage(0);
            return 0;
        case WM_CREATE: {
            InitCommonControls();
            hList = CreateWindowExA(0, WC_LISTVIEW, NULL, WS_CHILD|WS_VISIBLE|LVS_REPORT|LVS_OWNERDATA,
              0, 0, 10, 10, hwnd, (HMENU)1, ((LPCREATESTRUCTA)lParam)->hInstance, NULL);
            if (!hList) PostQuitMessage(1);
            //subscroll
            //SetWindowLongA(hList, GWL_USERDATA, SetWindowLongA(hList, GWL_WNDPROC, (LONG)proc_listView));
            
            SendMessage(hList, WM_SETFONT, hFont, TRUE);
            
            col.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
            col.fmt  = LVCFMT_LEFT;
            
            col.pszText = " ";
            col.iSubItem = COL_INDICATOR;
            col.cx = 20;
            ListView_InsertColumn(hList, COL_INDICATOR, &col);
            col.pszText = TXT_LVINDEX;
            col.iSubItem = COL_INDEX;
            col.cx = 100;
            ListView_InsertColumn(hList, COL_INDEX, &col);
            col.pszText = TXT_LVDURAT;
            col.iSubItem = COL_DURAT;
            col.cx = 100;
            ListView_InsertColumn(hList, COL_DURAT, &col);
            col.pszText = TXT_LVPORT;
            col.iSubItem = COL_PORT;
            col.cx = 35;
            ListView_InsertColumn(hList, COL_PORT, &col);
            col.pszText = TXT_LVDATA;
            col.iSubItem = COL_DATA;
            col.cx = 35;
            ListView_InsertColumn(hList, COL_DATA, &col);
            col.pszText = TXT_LVDESCR;
            col.iSubItem = COL_DESCR;
            col.cx = 9000;
            ListView_InsertColumn(hList, COL_DESCR, &col);
            
            ListView_SetItemCount(hList, MAX_LVROWS);
            //SetScrollRange(hList, SB_VERT, 0, 0, TRUE);
            //SetScrollRange(hList, SB_HORZ, 0, 0, TRUE);
            }
            return 0;
        case WM_SIZE: {
            DWORD cliWidth  = LOWORD(lParam);
            DWORD cliHeight = HIWORD(lParam);
            
            MoveWindow(hList, 0, 0, cliWidth, cliHeight, FALSE);
            SetScrollRange(hList, SB_VERT, 0, song ? song->dataSize : 0, TRUE);
            }
            return 0;
        case WM_NOTIFY: {
            NMLVDISPINFOA* plvdi = (NMLVDISPINFOA*)lParam;
            char tBuf[256];
            
            switch (((LPNMHDR)lParam)->code) {
                case LVN_GETDISPINFO: {
                    LVITEMA* pItem = &plvdi->item;
    numRows = ListView_GetCountPerPage(hList);
    if (numRows > MAX_LVROWS) numRows = MAX_LVROWS;
                    int offset = pItem->iItem - numRows/2;
                    int index  = offset + plpl->curIndex;
                    
                    if (pItem->iSubItem == COL_INDICATOR) {
                        char* szInd = (pItem->iItem == numRows/2) ? ">" : "";
                        sprintf(tBuf, "%s", szInd);
                    } else if (index >= 0 && song && index < song->dataSize) {
                        SongRow* sRow = &song->rows[index];
                        
                        switch (pItem->iSubItem) {
                            case COL_INDEX:
                                sprintf(tBuf, "%07d", index);
                                break;
                            case COL_DURAT:
                                sprintf(tBuf, "%11.8f", sRow->duration);
                                break;
                            case COL_PORT:
                                sprintf(tBuf, "%1X", sRow->port);
                                break;
                            case COL_DATA:
                                sprintf(tBuf, "%02X", sRow->data);
                                break;
                            case COL_DESCR:
                                //TODO
                                sprintf(tBuf, "%s", "");
                                break;
                        }
                    } else {
                        sprintf(tBuf, "%s", "");
                    }
                    pItem->pszText = tBuf;
                    return 0;
                } break;
            }
        } break;
        case WM_TIMER:
            if (wParam == TIMER_PLAY) {
                if (!plpl->isPlaying) KillTimer(hMain, TIMER_PLAY);
                redrawList();
                return 0;
            }
            break;
        case WM_COMMAND: {
            int com = LOWORD(wParam);
            
            switch (com) {
                case ACC_OPENFILE:
                    openFileDialog();
                    return 0;
                case ACC_PLAYTOGGLE:
                    esslp_playCtrl(!plpl->isPlaying);
                    //do check after unpause
                    if (plpl->isPlaying) {
                        SetTimer(hMain, TIMER_PLAY, DELAY_ANIM, NULL);
                    }
                    redrawList();
                    return 0;
            }
        } break;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	MSG msg;
	WNDCLASSEXA wc;
    
    if (!initInOut()) {
        OutputDebugStringA("Failed to init InpOut!\n");
        return -1;
    }
    if (!(plpl = esslp_init())) {
        OutputDebugStringA("Failed to init Player!\n");
        return -1;
    }
    
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = proc_mainWnd;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = wndClassName;
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassExA(&wc)) return -1;
    
    hAccel = CreateAcceleratorTableA(tabAccel, COUNTOF(tabAccel));
    
    hFont = CreateFontA(FONT_DEFSIZE, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET, 
      OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
      DEFAULT_PITCH | FF_DONTCARE, "Monospace");
    
    hMain = CreateWindowExA(WS_EX_APPWINDOW|WS_EX_CLIENTEDGE, wndClassName, "",
      WS_OVERLAPPEDWINDOW|WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, //WS_CLIPCHILDREN
      NULL, NULL, hInstance, NULL);
	if (!hAccel || !hFont || !hMain) return -1;
    
	while(GetMessageA(&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator(hMain, hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
	}
	return msg.wParam;
}