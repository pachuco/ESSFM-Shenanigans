#include <windows.h>
#include <commctrl.h>

#include "guiresources.h"
#include "support.h"
#include "logplayer.h"
#include "file.h"
#include "_fm.c"

char wndClassName[] = "cls_logThinghieWindow";
static HWND hMain;
static HFONT hFont;
static HWND hList;
static DWORD cliWidth;
static DWORD cliHeight;
//static int numRows;
static HACCEL hAccel;
static char filePath[2048];
static char fileName[256];

static LogPlayer* plpl;
static Song* song;

#define TIMER_PLAY 1

enum {
    RANGE_ACC = 10000,
    ACC_OPENFILE,
    ACC_PLAYTOGGLE
};

enum {
    SIT_INDICATOR,
    SIT_INDEX,
    SIT_DURAT,
    SIT_PORT,
    SIT_DATA,
    SIT_DESCR
};



static ACCEL tabAccel[] = {
    {FVIRTKEY, VK_F3,       ACC_OPENFILE},
    {FVIRTKEY, VK_SPACE,    ACC_PLAYTOGGLE}
};



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
            KillTimer(hMain, TIMER_PLAY);
            InvalidateRect(hMain, NULL, FALSE);
        }
    }
}



static LRESULT proc_listView(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    NMLVDISPINFOA* plvdi;
    switch (msg) {
        case WM_SIZE:
            break;
        case WM_VSCROLL:
            break;
        default:
            return CallWindowProc((void*)GetWindowLong(hwnd, GWL_USERDATA), hwnd, msg, wParam, lParam);
    }
    return 0;
}

static LRESULT CALLBACK proc_mainWnd(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	LVCOLUMN col;
	LVITEMA item = {};

	switch (msg) {
        case WM_DESTROY:
            esslp_destroy();
            FM_stopSynth();
            if (song) free(song);
            PostQuitMessage(0);
            break;
        case WM_CREATE:
            InitCommonControls();
            hList = CreateWindowExA(0, WC_LISTVIEW, NULL, WS_CHILD|WS_VISIBLE|LVS_REPORT|LVS_OWNERDATA,
              0, 0, 10, 10, hwnd, (HMENU)1, ((LPCREATESTRUCTA)lParam)->hInstance, NULL);
            if (!hList) PostQuitMessage(1);
            SetWindowLongA(hList, GWL_USERDATA, SetWindowLongA(hList, GWL_WNDPROC, (LONG)proc_listView));
            
            SendMessage(hList, WM_SETFONT, hFont, TRUE);
            
            col.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
            col.fmt  = LVCFMT_LEFT;
            
            col.pszText = " ";
            col.iSubItem = SIT_INDICATOR;
            col.cx = 20;
            ListView_InsertColumn(hList, SIT_INDICATOR, &col);
            col.pszText = COL_INDEX;
            col.iSubItem = SIT_INDEX;
            col.cx = 100;
            ListView_InsertColumn(hList, SIT_INDEX, &col);
            col.pszText = COL_DURAT;
            col.iSubItem = SIT_DURAT;
            col.cx = 100;
            ListView_InsertColumn(hList, SIT_DURAT, &col);
            col.pszText = COL_PORT;
            col.iSubItem = SIT_PORT;
            col.cx = 35;
            ListView_InsertColumn(hList, SIT_PORT, &col);
            col.pszText = COL_DATA;
            col.iSubItem = SIT_DATA;
            col.cx = 35;
            ListView_InsertColumn(hList, SIT_DATA, &col);
            col.pszText = COL_DESCR;
            col.iSubItem = SIT_DESCR;
            col.cx = 9000;
            ListView_InsertColumn(hList, SIT_DESCR, &col);

            item.mask = LVIF_TEXT;
            for (int i=0 ;i < 512 ; i++) {
                item.pszText = LPSTR_TEXTCALLBACK;
                item.iItem = i;
                item.iSubItem = 0;
                ListView_InsertItem(hList, &item);
            }
            break;
        case WM_SIZE:
            cliWidth  = LOWORD(lParam);
            cliHeight = HIWORD(lParam);
            
            MoveWindow(hList, 0, 0, cliWidth, cliHeight, FALSE);
            break;
        case WM_NOTIFY: {
            NMLVDISPINFOA* plvdi = (NMLVDISPINFOA*)lParam;
            
            switch (((LPNMHDR)lParam)->code) {
                case LVN_GETDISPINFO: {
                    LVITEMA* pItem = &plvdi->item;
                    int numRows    = ListView_GetCountPerPage(hList);
                    int offset = pItem->iItem - numRows/2;
                    int index  = offset + plpl->curIndex;
                    
                    if (!song || index < 0 || index >= song->dataSize) {
                        pItem->pszText = " ";
                        break;
                    } else {
                        SongRow* sRow = &song->rows[index];
                        char tBuf[256];
                        
                        tBuf[0] = 0;
                        switch (pItem->iSubItem) {
                            case SIT_INDICATOR:
                                //tBuf[0] = (offset == 0) ? '>' : ' ';
                                break;
                            case SIT_INDEX:
                                sprintf(tBuf, "%07d", index);
                                break;
                            case SIT_DURAT:
                                sprintf(tBuf, "%11.8f", sRow->duration);
                                break;
                            case SIT_PORT:
                                sprintf(tBuf, "%1X", sRow->port);
                                break;
                            case SIT_DATA:
                                sprintf(tBuf, "%02X", sRow->data);
                                break;
                            case SIT_DESCR:
                                break;
                        }
                        pItem->pszText = tBuf;
                    }
                    if (pItem->iSubItem == SIT_INDICATOR && offset == 0) pItem->pszText = ">";
                } break;
            }
        } break;
        case WM_TIMER:
            //SendMessage(hList, WM_SETREDRAW, FALSE, 0);
            if (wParam == TIMER_PLAY) {
                if (!plpl->isPlaying) KillTimer(hMain, TIMER_PLAY);
                InvalidateRect(hMain, NULL, FALSE);
            }
            //SendMessage(hList, WM_SETREDRAW, TRUE, 0);
            break;
        case WM_COMMAND: {
            int com = LOWORD(wParam);
            
            switch (com) {
                case ACC_OPENFILE:
                    openFileDialog();
                    break;
                case ACC_PLAYTOGGLE:
                    esslp_playCtrl(!plpl->isPlaying);
                    if (plpl->isPlaying) {
                        SetTimer(hMain, TIMER_PLAY, 100, NULL);
                    }
                    InvalidateRect(hMain, NULL, FALSE);
                    break;
            }
        } break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	MSG msg;
	WNDCLASSEXA wc;
    
    if (!initInOut()) {
        printf("Failed to init InpOut!\n");
        return -1;
    }
    if (!(plpl = esslp_init())) {
        printf("Failed to init Player!\n");
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
      WS_OVERLAPPEDWINDOW|WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
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