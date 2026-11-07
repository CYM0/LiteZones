/* LiteZones 1.0.0: self-contained Win32 x64 snap-zone assistant for Windows 10.
   No dependency on the C runtime, frameworks, browser engines, services, or drivers. */
#include "winlite.h"
#include "geometry.h"
#include "zone_book.h"
#include "zone_zorder.h"
#include "restore.h"
#include "frame_math.h"
#include "seam.h"

#define WM_APP_TRAY (0x8000u + 25)
#define WM_TIMER 0x0113u
#define WM_PAINT 0x000Fu
#define WM_DRAWITEM 0x002Bu
#define WM_MEASUREITEM 0x002Cu
#define WM_DESTROY 0x0002u
#define WM_DISPLAYCHANGE 0x007Eu
#define WM_SETTINGCHANGE 0x001Au
#define WM_DEVICECHANGE 0x0219u
#define WM_NCHITTEST 0x0084u
#define WM_RBUTTONUP 0x0205u
#define WM_LBUTTONUP 0x0202u
#define WM_CONTEXTMENU 0x007Bu
#define HTTRANSPARENT (-1)
#define WS_POPUP 0x80000000u
#define WS_EX_TOOLWINDOW 0x00000080u
#define WS_EX_TRANSPARENT 0x00000020u
#define WS_EX_TOPMOST 0x00000008u
#define WS_EX_LAYERED 0x00080000u
#define WS_EX_NOACTIVATE 0x08000000u
#define WS_CHILD 0x40000000u
#define SW_HIDE 0
#define SW_SHOWNOACTIVATE 4
#define SW_RESTORE 9
#define SWP_NOSIZE 0x0001
#define SWP_NOMOVE 0x0002
#define SWP_NOZORDER 0x0004
#define SWP_NOACTIVATE 0x0010
#define GWL_STYLE (-16)
#define GWL_EXSTYLE (-20)
#define WINEVENT_OUTOFCONTEXT 0x0000
#define WINEVENT_SKIPOWNPROCESS 0x0002
#define EVENT_SYSTEM_FOREGROUND 0x0003u
#define EVENT_SYSTEM_MOVESIZESTART 0x000Au
#define EVENT_SYSTEM_MOVESIZEEND 0x000Bu
#define EVENT_OBJECT_DESTROY 0x8001u
#define VK_LBUTTON 0x01
#define VK_SHIFT 0x10
#define VK_ESCAPE 0x1B
#define MONITOR_DEFAULTTONEAREST 2
#define LWA_COLORKEY 0x0001
#define LWA_ALPHA 0x0002
#define DT_CENTER 0x0001
#define DT_VCENTER 0x0004
#define DT_SINGLELINE 0x0020
#define DT_END_ELLIPSIS 0x8000
#define TRANSPARENT 1
#define MF_STRING 0x0000
#define MF_OWNERDRAW 0x0100u
#define MF_GRAYED 0x0001u
#define ODS_SELECTED 0x0001u
#define MF_POPUP 0x0010
#define MF_SEPARATOR 0x0800
#define MF_CHECKED 0x0008
#define MF_BYCOMMAND 0x0000
#define TPM_RIGHTBUTTON 0x0002
#define TPM_RETURNCMD 0x0100
#define NIM_ADD 0
#define NIM_MODIFY 1
#define NIM_DELETE 2
#define NIF_MESSAGE 0x0001
#define NIF_ICON 0x0002
#define NIF_TIP 0x0004
#define MB_OK 0
#define MB_ICONINFORMATION 0x40
#define MB_ICONERROR 0x10
#define HKEY_CURRENT_USER ((HKEY)(INT_PTR)(int)0x80000001u)
#define KEY_QUERY_VALUE 0x0001
#define KEY_SET_VALUE 0x0002
#define REG_SZ 1
#define ERROR_ALREADY_EXISTS 183
#define CMD_ENABLE 1001
#define CMD_HELP 1002
#define CMD_EXIT 1003
#define CMD_STARTUP 1004
#define CMD_LANGUAGE 1005
#define CMD_GAP_0 1010
#define CMD_GAP_8 1011
#define CMD_GAP_16 1012
#define CMD_THEME_BASE 1100
#define CMD_THEME_CUSTOM_PICK 1115
#define CMD_OPACITY_LIGHT 1120
#define CMD_OPACITY_NORMAL 1121
#define CMD_OPACITY_STRONG 1122
#define CMD_FINE_TUNE 1130
#define CMD_SEAM_ENABLE 1140
#define CMD_SEAM_TUNE 1141
#define CMD_LAYOUT_BASE 2000
#define CMD_PROFILE_BASE 4000
#define CMD_RESTORE 1020
#define LZ_PROFILE_COUNT 12
#define CMD_EDITOR_BASE 3000
#define WM_CLOSE 0x0010u
#define WM_SIZE 0x0005u
#define WM_LBUTTONDOWN 0x0201u
#define WM_LBUTTONUP 0x0202u
#define WM_MOUSEMOVE 0x0200u
#define WM_CAPTURECHANGED 0x0215u
#define WS_CAPTION 0x00C00000u
#define WS_SYSMENU 0x00080000u
#define WS_THICKFRAME 0x00040000u
#define WS_MINIMIZEBOX 0x00020000u
#define WS_MAXIMIZEBOX 0x00010000u
#define SW_SHOW 5
#define MAX_MONITORS 8
#define COLORREF_(r,g,b) ((DWORD)((BYTE)(r)|((WORD)(BYTE)(g)<<8)|((DWORD)(BYTE)(b)<<16)))

static const wchar_t kClassMain[] = L"LiteZonesHiddenWindow";
static const wchar_t kClassOverlay[] = L"LiteZonesSnapOverlay";
static const wchar_t kClassEditor[] = L"LiteZonesCustomEditor";
static const wchar_t kClassTuner[] = L"LiteZonesTuner";
static const wchar_t kRunKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
static const wchar_t kLayouts[7][26] = {
    L"左右两栏",L"平均三栏",L"平均四栏",L"主窗口 + 右侧上下",L"四宫格",
    L"中间大两侧小",L"自定义分区"
};

