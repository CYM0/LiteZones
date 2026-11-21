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
typedef struct {
    DWORD cb;
    wchar_t DeviceName[32],DeviceString[128];
    DWORD StateFlags;
    wchar_t DeviceID[128],DeviceKey[128];
} DISPLAY_DEVICEW;
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
typedef char lz_assert_choosecolor_size[(sizeof(CHOOSECOLORW)==72)?1:-1];
/* Compile-time ABI size checks for 64-bit Windows structures. */
typedef char lz_assert_rect_size[(sizeof(RECT)==16)?1:-1];
typedef char lz_assert_point_size[(sizeof(POINT)==8)?1:-1];
typedef char lz_assert_monitorinfoex_size[(sizeof(MONITORINFOEXW)==104)?1:-1];
typedef char lz_assert_display_device_size[(sizeof(DISPLAY_DEVICEW)==840)?1:-1];
typedef char lz_assert_msg_size[(sizeof(MSG)==48)?1:-1];
typedef char lz_assert_class_size[(sizeof(WNDCLASSEXW)==80)?1:-1];
typedef char lz_assert_notify_size[(sizeof(NOTIFYICONDATAW)==976)?1:-1];

/* kernel32.dll */
WINIMP void WINCALL ExitProcess(UINT);
WINIMP DWORD WINCALL GetLastError(void);
WINIMP WORD WINCALL GetUserDefaultUILanguage(void);
WINIMP HMODULE WINCALL GetModuleHandleW(const wchar_t*);
WINIMP void* WINCALL GetProcAddress(HMODULE,const char*);
WINIMP HANDLE WINCALL CreateMutexW(void*,BOOL,const wchar_t*);
WINIMP BOOL WINCALL CloseHandle(HANDLE);
WINIMP DWORD WINCALL GetEnvironmentVariableW(const wchar_t*,wchar_t*,DWORD);
WINIMP BOOL WINCALL CreateDirectoryW(const wchar_t*,void*);
WINIMP UINT WINCALL GetPrivateProfileIntW(const wchar_t*,const wchar_t*,int,const wchar_t*);
WINIMP BOOL WINCALL WritePrivateProfileStringW(const wchar_t*,const wchar_t*,const wchar_t*,const wchar_t*);
WINIMP DWORD WINCALL GetModuleFileNameW(HMODULE,wchar_t*,DWORD);
WINIMP DWORD WINCALL GetPrivateProfileStringW(const wchar_t*,const wchar_t*,const wchar_t*,wchar_t*,DWORD,const wchar_t*);
/* user32.dll */
WINIMP BOOL WINCALL SetProcessDPIAware(void);
WINIMP unsigned short WINCALL RegisterClassExW(const WNDCLASSEXW*);
WINIMP HWND WINCALL CreateWindowExW(DWORD,const wchar_t*,const wchar_t*,DWORD,int,int,int,int,HWND,HMENU,HINSTANCE,void*);
WINIMP BOOL WINCALL DestroyWindow(HWND);
WINIMP LRESULT WINCALL DefWindowProcW(HWND,UINT,WPARAM,LPARAM);
WINIMP int WINCALL GetMessageW(MSG*,HWND,UINT,UINT);
WINIMP BOOL WINCALL TranslateMessage(const MSG*);
WINIMP LRESULT WINCALL DispatchMessageW(const MSG*);
WINIMP void WINCALL PostQuitMessage(int);
WINIMP BOOL WINCALL ShowWindow(HWND,int);
WINIMP BOOL WINCALL SetWindowPos(HWND,HWND,int,int,int,int,UINT);
WINIMP BOOL WINCALL GetWindowRect(HWND,RECT*);
WINIMP BOOL WINCALL GetClientRect(HWND,RECT*);
WINIMP HWND WINCALL SetCapture(HWND);
WINIMP BOOL WINCALL ReleaseCapture(void);
WINIMP BOOL WINCALL MoveWindow(HWND,int,int,int,int,BOOL);
WINIMP int WINCALL GetWindowTextW(HWND,wchar_t*,int);
WINIMP BOOL WINCALL SetWindowTextW(HWND,const wchar_t*);
WINIMP LRESULT WINCALL SendMessageW(HWND,UINT,WPARAM,LPARAM);
WINIMP BOOL WINCALL IsWindow(HWND);
WINIMP HWND WINCALL GetForegroundWindow(void);
WINIMP BOOL WINCALL IsWindowVisible(HWND);
WINIMP BOOL WINCALL IsIconic(HWND);
WINIMP BOOL WINCALL IsZoomed(HWND);
WINIMP LONG_PTR WINCALL GetWindowLongPtrW(HWND,int);
WINIMP BOOL WINCALL GetCursorPos(POINT*);
WINIMP LRESULT WINCALL SendMessageTimeoutW(HWND,UINT,WPARAM,LPARAM,UINT,UINT,UINT_PTR*);
WINIMP short WINCALL GetAsyncKeyState(int);
WINIMP HMONITOR WINCALL MonitorFromPoint(POINT,DWORD);
WINIMP HMONITOR WINCALL MonitorFromWindow(HWND,DWORD);
WINIMP BOOL WINCALL EnumDisplayMonitors(HDC,const RECT*,MONITORENUMPROC,LPARAM);
WINIMP BOOL WINCALL EnumDisplayDevicesW(const wchar_t*,DWORD,DISPLAY_DEVICEW*,DWORD);
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
