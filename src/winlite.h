Warning: truncated output (original token count: 2242)
Total output lines: 187

/* Minimal Win32 declarations. Keeping the executable free of SDK/CRT runtime dependencies.
   Target: Windows 10 x64, Unicode, MSVC x64 calling convention. */
#ifndef LZ_WINLITE_H
#define LZ_WINLITE_H
#if defined(_MSC_VER) && !defined(__clang__)
#define WINCALL __stdcall
#else
#define WINCALL __attribute__((ms_abi))
#endif
typedef unsigned short wchar_t;
#define WINIMP __declspec(dllimport)
typedef int BOOL;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef unsigned int UINT;
typedef unsigned long long ULONGLONG;
typedef unsigned long long UINT_PTR;
typedef unsigned long long WPARAM;
typedef long long LPARAM;
typedef long long LRESULT;
typedef long long LONG_PTR;
typedef long long INT_PTR;
typedef void *HANDLE;
typedef HANDLE HWND;
typedef HANDLE HMONITOR;
typedef HANDLE HMODULE;
typedef HANDLE HINSTANCE;
typedef HANDLE HDC;
typedef HANDLE HBRUSH;
typedef HANDLE HICON;
typedef HANDLE HFONT;
typedef HANDLE HCURSOR;
typedef HANDLE HMENU;
typedef HANDLE HGDIOBJ;
typedef HANDLE HWINEVENTHOOK;
typedef HANDLE HKEY;
typedef struct { int x, y; } POINT;
typedef struct { int left,top,right,bottom; } RECT;
typedef LRESULT (WINCALL *WNDPROC)(HWND,UINT,WPARAM,LPARAM);
typedef void (WINCALL *WINEVENTPROC)(HWINEVENTHOOK,DWORD,HWND,int,int,DWORD,DWORD);
typedef BOOL (WINCALL *MONITORENUMPROC)(HMONITOR,HDC,RECT*,LPARAM);
typedef struct {
    UINT cbSize, style;
    WNDPROC lpfnWndProc;
    int cbClsExtra, cbWndExtra;
    HINSTANCE hInstance;
    HICON hIcon;
    HCURSOR hCursor;
    HBRUSH hbrBackground;
    const wchar_t *lpszMenuName, *lpszClassName;
    HICON hIconSm;
} WNDCLASSEXW;
typedef struct { DWORD cbSize; RECT rcMonitor,rcWork; DWORD dwFlags; wchar_t szDevice[32]; } MONITORINFOEXW;
typedef struct { HDC hdc; BOOL fErase; RECT rcPaint; BOOL fRestore,fIncUpdate; BYTE rgbReserved[32]; } PAINTSTRUCT;
typedef struct { HWND hwnd; UINT message; WPARAM wParam; LPARAM lParam; DWORD time; POINT pt; DWORD lPrivate; } MSG;
typedef struct { DWORD Data1; WORD Data2,Data3; BYTE Data4[8]; } GUID;
typedef struct {
    UINT CtlType,CtlID,itemID,itemWidth,itemHeight;
    UINT_PTR itemData;
} MEASUREITEMSTRUCT;
typedef struct {
    UINT CtlType,CtlID,itemID,itemAction,itemState;
    HWND hwndItem; HDC hDC; RECT rcItem; UINT_PTR itemData;
} DRAWITEMSTRUCT;
typedef char lz_assert_measureitem_size[(sizeof(MEASUREITEMSTRUCT)==32)?1:-1];
typedef char lz_assert_drawitem_size[(sizeof(DRAWITEMSTRUCT)==64)?1:-1];