typedef struct {
    HMONITOR handle;
    RECT screen, work;
    wchar_t device[32], configKey[32];
    HWND overlay;
    int layout, count;
    LZCustom profiles[LZ_PROFILE_COUNT];
    wchar_t profileNames[LZ_PROFILE_COUNT][64];
    int activeProfile;
    LZRect zones[LZ_MAX_ZONES];
} Display;
typedef struct {
    BOOL active, armed, cancelled;
    HWND window;
    POINT start;
    int startW, startH, startedZoomed;
    int display, hovered, missingButtonTicks;
    int originDisplay,originZone;
    RECT originalRect;
} Drag;

static Display g_displays[MAX_MONITORS];
static int g_displayCount = 0;
static HWND g_main = (HWND)0;
static HINSTANCE g_instance = (HINSTANCE)0;
static HWINEVENTHOOK g_hook = (HWINEVENTHOOK)0;
static HWINEVENTHOOK g_destroyHook = (HWINEVENTHOOK)0;
static HWINEVENTHOOK g_focusHook = (HWINEVENTHOOK)0;
static HANDLE g_singleton = (HANDLE)0;
static NOTIFYICONDATAW g_tray;
static Drag g_drag;
static int g_enabled = 1, g_gap = 0, g_restoreOnDetach = 1;
static int g_seamEnabled = 0;
static int g_bleedLeft=1,g_bleedRight=1,g_bleedTop=0,g_bleedBottom=1;
static wchar_t g_configPath[700], g_exePath[600];
static UINT g_taskbarCreated = 0;
static HICON g_appIcon=(HICON)0;
static HWND g_editor = (HWND)0;

/* Explicitly provided compiler intrinsics: no implicit UCRT dependency. */
void *memset(void *p, int c, unsigned long long n) {
    BYTE *q=(BYTE*)p; while(n--) *q++=(BYTE)c; return p;
}
void *memcpy(void *p, const void *s, unsigned long long n) {
    BYTE *q=(BYTE*)p; const BYTE *t=(const BYTE*)s; while(n--) *q++=*t++; return p;
}
static int lengthW(const wchar_t *s) { int n=0; if(s) while(s[n]) n++; return n; }
static void copyW(wchar_t *dst,int cap,const wchar_t *src) {
    int i=0; if(cap<1) return; while(src && src[i] && i<cap-1) {dst[i]=src[i];i++;} dst[i]=0;
}
static void appendW(wchar_t *dst,int cap,const wchar_t *src) {
    int n=lengthW(dst); if(n<cap) copyW(dst+n,cap-n,src);
}
static void appendInt(wchar_t *dst,int cap,int value) {
    wchar_t buffer[16]; int i=0,j; unsigned int n;
    if(value<0) {appendW(dst,cap,L"-"); n=(unsigned int)(-value);} else n=(unsigned int)value;
    do { buffer[i++]=(wchar_t)(L'0'+n%10); n/=10; } while(n && i<15);
    for(j=i-1;j>=0;j--) { wchar_t part[2]={buffer[j],0}; appendW(dst,cap,part); }
}
#include "language.inc"
static int absolute(int v) {return v<0?-v:v;}
static int resizeHitTest(HWND hwnd, POINT p) {
    UINT_PTR result=0;
    LPARAM coords=(LPARAM)((UINT)(WORD)p.x | ((UINT)(WORD)p.y<<16));
    if(SendMessageTimeoutW(hwnd,WM_NCHITTEST,0,coords,0x0002,75,&result)) {
        int hit=(int)result;
        return hit>=10 && hit<=17;
    }
    return 0; /* No response: defer to normal filtering and the OS. */
}
static void setConfigNumber(const wchar_t *section,const wchar_t *key,int number) {
    wchar_t tmp[20]={0}; appendInt(tmp,20,number);
    WritePrivateProfileStringW(section,key,tmp,g_configPath);
}
static void initPaths(void) {
    wchar_t appData[600]={0};
    DWORD n=GetEnvironmentVariableW(L"APPDATA",appData,600);
    if(!n || n>=550) {
        n=GetEnvironmentVariableW(L"USERPROFILE",appData,600);
        if(!n || n>=550) { copyW(appData,600,L"."); }
    }
    appendW(appData,600,L"\\LiteZones");
    CreateDirectoryW(appData,(void*)0);
    copyW(g_configPath,700,appData);
    appendW(g_configPath,700,L"\\settings.ini");
    GetModuleFileNameW((HMODULE)0,g_exePath,600);
    int language=(int)GetPrivateProfileIntW(L"General",L"Language",-1,g_configPath);
    g_languageEnglish=(language==0||language==1)?language:lz_detectSystemLanguage();
    g_enabled = GetPrivateProfileIntW(L"General",L"Enabled",1,g_configPath) ? 1:0;
    g_gap=(int)GetPrivateProfileIntW(L"General",L"Gap",0,g_configPath);
    g_restoreOnDetach=GetPrivateProfileIntW(L"General",L"RestoreOnDetach",1,g_configPath)?1:0;
    if(g_gap<0||g_gap>40) g_gap=0;
    g_seamEnabled=(int)GetPrivateProfileIntW(L"Seam",L"Enabled",0,g_configPath)?1:0;
    g_bleedLeft=(int)GetPrivateProfileIntW(L"Seam",L"Left",1,g_configPath);
    g_bleedRight=(int)GetPrivateProfileIntW(L"Seam",L"Right",1,g_configPath);
    g_bleedTop=(int)GetPrivateProfileIntW(L"Seam",L"Top",0,g_configPath);
    g_bleedBottom=(int)GetPrivateProfileIntW(L"Seam",L"Bottom",1,g_configPath);
    if(g_bleedLeft<0||g_bleedLeft>12)g_bleedLeft=1;
    if(g_bleedRight<0||g_bleedRight>12)g_bleedRight=1;
    if(g_bleedTop<0||g_bleedTop>12)g_bleedTop=0;
    if(g_bleedBottom<0||g_bleedBottom>12)g_bleedBottom=1;
}
static void deviceKey(Display *d) {
    int i,j=0;
    for(i=0;d->device[i] && j<30;i++) {
        wchar_t ch=d->device[i];
        if((ch>=L'A'&&ch<=L'Z')||(ch>=L'a'&&ch<=L'z')||(ch>=L'0'&&ch<=L'9')) d->configKey[j++]=ch;
    }
    d->configKey[j]=0;
    if(!j) copyW(d->configKey,32,L"DisplayFallback");
}
static int startupEnabled(void) {
    HKEY key=(HKEY)0; long err; DWORD dataSize=0;
    err=RegOpenKeyExW(HKEY_CURRENT_USER,kRunKey,0,KEY_QUERY_VALUE,&key);
    if(err) return 0;
    err=RegQueryValueExW(key,L"LiteZones",(DWORD*)0,(DWORD*)0,(BYTE*)0,&dataSize);
    RegCloseKey(key);
    return err==0;
}
static void setStartup(int enabled) {
    HKEY key=(HKEY)0; long err;
    if(enabled) {
        wchar_t command[650]=L"\"";
        appendW(command,650,g_exePath); appendW(command,650,L"\"");
        err=RegCreateKeyExW(HKEY_CURRENT_USER,kRunKey,0,(wchar_t*)0,0,KEY_SET_VALUE,(void*)0,&key,(DWORD*)0);
        if(err==0) {
            err=RegSetValueExW(key,L"LiteZones",0,REG_SZ,(const BYTE*)command,(DWORD)((lengthW(command)+1)*sizeof(wchar_t)));
            RegCloseKey(key);
        }
    } else {
        err=RegOpenKeyExW(HKEY_CURRENT_USER,kRunKey,0,KEY_SET_VALUE,&key);
        if(err==0) {err=RegDeleteValueW(key,L"LiteZones");RegCloseKey(key);}
    }
    if(err!=0) MessageBoxW(g_main,lz_text(L"无法更改开机自启设置，请检查 Windows 用户权限。"),L"LiteZones",MB_OK|MB_ICONERROR);
}
static void dpiAware(void) {
    HMODULE u=GetModuleHandleW(L"user32.dll");
    typedef BOOL (WINCALL *SetDpiCtx)(void*);
    SetDpiCtx api=(SetDpiCtx)GetProcAddress(u,"SetProcessDpiAwarenessContext");
    if(!api || !api((void*)(INT_PTR)-4)) SetProcessDPIAware();
}
static int indexOfMonitor(HMONITOR m) {
    int i; for(i=0;i<g_displayCount;i++) if(g_displays[i].handle==m) return i;
    return -1;
}
static int monitorUnderCursor(POINT p) {
    return indexOfMonitor(MonitorFromPoint(p,MONITOR_DEFAULTTONEAREST));
}
static LZRect workRect(Display *d) {
    LZRect r={d->work.left,d->work.top,d->work.right,d->work.bottom}; return r;
}
static void refreshZones(Display *d) {
    if(d->layout==LZ_LAYOUT_CUSTOM)
        d->count=lz_custom_zones(workRect(d),&d->profiles[d->activeProfile],g_gap,d->zones,(int*)0);
    else d->count=lz_compute_zones(workRect(d),d->layout,g_gap,d->zones);
}
#include "theme.inc"
static HWND occupying(int mi,int zi,HWND except);
static int nearestVacant(int mi,int zi);
static LRESULT WINCALL overlayProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam) {
    (void)wParam; (void)lParam;
    if(msg==WM_NCHITTEST) return HTTRANSPARENT;
    if(msg==WM_PAINT) {
        PAINTSTRUCT ps; HDC dc=BeginPaint(hwnd,&ps);
        int i,j;
        HBRUSH mask=CreateSolidBrush(COLORREF_(255,0,255));
        RECT whole;
        for(i=0;i<g_displayCount;i++) if(g_displays[i].overlay==hwnd) break;
        if(i<g_displayCount) {
            Display *d=&g_displays[i];
            whole.left=0;whole.top=0;
            whole.right=d->work.right-d->work.left;
            whole.bottom=d->work.bottom-d->work.top;
            FillRect(dc,&whole,mask);
            SetBkMode(dc,TRANSPARENT);
            SetTextColor(dc,g_themeMode==LZ_THEME_CLASSIC?COLORREF_(255,255,255):COLORREF_(76,46,68));
            for(j=0;j<d->count;j++) {
                int selected=g_drag.active && g_drag.armed && g_drag.display==i && g_drag.hovered==j;
                int covered=selected && g_drag.originDisplay<0 &&
                    occupying(i,j,g_drag.window)!=(HWND)0;
                int relocating=covered && nearestVacant(i,j)>=0;
                LZRect *z=&d->zones[j];
                RECT r={z->left-d->work.left,z->top-d->work.top,z->right-d->work.left,z->bottom-d->work.top};
                HBRUSH background=CreateSolidBrush(g_themeMode==LZ_THEME_CLASSIC ?
                    (selected?COLORREF_(44,117,208):COLORREF_(43,63,85)) : lz_softColor(selected));
                HBRUSH border=CreateSolidBrush(g_themeMode==LZ_THEME_CLASSIC ?
                    (selected?COLORREF_(200,230,255):COLORREF_(126,160,192)) :
                    lz_mix(lz_accent(),COLORREF_(255,255,255),selected?5:55));
                wchar_t label[20];copyW(label,20,lz_text(L"区域 "));
                if(covered)copyW(label,20,relocating?L"腾挪旧窗口":L"覆盖此区");
                else appendInt(label,20,j+1);
                FillRect(dc,&r,background);
                FrameRect(dc,&r,border);
                if((r.right-r.left)>50 && (r.bottom-r.top)>32) DrawTextW(dc,lz_text(label),-1,&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
                DeleteObject((HGDIOBJ)background);
                DeleteObject((HGDIOBJ)border);
            }
        }
        DeleteObject((HGDIOBJ)mask);
        EndPaint(hwnd,&ps);return 0;
    }
    return DefWindowProcW(hwnd,msg,wParam,lParam);
}
static void hideOverlays(void) {
    int i;for(i=0;i<g_displayCount;i++) if(g_displays[i].overlay) ShowWindow(g_displays[i].overlay,SW_HIDE);
}
static void clearDrag(void) {
    KillTimer(g_main,1);
    hideOverlays();
    memset(&g_drag,0,sizeof(g_drag));
    g_drag.display=-1;g_drag.hovered=-1;
}
static void destroyDisplays(void) {
    int i;for(i=0;i<g_displayCount;i++) {
        if(g_displays[i].overlay) DestroyWindow(g_displays[i].overlay);
        g_displays[i].overlay=(HWND)0;
    }
    g_displayCount=0;
}
static void loadCustom(Display *d);
static BOOL WINCALL enumDisplay(HMONITOR h,HDC dc,RECT *r,LPARAM value) {
    MONITORINFOEXW info;
    Display *d;
    (void)dc;(void)r;(void)value;
    if(g_displayCount>=MAX_MONITORS) return 0;
    memset(&info,0,sizeof(info));info.cbSize=sizeof(info);
    if(!GetMonitorInfoW(h,&info)) return 1;
    d=&g_displays[g_displayCount];memset(d,0,sizeof(*d));
    d->handle=h;d->screen=info.rcMonitor;d->work=info.rcWork;
    copyW(d->device,32,info.szDevice);deviceKey(d);
    d->layout=(int)GetPrivateProfileIntW(L"Monitors",d->configKey,5,g_configPath);
    if(d->layout<0||d->layout>=LZ_LAYOUT_COUNT) d->layout=5;
    loadCustom(d);
    refreshZones(d);
    g_displayCount++;
    return 1;
}
static void initDisplays(void) {
    int i;
    clearDrag();destroyDisplays();
    EnumDisplayMonitors((HDC)0,(const RECT*)0,enumDisplay,0);
    for(i=0;i<g_displayCount;i++) {
        Display *d=&g_displays[i];
        d->overlay=CreateWindowExW(WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_TOOLWINDOW|WS_EX_TOPMOST|WS_EX_NOACTIVATE,
                                  kClassOverlay,L"",WS_POPUP,
                                  d->work.left,d->work.top,d->work.right-d->work.left,d->work.bottom-d->work.top,
                                  (HWND)0,(HMENU)0,g_instance,(void*)0);
        if(d->overlay) SetLayeredWindowAttributes(d->overlay,COLORREF_(255,0,255),(BYTE)g_overlayOpacity,LWA_COLORKEY|LWA_ALPHA);
    }
}
static int isEligible(HWND hwnd) {
    RECT wr;
    LONG_PTR style,exstyle;
    if(!hwnd || hwnd==g_main || !IsWindow(hwnd) || !IsWindowVisible(hwnd) || IsIconic(hwnd)) return 0;
    style=GetWindowLongPtrW(hwnd,GWL_STYLE);
    exstyle=GetWindowLongPtrW(hwnd,GWL_EXSTYLE);
    if((style&WS_CHILD)||(exstyle&(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE))) return 0;
    if(!GetWindowRect(hwnd,&wr) || wr.right-wr.left<100 || wr.bottom-wr.top<60) return 0;
    /* Do not touch true borderless fullscreen windows (games, media players). */
    {
        POINT p={(wr.left+wr.right)/2,(wr.top+wr.bottom)/2};
        HMONITOR hm=MonitorFromPoint(p,MONITOR_DEFAULTTONEAREST);
        int idx=indexOfMonitor(hm);
        if(idx>=0) {
            RECT full=g_displays[idx].screen;
            if(wr.left<=full.left && wr.top<=full.top && wr.right>=full.right && wr.bottom>=full.bottom && !IsZoomed(hwnd)) return 0;
        }
    }
    return 1;
}
static BOOL moveToZone(HWND hwnd,Display *d,int zone) {
    RECT outer,visible;
    LZRect t;
    int x,y,w,h;
    if(!IsWindow(hwnd) || zone<0 ||zone>=d->count) return 0;
    if(IsZoomed(hwnd)) ShowWindow(hwnd,SW_RESTORE);
    if(!GetWindowRect(hwnd,&outer)) return 0;
    t=d->zones[zone];
    if(g_seamEnabled) t=lz_expand_zone(t,(LZBleed){g_bleedLeft,g_bleedRight,g_bleedTop,g_bleedBottom});
    if(t.right-t.left<120||t.bottom-t.top<80)return 0;
    /* Windows 10 DWM's extended frame bounds are not identical to the
       GetWindowRect bounds, especially for resizeable Chromium/WPF windows.
       Solve against the *post-move* visible frame. A single pre-move sample
       can leave an apparent 1-8px seam even when the zone gap is zero. */
    x=t.left;y=t.top;w=t.right-t.left;h=t.bottom-t.top;
    if(DwmGetWindowAttribute(hwnd,9,&visible,sizeof(visible))==0) {
        x+=outer.left-visible.left;
        y+=outer.top-visible.top;
        w+=(visible.left-outer.left)+(outer.right-visible.right);
        h+=(visible.top-outer.top)+(outer.bottom-visible.bottom);
    }
    if(!SetWindowPos(hwnd,(HWND)0,x,y,w,h,SWP_NOZORDER|SWP_NOACTIVATE))return 0;
    for(int attempt=0;attempt<3;attempt++) {
        RECT actual;
        if(DwmGetWindowAttribute(hwnd,9,&actual,sizeof(actual))!=0)break;
        LZRect frame={actual.left,actual.top,actual.right,actual.bottom};
        LZFrameDelta delta=lz_frame_delta(t,frame);
        if(!delta.left&&!delta.right&&!delta.top&&!delta.bottom)break;
        /* A constrained or DPI-unstable window must not oscillate. */
        if(!lz_frame_delta_small(delta))break;
        x+=delta.left;y+=delta.top;
        w+=delta.right-delta.left;h+=delta.bottom-delta.top;
        if(w<120||h<80)break;
        if(!SetWindowPos(hwnd,(HWND)0,x,y,w,h,SWP_NOZORDER|SWP_NOACTIVATE))break;
    }
    /* Some applications enforce a minimum size. Never claim an exchange
       succeeded if the window couldn't fit the requested zone. */
    {
        RECT check;
        if(GetWindowRect(hwnd,&check)) {
            RECT vis;
            if(DwmGetWindowAttribute(hwnd,9,&vis,sizeof(vis))==0)check=vis;
            if(absolute(check.left-t.left)>54||absolute(check.right-t.right)>54||
               absolute(check.top-t.top)>54||absolute(check.bottom-t.bottom)>54) {
                SetWindowPos(hwnd,(HWND)0,outer.left,outer.top,outer.right-outer.left,
                    outer.bottom-outer.top,SWP_NOZORDER|SWP_NOACTIVATE);
                return 0;
            }
        }
    }
    return 1;
}
#include "stacking.inc"
#include "placements.inc"

/* Reapply compensation to each independently managed window. Logical zone
   boundaries remain unchanged, and reapplying does not compound the bleed:
   moveToZone always starts with the original zone geometry. */
static void reapplyManagedZones(void) {
    if(g_drag.active)return;
    for(int i=0;i<MAX_PLACEMENTS;i++) {
        LZMember *p=&g_book.members[i];
        if(!p->id)continue;
        if(!IsWindow(idWindow(p->id))){p->id=0;continue;}
        if(IsIconic(idWindow(p->id))||!IsWindowVisible(idWindow(p->id)))continue;
        int found=0;
        for(int m=0;m<g_displayCount;m++) {
            Display *d=&g_displays[m];
            if(m!=p->monitor)continue;
            found=1;
            if(p->zone<0||p->zone>=d->count || !IsWindow(idWindow(p->id))) {
                p->id=0;
            } else if(IsWindowVisible(idWindow(p->id)) && !IsIconic(idWindow(p->id))) {
                moveToZone(idWindow(p->id),d,p->zone);
            }
            break;
        }
        if(!found)p->id=0;
    }
}

static void finishDrag(void) {
    if(!g_drag.active)return;
    HWND hwnd=g_drag.window;
    int mi=-1, zi=-1;
    int sourceMi=g_drag.originDisplay,sourceZi=g_drag.originZone;
    int wasCancelled=g_drag.cancelled;
    int permitted=g_enabled && (GetAsyncKeyState(VK_SHIFT)&0x8000) && !wasCancelled;
    RECT previous=g_drag.originalRect;
    POINT began=g_drag.start;
    POINT release={0,0};
    BOOL hasCursor=GetCursorPos(&release);
    if(permitted && hasCursor) {
        mi=monitorUnderCursor(release);
        if(mi>=0) zi=lz_pick_zone(workRect(&g_displays[mi]),g_displays[mi].zones,
                                    g_displays[mi].count,release.x,release.y);
    }
    clearDrag();
    if(permitted && mi>=0 && zi>=0) {
        snapWithExchange(hwnd,mi,zi,sourceMi,sourceZi,&previous);
    } else if(sourceMi>=0 && !wasCancelled) {
        /* Use the ownership identified at move-start; re-checking final
           geometry would incorrectly discard it after an ordinary move. */
        detachAndRestore(hwnd,hasCursor?&release:(POINT*)0,&began,&previous,g_restoreOnDetach);
    }
}
static void updateDrag(void) {
    POINT p; int mi, hover;
    if(!g_drag.active || !g_enabled) return;
    if(GetAsyncKeyState(VK_ESCAPE)&0x8000) {g_drag.cancelled=1;hideOverlays();return;}
    if(g_drag.cancelled || !IsWindow(g_drag.window)) {clearDrag();return;}
    if(!GetCursorPos(&p)) return;
    /* Do not infer a resize from changing width or height: dragging a previously
       Windows-snapped window commonly restores its previous size mid-drag. */
    mi=monitorUnderCursor(p);
    if(mi<0) {hideOverlays();g_drag.display=-1;g_drag.hovered=-1;return;}
    /* The modifier is LEVEL-triggered, not latched: releasing Shift hides
       the preview immediately (next 35 ms timer tick) and cancels snapping. */
    if(!(GetAsyncKeyState(VK_SHIFT)&0x8000)) {
        g_drag.armed=0;
        hideOverlays();g_drag.display=-1;g_drag.hovered=-1;
        return;
    }
    g_drag.armed=1;
    hover=lz_pick_zone(workRect(&g_displays[mi]),g_displays[mi].zones,g_displays[mi].count,p.x,p.y);
    if(mi!=g_drag.display) {
        hideOverlays();g_drag.display=mi;g_drag.hovered=hover;
        if(g_displays[mi].overlay) {ShowWindow(g_displays[mi].overlay,SW_SHOWNOACTIVATE);InvalidateRect(g_displays[mi].overlay,(RECT*)0,0);}
    } else if(hover!=g_drag.hovered) {
        g_drag.hovered=hover;
        if(g_displays[mi].overlay) InvalidateRect(g_displays[mi].overlay,(RECT*)0,0);
    }
}
static void WINCALL eventHook(HWINEVENTHOOK hook,DWORD event,HWND hwnd,int idObject,int idChild,DWORD tid,DWORD ms) {
    RECT r;
    (void)hook;(void)tid;(void)ms;
    if(idObject!=0 || idChild!=0) return;
    if(event==EVENT_SYSTEM_FOREGROUND) {
        /* Windows has already changed the real z-order; mirror the new
           foreground member as the visible top of its recorded zone. */
        /* The hook is queued out-of-context: ignore obsolete events if
           another window became foreground before delivery. */
        if(hwnd && hwnd==GetForegroundWindow())activatePlacement(hwnd);
        return;
    }
    if(event==EVENT_OBJECT_DESTROY) {
        forgetWindow(hwnd);
        if(g_drag.active && g_drag.window==hwnd)clearDrag();
        return;
    }
    if(event==EVENT_SYSTEM_MOVESIZESTART) {
        if(g_drag.active) clearDrag();
        if(!g_enabled || !isEligible(hwnd) || !(GetAsyncKeyState(VK_LBUTTON)&0x8000)) return;
        {
            POINT startPoint;
            if(GetCursorPos(&startPoint) && resizeHitTest(hwnd,startPoint)) return;
        }
        memset(&g_drag,0,sizeof(g_drag));
        g_drag.active=1;g_drag.window=hwnd;g_drag.display=-1;g_drag.hovered=-1;
        g_drag.originDisplay=-1;g_drag.originZone=-1;
        knownSource(hwnd,&g_drag.originDisplay,&g_drag.originZone);
        /* Move-start can precede foreground notification: dragged managed
           window is treated as the zone's current top either way. */
        activatePlacement(hwnd);
        g_drag.startedZoomed=IsZoomed(hwnd);
        GetCursorPos(&g_drag.start);
        if(GetWindowRect(hwnd,&r)) {g_drag.startW=r.right-r.left;g_drag.startH=r.bottom-r.top;g_drag.originalRect=r;}
        SetTimer(g_main,1,35,(void*)0);
    } else if(event==EVENT_SYSTEM_MOVESIZEEND && g_drag.active && g_drag.window==hwnd) finishDrag();
}
static void updateTray(void) {
    g_tray.cbSize=sizeof(g_tray);
    g_tray.hWnd=g_main;g_tray.uID=1;
    g_tray.uFlags=NIF_MESSAGE|NIF_ICON|NIF_TIP;
    g_tray.uCallbackMessage=WM_APP_TRAY;
    g_tray.hIcon=g_appIcon?g_appIcon:LoadIconW((HINSTANCE)0,(const wchar_t*)(UINT_PTR)32512);
    copyW(g_tray.szTip,128,lz_text(g_enabled?L"LiteZones 轻格 - 按 Shift 拖动分屏":L"LiteZones 轻格 - 已暂停"));
    Shell_NotifyIconW(NIM_ADD,&g_tray);
}
static void showHelp(void) {
    if(g_languageEnglish) MessageBoxW(g_main,
      L"LiteZones 1.0.0\n\n"
      L"1. Drag the title bar of a regular application window.\n"
      L"2. Hold Shift to preview the available zones.\n"
      L"3. Release the mouse over a zone to snap the window.\n\n"
      L"Drag without Shift to detach a managed window. By default, it returns to its original size.\n"
      L"Press Esc to cancel snapping.\n\n"
      L"Right-click the notification-area icon to:\n"
      L"· Choose a layout for each display and edit custom layouts\n"
      L"· Swap managed windows or place another window in a zone\n"
      L"· Adjust spacing, appearance, or snapping settings\n"
      L"· Choose whether LiteZones starts with Windows\n"
      L"· Switch languages instantly\n\n"
      L"Some elevated, full-screen, or minimum-size-restricted windows may not be repositioned precisely.\n"
      L"Settings: %APPDATA%\\LiteZones\\settings.ini",
      L"LiteZones · User Guide",MB_OK);
    else MessageBoxW(g_main,
      L"LiteZones 轻格 1.0.0\n\n"
      L"① 用鼠标拖动普通程序窗口的标题栏。\n"
      L"② 按住 Shift，显示半透明布局预览。\n"
      L"③ 拖到目标区域，松开鼠标即可吸附。\n\n"
      L"普通拖动可自由拖出分区，默认恢复到吸附前的尺寸。\n"
      L"按 Esc 可取消当前吸附。\n\n"
      L"右键任务栏托盘中的图标：\n"
      L"· 为每台显示器独立选择布局并打开编辑器\n"
      L"· 已入区窗口互换；外部窗口可覆盖现有区域\n"
      L"· 调整窗口间距、开启或暂停吸附\n"
      L"· 可选择是否随 Windows 自动启动\n\n"
      L"提示：部分管理员窗口、游戏全屏窗口及有最小尺寸限制的窗口\n"
      L"可能无法被其他普通权限程序精确调整。\n"
      L"设置位置：%APPDATA%\\LiteZones\\settings.ini",
      L"LiteZones · 使用说明",MB_OK);
}
#include "editor.inc"
#include "tuner.inc"
#include "seam_tuner.inc"

static void showTrayMenu(void);
static void panelRefreshLanguage(void);
static void menuAction(UINT command) {
    if(command==CMD_EXIT) { DestroyWindow(g_main);return; }
    if(command==CMD_HELP) {showHelp();return;}
    if(command==CMD_ENABLE) {
        g_enabled=!g_enabled;
        setConfigNumber(L"General",L"Enabled",g_enabled);
        if(!g_enabled) clearDrag();
        Shell_NotifyIconW(NIM_DELETE,&g_tray);updateTray();return;
    }
    if(command==CMD_STARTUP) {setStartup(!startupEnabled());return;}
    if(command==CMD_LANGUAGE) {
        g_languageEnglish=!g_languageEnglish;
        setConfigNumber(L"General",L"Language",g_languageEnglish);
        lz_repaintTheme();editorInvalidate();
        if(g_tuner)InvalidateRect(g_tuner,(const RECT*)0,0);
        if(g_seamTuner)InvalidateRect(g_seamTuner,(const RECT*)0,0);
        g_tray.uFlags=NIF_MESSAGE|NIF_ICON|NIF_TIP;
        g_tray.hIcon=g_appIcon?g_appIcon:LoadIconW((HINSTANCE)0,(const wchar_t*)(UINT_PTR)32512);
        copyW(g_tray.szTip,128,lz_text(g_enabled?L"LiteZones 轻格 - 按 Shift 拖动分屏":L"LiteZones 轻格 - 已暂停"));
        Shell_NotifyIconW(NIM_MODIFY,&g_tray);
        panelRefreshLanguage();return;
    }
    if(command==CMD_RESTORE) {
        g_restoreOnDetach=!g_restoreOnDetach;
        setConfigNumber(L"General",L"RestoreOnDetach",g_restoreOnDetach);
        return;
    }
    if(command>=CMD_PROFILE_BASE && command<CMD_PROFILE_BASE+MAX_MONITORS*LZ_PROFILE_COUNT) {
        int mon=(command-CMD_PROFILE_BASE)/LZ_PROFILE_COUNT;
        int slot=(command-CMD_PROFILE_BASE)%LZ_PROFILE_COUNT;
        if(mon<g_displayCount) {
            Display *d=&g_displays[mon];
            forgetMonitor(d->device);
            d->activeProfile=slot;d->layout=LZ_LAYOUT_CUSTOM;
            refreshZones(d);
            setConfigNumber(L"Monitors",d->configKey,LZ_LAYOUT_CUSTOM);
            wchar_t section[65]=L"Profiles_";appendW(section,65,d->configKey);
            setConfigNumber(section,L"Active",slot);
        }
        return;
    }
    if(command>=CMD_THEME_BASE&&command<=CMD_THEME_BASE+LZ_THEME_CLASSIC) {
        lz_setTheme((int)(command-CMD_THEME_BASE));return;
    }
    if(command==CMD_THEME_CUSTOM_PICK) {lz_pickCustomColor();return;}
    if(command==CMD_FINE_TUNE){openTuner();return;}
    if(command==CMD_SEAM_ENABLE){
        g_seamEnabled=!g_seamEnabled;
        setConfigNumber(L"Seam",L"Enabled",g_seamEnabled);
        reapplyManagedZones();
        return;
    }
    if(command==CMD_SEAM_TUNE){openSeamTuner();return;}
    if(command>=CMD_OPACITY_LIGHT&&command<=CMD_OPACITY_STRONG) {
        g_overlayOpacity=command==CMD_OPACITY_LIGHT?65:command==CMD_OPACITY_NORMAL?108:156;
        setConfigNumber(L"Appearance",L"Opacity",g_overlayOpacity);
        lz_applyOpacity();return;
    }
    if(command>=CMD_EDITOR_BASE&&command<CMD_EDITOR_BASE+MAX_MONITORS) {
        openEditor((int)(command-CMD_EDITOR_BASE));return;
    }
    if(command==CMD_GAP_0||command==CMD_GAP_8||command==CMD_GAP_16) {
        int i;g_gap=command==CMD_GAP_0?0:command==CMD_GAP_8?8:16;
        setConfigNumber(L"General",L"Gap",g_gap);
        for(i=0;i<g_displayCount;i++) {forgetMonitor(g_displays[i].device);refreshZones(&g_displays[i]);}
        return;
    }
    if(command>=CMD_LAYOUT_BASE && command<CMD_LAYOUT_BASE+MAX_MONITORS*10) {
        int mon=(command-CMD_LAYOUT_BASE)/10;
        int layout=(command-CMD_LAYOUT_BASE)%10;
        if(mon<g_displayCount && layout<LZ_LAYOUT_COUNT) {
            forgetMonitor(g_displays[mon].device);
            g_displays[mon].layout=layout;
            refreshZones(&g_displays[mon]);
            setConfigNumber(L"Monitors",g_displays[mon].configKey,layout);
        }
    }
}
#include "panel.inc"

static LRESULT WINCALL mainProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam) {
    (void)wParam;
    if(msg==WM_APP_TRAY) {
        if(lParam==WM_RBUTTONUP || lParam==WM_CONTEXTMENU || lParam==WM_LBUTTONUP) showTrayMenu();
        return 0;
    }
    if(msg==WM_TIMER) {
        if(g_drag.active) {
            if(!(GetAsyncKeyState(VK_LBUTTON)&0x8000)) {
                /* Wait briefly for MOVESIZEEND; some apps emit it asynchronously. */
                if(++g_drag.missingButtonTicks>12) finishDrag();
            } else g_drag.missingButtonTicks=0;
            if(g_drag.active) updateDrag();
        }
        return 0;
    }
    if(msg==WM_SETTINGCHANGE) {
        /* Accent, accessibility and unrelated user settings must not discard
           ownership; rebuild only when a monitor's WORK AREA actually changes. */
        int changed=0;
        for(int i=0;i<g_displayCount;i++) {
            MONITORINFOEXW info;memset(&info,0,sizeof(info));info.cbSize=sizeof(info);
            if(GetMonitorInfoW(g_displays[i].handle,&info)) {
                RECT a=info.rcWork,b=g_displays[i].work;
                if(a.left!=b.left||a.top!=b.top||a.right!=b.right||a.bottom!=b.bottom)
                    changed=1;
            }
        }
        if(changed&&!g_drag.active) {
            if(g_editor)DestroyWindow(g_editor);
            lz_book_clear(&g_book);
            initDisplays();
        } else lz_repaintTheme();
        return 0;
    }
    if(msg==WM_DISPLAYCHANGE) {
        /* Never destroy overlays in the middle of a Windows move/size modal loop. */
        if(!g_drag.active) {
            if(g_editor)DestroyWindow(g_editor);
            lz_book_clear(&g_book);
            initDisplays();
        }
        return 0;
    }
    if(msg==WM_DESTROY) {
        if(p_hwnd)DestroyWindow(p_hwnd);
        if(g_hook) UnhookWinEvent(g_hook);
        if(g_destroyHook) UnhookWinEvent(g_destroyHook);
        if(g_focusHook) UnhookWinEvent(g_focusHook);
        g_hook=(HWINEVENTHOOK)0;
        g_destroyHook=(HWINEVENTHOOK)0;
        g_focusHook=(HWINEVENTHOOK)0;
        clearDrag();destroyDisplays();
        if(g_editor)DestroyWindow(g_editor);
        if(g_tuner)DestroyWindow(g_tuner);
        if(g_seamTuner)DestroyWindow(g_seamTuner);
        Shell_NotifyIconW(NIM_DELETE,&g_tray);
        if(g_uiFont){DeleteObject(g_uiFont);g_uiFont=(HGDIOBJ)0;}
        if(g_titleFont){DeleteObject(g_titleFont);g_titleFont=(HGDIOBJ)0;}
        if(g_menuFont){DeleteObject(g_menuFont);g_menuFont=(HGDIOBJ)0;}
        PostQuitMessage(0);return 0;
    }
    if(g_taskbarCreated && msg==g_taskbarCreated) {updateTray();return 0;}
    return DefWindowProcW(hwnd,msg,wParam,lParam);
}
static int appMain(void) {
    WNDCLASSEXW wc; MSG message;
    g_singleton=CreateMutexW((void*)0,0,L"Local\\LiteZones-v1-singleton");
    if(g_singleton && GetLastError()==ERROR_ALREADY_EXISTS) {
        MessageBoxW((HWND)0,lz_text(L"LiteZones 已经运行。请查看任务栏通知区域（小箭头）。"),L"LiteZones",MB_OK|MB_ICONINFORMATION);
        CloseHandle(g_singleton);return 0;
    }
    dpiAware();initPaths();
    g_themeMode=(int)GetPrivateProfileIntW(L"Appearance",L"Theme",LZ_THEME_CHERRY,g_configPath);
    if(g_themeMode<0||g_themeMode>LZ_THEME_CLASSIC)g_themeMode=LZ_THEME_CHERRY;
    g_customAccent=(DWORD)GetPrivateProfileIntW(L"Appearance",L"CustomColor",(int)g_customAccent,g_configPath);
    g_overlayOpacity=(int)GetPrivateProfileIntW(L"Appearance",L"Opacity",108,g_configPath);
    if(g_overlayOpacity<0||g_overlayOpacity>255)g_overlayOpacity=108;
    g_instance=(HINSTANCE)GetModuleHandleW((const wchar_t*)0);
    g_appIcon=LoadIconW(g_instance,(const wchar_t*)(UINT_PTR)201);
    g_uiFont=CreateFontW(-17,0,0,0,400,0,0,0,1,0,0,5,0,L"Segoe UI");
    g_titleFont=CreateFontW(-26,0,0,0,600,0,0,0,1,0,0,5,0,L"Segoe UI");
    g_menuFont=CreateFontW(-15,0,0,0,400,0,0,0,1,0,0,5,0,L"Segoe UI");
    memset(&wc,0,sizeof(wc));wc.cbSize=sizeof(wc);wc.hInstance=g_instance;
    wc.lpfnWndProc=mainProc;wc.lpszClassName=kClassMain;wc.hIcon=g_appIcon;wc.hIconSm=g_appIcon;
    if(!RegisterClassExW(&wc)) goto fail;
    wc.lpfnWndProc=editorProc;wc.lpszClassName=kClassEditor;
    wc.hCursor=LoadCursorW((HINSTANCE)0,(const wchar_t*)(UINT_PTR)32512);
    if(!RegisterClassExW(&wc)) goto fail;
    wc.lpfnWndProc=editorLabelsProc;wc.lpszClassName=kClassEditorLabels;
    if(!RegisterClassExW(&wc)) goto fail;
    wc.lpfnWndProc=editorToolsProc;wc.lpszClassName=kClassEditorTools;
    if(!RegisterClassExW(&wc)) goto fail;
    wc.lpfnWndProc=tunerProc;wc.lpszClassName=kClassTuner;
    if(!RegisterClassExW(&wc)) goto fail;
    wc.lpfnWndProc=seamTunerProc;wc.lpszClassName=kClassSeamTuner;
    if(!RegisterClassExW(&wc)) goto fail;
    wc.lpfnWndProc=panelProc;wc.lpszClassName=kClassPanel;
    if(!RegisterClassExW(&wc)) goto fail;
    wc.lpfnWndProc=overlayProc;wc.lpszClassName=kClassOverlay;
    wc.hCursor=LoadCursorW((HINSTANCE)0,(const wchar_t*)(UINT_PTR)32512);
    if(!RegisterClassExW(&wc)) goto fail;
    g_main=CreateWindowExW(0,kClassMain,L"LiteZones",0,0,0,0,0,(HWND)0,(HMENU)0,g_instance,(void*)0);
    if(!g_main) goto fail;
    g_taskbarCreated=RegisterWindowMessageW(L"TaskbarCreated");
    initDisplays();
    updateTray();
    g_hook=SetWinEventHook(EVENT_SYSTEM_MOVESIZESTART,EVENT_SYSTEM_MOVESIZEEND,(HMODULE)0,eventHook,0,0,
                           WINEVENT_OUTOFCONTEXT|WINEVENT_SKIPOWNPROCESS);
    if(!g_hook) {
        MessageBoxW((HWND)0,lz_text(L"无法监听窗口移动事件。请检查 Windows 用户权限。"),L"LiteZones",MB_OK|MB_ICONERROR);
        DestroyWindow(g_main);return 2;
    }
    /* Track OS foreground activations rather than trying to infer z-order
       from positions. This hook does not poll or alter any application's UI. */
    g_focusHook=SetWinEventHook(EVENT_SYSTEM_FOREGROUND,EVENT_SYSTEM_FOREGROUND,(HMODULE)0,eventHook,0,0,
                                WINEVENT_OUTOFCONTEXT|WINEVENT_SKIPOWNPROCESS);
    if(!g_focusHook) {
        MessageBoxW((HWND)0,lz_text(L"无法监听窗口激活事件，分区顶层识别不可用。"),L"LiteZones",MB_OK|MB_ICONERROR);
        DestroyWindow(g_main);return 2;
    }
    /* Best-effort cleanup of destroyed HWNDs (Windows can recycle handles). */
    g_destroyHook=SetWinEventHook(EVENT_OBJECT_DESTROY,EVENT_OBJECT_DESTROY,(HMODULE)0,eventHook,0,0,
                                  WINEVENT_OUTOFCONTEXT|WINEVENT_SKIPOWNPROCESS);
    /* No modal welcome dialog or Windows notification beep on startup.
       Guidance is available from the tray menu when requested. */
    while(GetMessageW(&message,(HWND)0,0,0)>0) {
        /* Esc works even when a native text field owns keyboard focus. */
        if(g_editor && message.message==0x0100u && message.wParam==0x1Bu) {
            DestroyWindow(g_editor);continue;
        }
        TranslateMessage(&message);DispatchMessageW(&message);
    }
    if(g_singleton) CloseHandle(g_singleton);
    return 0;
fail:
    MessageBoxW((HWND)0,lz_text(L"启动失败：无法创建 LiteZones 系统窗口。"),L"LiteZones",MB_OK|MB_ICONERROR);
    if(g_singleton) CloseHandle(g_singleton);
    return 1;
}
void mainCRTStartup(void) {ExitProcess((UINT)appMain());}