typedef struct {
    DWORD cbSize; HWND hWnd; UINT uID,uFlags,uCallbackMessage; HICON hIcon;
    wchar_t szTip[128]; DWORD dwState,dwStateMask; wchar_t szInfo[256];
    UINT uTimeoutOrVersion; wchar_t szInfoTitle[64]; DWORD dwInfoFlags;
    GUID guidItem; HICON hBalloonIcon;
} NOTIFYICONDATAW;
typedef struct {
    DWORD lStructSize; HWND hwndOwner; HINSTANCE hInstance; DWORD rgbResult;
    DWORD *lpCustColors; DWORD Flags; LPARAM lCustData;
    void *lpfnHook; const wchar_t *lpTemplateName;
} CHOOSECOLORW;
typedef char lz_assert…742 tokens truncated…NIMP BOOL WINCALL EnumDisplayMonitors(HDC,const RECT*,MONITORENUMPROC,LPARAM);
WINIMP BOOL WINCALL EnumWindows(BOOL (WINCALL *)(HWND,LPARAM),LPARAM);
WINIMP BOOL WINCALL GetMonitorInfoW(HMONITOR,MONITORINFOEXW*);
WINIMP UINT_PTR WINCALL SetTimer(HWND,UINT_PTR,UINT,void*);
WINIMP BOOL WINCALL KillTimer(HWND,UINT_PTR);
WINIMP HWINEVENTHOOK WINCALL SetWinEventHook(DWORD,DWORD,HMODULE,WINEVENTPROC,DWORD,DWORD,DWORD);
WINIMP BOOL WINCALL UnhookWinEvent(HWINEVENTHOOK);
WINIMP BOOL WINCALL SetLayeredWindowAttributes(HWND,DWORD,BYTE,DWORD);
WINIMP BOOL WINCALL InvalidateRect(HWND,const RECT*,BOOL);
WINIMP HDC WINCALL BeginPaint(HWND,PAINTSTRUCT*);
WINIMP BOOL WINCALL EndPaint(HWND,const PAINTSTRUCT*);
WINIMP BOOL WINCALL FillRect(HDC,const RECT*,HBRUSH);
WINIMP BOOL WINCALL FrameRect(HDC,const RECT*,HBRUSH);
WINIMP int WINCALL DrawTextW(HDC,const wchar_t*,int,RECT*,UINT);
WINIMP HICON WINCALL LoadIconW(HINSTANCE,const wchar_t*);
WINIMP HCURSOR WINCALL LoadCursorW(HINSTANCE,const wchar_t*);
WINIMP BOOL WINCALL SetForegroundWindow(HWND);
WINIMP HMENU WINCALL CreatePopupMenu(void);
WINIMP BOOL WINCALL AppendMenuW(HMENU,UINT,UINT_PTR,const wchar_t*);
WINIMP BOOL WINCALL DestroyMenu(HMENU);
WINIMP BOOL WINCALL CheckMenuItem(HMENU,UINT,UINT);
WINIMP UINT WINCALL TrackPopupMenu(HMENU,UINT,int,int,int,HWND,const RECT*);
WINIMP int WINCALL MessageBoxW(HWND,const wchar_t*,const wchar_t*,UINT);
WINIMP UINT WINCALL RegisterWindowMessageW(const wchar_t*);
WINIMP DWORD WINCALL GetSysColor(int);
/* gdi32.dll */
WINIMP int WINCALL SetBkMode(HDC,int);
WINIMP DWORD WINCALL SetTextColor(HDC,DWORD);
WINIMP HBRUSH WINCALL CreateSolidBrush(DWORD);
WINIMP BOOL WINCALL DeleteObject(HGDIOBJ);
WINIMP HGDIOBJ WINCALL SelectObject(HDC,HGDIOBJ);
WINIMP HGDIOBJ WINCALL CreateFontW(int,int,int,int,int,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,const wchar_t*);
WINIMP HGDIOBJ WINCALL CreatePen(int,int,DWORD);
WINIMP BOOL WINCALL RoundRect(HDC,int,int,int,int,int,int);
WINIMP HDC WINCALL CreateCompatibleDC(HDC);
WINIMP BOOL WINCALL DeleteDC(HDC);
WINIMP HGDIOBJ WINCALL CreateCompatibleBitmap(HDC,int,int);
WINIMP BOOL WINCALL SetViewportOrgEx(HDC,int,int,POINT*);
WINIMP BOOL WINCALL BitBlt(HDC,int,int,int,int,HDC,int,int,DWORD);
/* shell32.dll */
WINIMP BOOL WINCALL Shell_NotifyIconW(DWORD,NOTIFYICONDATAW*);
/* dwmapi.dll */
WINIMP long WINCALL DwmGetWindowAttribute(HWND,DWORD,void*,DWORD);
WINIMP long WINCALL DwmGetColorizationColor(DWORD*,BOOL*);
/* comdlg32.dll */
WINIMP BOOL WINCALL ChooseColorW(CHOOSECOLORW*);
/* advapi32.dll */
WINIMP long WINCALL RegOpenKeyExW(HKEY,const wchar_t*,DWORD,DWORD,HKEY*);
WINIMP long WINCALL RegCreateKeyExW(HKEY,const wchar_t*,DWORD,wchar_t*,DWORD,DWORD,void*,HKEY*,DWORD*);
WINIMP long WINCALL RegQueryValueExW(HKEY,const wchar_t*,DWORD*,DWORD*,BYTE*,DWORD*);
WINIMP long WINCALL RegSetValueExW(HKEY,const wchar_t*,DWORD,DWORD,const BYTE*,DWORD);
WINIMP long WINCALL RegDeleteValueW(HKEY,const wchar_t*);
WINIMP long WINCALL RegCloseKey(HKEY);
#endif

