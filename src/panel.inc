/* One-HWND hover flyout menu. No background UI or hook. */
#include "panel_geometry.h"
#define LZ_PANEL_WIDTH 257
#define LZ_PANEL_ROW 25
#define LZ_PANEL_HEAD 51
#define LZ_PANEL_BOTTOM 8
#define LZ_PANEL_CANVAS_W 864
#define LZ_PANEL_CANVAS_H 560
#define LZ_PANEL_ROW_MAX 18
#define LZ_PANEL_HOME 0
#define LZ_PANEL_MONITOR 1
#define LZ_PANEL_BUILTINS 2
#define LZ_PANEL_PROFILES 3
#define LZ_PANEL_COLORS 4
#define LZ_PANEL_TUNING 5
#define LZ_PANEL_SEAMS 6
#define LZ_VIEW_MONITOR_BASE 50000
#define LZ_VIEW_BUILTINS 50020
#define LZ_VIEW_PROFILES 50021
#define LZ_VIEW_COLORS 50022
#define LZ_VIEW_TUNING 50023
#define LZ_VIEW_SEAMS 50024
#define LZ_VIEW_EDITOR 50026
#define LZ_VIEW_MORE 50025
#define LZ_VIEW_SHOW_TUNER 50028
#define LZ_VIEW_SHOW_SEAM 50029
static const wchar_t kClassPanel[]=L"LiteZonesMicroPanel";
typedef struct {wchar_t label[112];int action,checked;} PanelRow;
typedef struct {
    HWND hwnd;
    int page,monitor,slotPage,count,hover,width,height,anchor,active,x,y;
    wchar_t title[80],subtitle[110];
    PanelRow rows[LZ_PANEL_ROW_MAX];
} PanelFace;
static PanelFace p_faces[3];
static HWND p_hwnd=(HWND)0;
/* A SINGLE native popup contains all flyout columns. Clicking a submenu can
   never deactivate an independent submenu HWND (there are no child HWNDs).
   Colored regions are hit-testable; transparent canvas is click-through. */
static int p_direction=-1, p_root_x=0,p_root_y=0;
static HGDIOBJ g_menuFont=(HGDIOBJ)0;
static void panelAdd(PanelFace *p,const wchar_t *name,int action,int checked) {
    if(p->count>=LZ_PANEL_ROW_MAX)return;
    PanelRow *r=&p->rows[p->count++];
    copyW(r->label,112,lz_text(name));r->action=action;r->checked=checked;
}
static int panelChildPage(int action) {
    if(action>=LZ_VIEW_MONITOR_BASE&&action<LZ_VIEW_MONITOR_BASE+MAX_MONITORS)return LZ_PANEL_MONITOR;
    if(action==LZ_VIEW_BUILTINS)return LZ_PANEL_BUILTINS;
    if(action==LZ_VIEW_PROFILES)return LZ_PANEL_PROFILES;
    if(action==LZ_VIEW_COLORS)return LZ_PANEL_COLORS;
    if(action==LZ_VIEW_TUNING)return LZ_PANEL_TUNING;
    if(action==LZ_VIEW_SEAMS)return LZ_PANEL_SEAMS;
    return -1;
}
static void panelBuild(PanelFace *p) {
    p->count=0;
    if(p->page==LZ_PANEL_HOME) {
        copyW(p->title,80,L"LiteZones · 轻格");
        copyW(p->subtitle,110,L"Shift + 拖动  ·  快速布局");
        panelAdd(p,L"启用窗口吸附",CMD_ENABLE,g_enabled);
        for(int i=0;i<g_displayCount;i++) {
            wchar_t line[112];copyW(line,112,lz_text(L"显示器 "));appendInt(line,112,i+1);
            appendW(line,112,L"  ·  ");appendInt(line,112,g_displays[i].screen.right-g_displays[i].screen.left);
            appendW(line,112,L" × ");appendInt(line,112,g_displays[i].screen.bottom-g_displays[i].screen.top);
            panelAdd(p,line,LZ_VIEW_MONITOR_BASE+i,0);
        }
        panelAdd(p,L"主题颜色",LZ_VIEW_COLORS,0);
        panelAdd(p,L"间距 / 透明度",LZ_VIEW_TUNING,0);
        panelAdd(p,L"真无间隙 · 边界补偿",LZ_VIEW_SEAMS,g_seamEnabled);
        panelAdd(p,L"拖出后恢复尺寸",CMD_RESTORE,g_restoreOnDetach);
        panelAdd(p,L"开机自动启动",CMD_STARTUP,startupEnabled());
        panelAdd(p,L"使用说明",CMD_HELP,0);
        panelAdd(p,L"切换语言",CMD_LANGUAGE,0);
        panelAdd(p,L"退出 LiteZones",CMD_EXIT,0);
    } else if(p->page==LZ_PANEL_MONITOR) {
        copyW(p->title,80,L"窗口布局");
        copyW(p->subtitle,110,L"选择预设或编辑自己的方案");
        panelAdd(p,L"系统预设",LZ_VIEW_BUILTINS,0);
        panelAdd(p,L"我的自定义布局",LZ_VIEW_PROFILES,0);
        panelAdd(p,L"编辑当前布局…",LZ_VIEW_EDITOR,0);
    } else if(p->page==LZ_PANEL_BUILTINS) {
        copyW(p->title,80,L"系统预设");
        copyW(p->subtitle,110,L"当前显示器独立保存");
        for(int i=0;i<LZ_BUILTIN_COUNT;i++)
            panelAdd(p,kLayouts[i],CMD_LAYOUT_BASE+p->monitor*10+i,g_displays[p->monitor].layout==i);
    } else if(p->page==LZ_PANEL_PROFILES) {
        copyW(p->title,80,L"自定义布局");
        copyW(p->subtitle,110,L"每台显示器保留 12 套");
        for(int i=p->slotPage*6;i<p->slotPage*6+6&&i<LZ_PROFILE_COUNT;i++) {
            wchar_t label[112]=L"";appendInt(label,112,i+1);
            appendW(label,112,L"  ·  ");appendW(label,112,g_displays[p->monitor].profileNames[i]);
            panelAdd(p,label,CMD_PROFILE_BASE+p->monitor*LZ_PROFILE_COUNT+i,
                g_displays[p->monitor].layout==LZ_LAYOUT_CUSTOM&&g_displays[p->monitor].activeProfile==i);
        }
        panelAdd(p,p->slotPage?L"‹  显示 1–6":L"显示 7–12  ›",LZ_VIEW_MORE,0);
        panelAdd(p,L"编辑布局…",LZ_VIEW_EDITOR,0);
    } else if(p->page==LZ_PANEL_COLORS) {
        copyW(p->title,80,L"主题颜色");
        copyW(p->subtitle,110,L"预览和菜单共用主题色");
        const wchar_t *names[]={L"跟随系统",L"樱粉",L"经典蓝",L"淡紫",L"薄荷绿",L"天空蓝"};
        const int modes[]={LZ_THEME_SYSTEM,LZ_THEME_CHERRY,LZ_THEME_CLASSIC,LZ_THEME_LAVENDER,LZ_THEME_MINT,LZ_THEME_SKY};
        for(int i=0;i<6;i++)panelAdd(p,names[i],CMD_THEME_BASE+modes[i],g_themeMode==modes[i]);
        panelAdd(p,L"自定义颜色…",CMD_THEME_CUSTOM_PICK,g_themeMode==LZ_THEME_CUSTOM);
    } else if(p->page==LZ_PANEL_TUNING) {
        copyW(p->title,80,L"间距与透明度");
        copyW(p->subtitle,110,L"无级滑动调节");
        wchar_t line[112];copyW(line,112,lz_text(L"窗口间距  ·  "));appendInt(line,112,g_gap);appendW(line,112,L" px");
        panelAdd(p,line,LZ_VIEW_SHOW_TUNER,0);
        copyW(line,112,lz_text(L"预览透明度  ·  "));appendInt(line,112,(g_overlayOpacity*100+127)/255);
        appendW(line,112,L"%");panelAdd(p,line,LZ_VIEW_SHOW_TUNER,0);
        panelAdd(p,L"打开精细调节…",LZ_VIEW_SHOW_TUNER,0);
    } else if(p->page==LZ_PANEL_SEAMS) {
        copyW(p->title,80,L"真无间隙 · 边界补偿");
        copyW(p->subtitle,110,L"以像素扩大窗口可见边界");
        panelAdd(p,L"启用边界补偿",CMD_SEAM_ENABLE,g_seamEnabled);
        wchar_t line[112];copyW(line,112,lz_text(L"左 "));appendInt(line,112,g_bleedLeft);
        appendW(line,112,lz_text(L"  ·  右 "));appendInt(line,112,g_bleedRight);
        appendW(line,112,lz_text(L"  ·  上 "));appendInt(line,112,g_bleedTop);
        appendW(line,112,lz_text(L"  ·  下 "));appendInt(line,112,g_bleedBottom);
        appendW(line,112,L" px");panelAdd(p,line,LZ_VIEW_SHOW_SEAM,0);
    }
    p->height=LZ_PANEL_HEAD+p->count*LZ_PANEL_ROW+LZ_PANEL_BOTTOM;
    p->width=(p->page==LZ_PANEL_PROFILES)?285:LZ_PANEL_WIDTH;
}
static void panelPositionFaces(void);
static void panelRefreshLanguage(void) {
    if(!p_hwnd)return;
    for(int d=1;d<3;d++){p_faces[d].active=0;p_faces[d].hover=-1;}
    p_faces[0].hover=-1;
    panelBuild(&p_faces[0]);
    panelPositionFaces();
    InvalidateRect(p_hwnd,(const RECT*)0,0);
}
static void panelDrawFace(HDC dc,PanelFace *p) {
    DWORD accent=lz_accent(),ink=COLORREF_(58,47,64),muted=COLORREF_(133,116,130);
    RECT all={0,0,p->width,p->height};
    HBRUSH blank=CreateSolidBrush(COLORREF_(255,0,255));FillRect(dc,&all,blank);DeleteObject(blank);
    drawBox(dc,(LZRect){1,1,p->width-1,p->height-1},COLORREF_(255,251,253),
        lz_mix(accent,COLORREF_(255,255,255),72));
    /* Same visual as EXE icon: narrow - wide - narrow, never a Windows-style 2x2. */
    drawBox(dc,(LZRect){10,10,42,42},accent,accent);
    HBRUSH b=CreateSolidBrush(COLORREF_(255,255,255));
    RECT r={15,18,20,34};FillRect(dc,&r,b);
    r=(RECT){21,18,31,34};FillRect(dc,&r,b);
    r=(RECT){32,18,37,34};FillRect(dc,&r,b);DeleteObject(b);
    drawLabelFont(dc,p->title,(LZRect){51,5,p->width-9,27},ink,g_uiFont,0);
    drawLabelFont(dc,p->subtitle,(LZRect){51,25,p->width-8,45},muted,g_menuFont,0);
    HBRUSH sep=CreateSolidBrush(lz_mix(accent,COLORREF_(255,255,255),84));
    r=(RECT){14,49,p->width-14,50};FillRect(dc,&r,sep);DeleteObject(sep);
    for(int i=0;i<p->count;i++) {
        int y=LZ_PANEL_HEAD+i*LZ_PANEL_ROW;
        PanelRow *line=&p->rows[i];
        if(p->hover==i) {
            drawBox(dc,(LZRect){6,y,p->width-6,y+LZ_PANEL_ROW},
                lz_mix(accent,COLORREF_(255,255,255),84),
                lz_mix(accent,COLORREF_(255,255,255),84));
        }
        if(line->checked) {
            drawBox(dc,(LZRect){12,y+5,27,y+20},accent,accent);
            drawLabelFont(dc,L"✓",(LZRect){12,y+5,27,y+20},COLORREF_(255,255,255),g_menuFont,DT_CENTER);
        } else {
            HBRUSH dot=CreateSolidBrush(lz_mix(accent,COLORREF_(255,255,255),55));
            r=(RECT){19,y+12,22,y+15};FillRect(dc,&r,dot);DeleteObject(dot);
        }
        drawLabelFont(dc,line->label,(LZRect){34,y,p->width-22,y+LZ_PANEL_ROW},
            line->action==CMD_EXIT?COLORREF_(181,85,114):ink,g_menuFont,0);
        if(panelChildPage(line->action)>=0) {
            drawLabelFont(dc,L"›",(LZRect){p->width-25,y,p->width-8,y+LZ_PANEL_ROW},
                accent,g_uiFont,DT_CENTER);
        }
    }
}
static int panelMax(int a,int b){return a>b?a:b;}
static int panelMin(int a,int b){return a<b?a:b;}
static void panelPositionFaces(void) {
    PanelFace *root=&p_faces[0];root->x=p_root_x;root->y=p_root_y;
    for(int d=1;d<3;d++) {
        PanelFace *c=&p_faces[d],*parent=&p_faces[d-1];
        if(!c->active||!parent->active)continue;
        c->x=parent->x+(p_direction>0?parent->width:-c->width);
        c->y=parent->y+LZ_PANEL_HEAD+parent->hover*LZ_PANEL_ROW-13;
        c->y=panelMax(0,panelMin(LZ_PANEL_CANVAS_H-c->height,c->y));
    }
}
static void panelPaintFace(HWND hwnd) {
    PAINTSTRUCT ps;HDC dc=BeginPaint(hwnd,&ps);
    HDC mem=CreateCompatibleDC(dc);
    HGDIOBJ bmp=mem?CreateCompatibleBitmap(dc,LZ_PANEL_CANVAS_W,LZ_PANEL_CANVAS_H):(HGDIOBJ)0;
    HDC target=(mem&&bmp)?mem:dc;
    HGDIOBJ prior=(HGDIOBJ)0;
    if(target==mem)prior=SelectObject(mem,bmp);
    /* Color-key alpha only outside menu shapes: no giant invisible hit area. */
    RECT all={0,0,LZ_PANEL_CANVAS_W,LZ_PANEL_CANVAS_H};
    HBRUSH bg=CreateSolidBrush(COLORREF_(255,0,255));FillRect(target,&all,bg);DeleteObject(bg);
    for(int d=0;d<3;d++)if(p_faces[d].active) {
        PanelFace *p=&p_faces[d];
        POINT previous={0,0};
        SetViewportOrgEx(target,p->x,p->y,&previous);
        panelDrawFace(target,p);
        SetViewportOrgEx(target,previous.x,previous.y,(POINT*)0);
    }
    if(target==mem) {
        BitBlt(dc,0,0,LZ_PANEL_CANVAS_W,LZ_PANEL_CANVAS_H,mem,0,0,SRCCOPY);
        SelectObject(mem,prior);DeleteObject(bmp);DeleteDC(mem);
    } else {if(mem)DeleteDC(mem);if(bmp)DeleteObject(bmp);}
    EndPaint(hwnd,&ps);
}
static void panelCloseFrom(int depth) {
    for(int d=depth;d<3;d++) {
        p_faces[d].active=0;
        p_faces[d].hover=-1;
    }
    if(p_hwnd)InvalidateRect(p_hwnd,(const RECT*)0,0);
}
static void panelCloseAll(void) {
    HWND previous=p_hwnd;
    p_hwnd=(HWND)0;
    for(int d=0;d<3;d++)p_faces[d].active=0;
    if(previous)DestroyWindow(previous);
}
static void panelShowChild(int parentDepth,int action) {
    int page=panelChildPage(action);
    if(parentDepth>=2||page<0) {panelCloseFrom(parentDepth+1);return;}
    PanelFace *parent=&p_faces[parentDepth],*child=&p_faces[parentDepth+1];
    int mon=parent->monitor;
    if(action>=LZ_VIEW_MONITOR_BASE&&action<LZ_VIEW_MONITOR_BASE+MAX_MONITORS)
        mon=action-LZ_VIEW_MONITOR_BASE;
    if(child->active&&child->anchor==action&&child->monitor==mon&&child->page==page)return;
    panelCloseFrom(parentDepth+1);
    if(page==LZ_PANEL_MONITOR&&(mon<0||mon>=g_displayCount))return;
    memset(child,0,sizeof(*child));
    child->page=page;child->monitor=mon;child->active=1;
    child->slotPage=0;child->hover=-1;child->anchor=action;
    panelBuild(child);panelPositionFaces();
    if(p_hwnd)InvalidateRect(p_hwnd,(const RECT*)0,0);
}
static void panelDoAction(int depth,int action) {
    PanelFace *p=&p_faces[depth];
    if(action==CMD_LANGUAGE){menuAction((UINT)action);return;}
    if(panelChildPage(action)>=0){panelShowChild(depth,action);return;}
    if(action==LZ_VIEW_MORE) {
        p->slotPage=1-p->slotPage;panelBuild(p);
        panelPositionFaces();
        InvalidateRect(p_hwnd,(RECT*)0,0);return;
    }
    if(action==LZ_VIEW_EDITOR)action=CMD_EDITOR_BASE+p->monitor;
    if(action==LZ_VIEW_SHOW_TUNER)action=CMD_FINE_TUNE;
    if(action==LZ_VIEW_SHOW_SEAM)action=CMD_SEAM_TUNE;
    /* First destroy the popup so focus and mouse capture cannot interfere
       with the editor, color picker or slider window opened by menuAction. */
    panelCloseAll();
    menuAction((UINT)action);
}
static int panelHit(int x,int y,int *rowOut) {
    LZPanelHitRect bounds[3];
    for(int d=0;d<3;d++) {
        PanelFace *p=&p_faces[d];
        bounds[d]=(LZPanelHitRect){p->active,p->x,p->y,p->width,p->height,
                                  LZ_PANEL_HEAD,LZ_PANEL_ROW,p->count};
    }
    return lz_panel_hit(bounds,3,x,y,rowOut);
}
static LRESULT WINCALL panelProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp) {
    if(hwnd!=p_hwnd)return DefWindowProcW(hwnd,msg,wp,lp);
    if(msg==WM_ERASEBKGND)return 1;
    if(msg==WM_PAINT){panelPaintFace(hwnd);return 0;}
    if(msg==WM_ACTIVATE&&(wp&65535u)==0){panelCloseAll();return 0;}
    if(msg==WM_KEYDOWN) {
        if(wp==VK_ESCAPE){panelCloseAll();return 0;}
        /* Keyboard navigation keeps the currently deepest flyout active. */
        int d=p_faces[2].active?2:p_faces[1].active?1:0;
        PanelFace *p=&p_faces[d];
        if(wp==0x26u||wp==0x28u) {
            p->hover=(p->hover+(wp==0x28u?1:-1)+p->count)%p->count;
            panelShowChild(d,p->rows[p->hover].action);
            panelPositionFaces();InvalidateRect(hwnd,(RECT*)0,0);return 0;
        }
        if((wp==0x27u||wp==VK_RETURN)&&p->hover>=0&&p->hover<p->count) {
            panelDoAction(d,p->rows[p->hover].action);return 0;
        }
        if(wp==0x25u&&d>0){panelCloseFrom(d);return 0;}
    }
    if(msg==WM_MOUSEMOVE) {
        int x=(short)(lp&65535),y=(short)((lp>>16)&65535),row=-1;
        int depth=panelHit(x,y,&row);
        if(depth>=0) {
            PanelFace *p=&p_faces[depth];
            if(row!=p->hover) {
                p->hover=row;
                panelShowChild(depth,row>=0?p->rows[row].action:-1);
                panelPositionFaces();InvalidateRect(hwnd,(RECT*)0,0);
            }
        }
        return 0;
    }
    if(msg==WM_LBUTTONUP) {
        int x=(short)(lp&65535),y=(short)((lp>>16)&65535),row=-1;
        int depth=panelHit(x,y,&row);
        if(depth>=0&&row>=0)panelDoAction(depth,p_faces[depth].rows[row].action);
        return 0;
    }
    if(msg==WM_CLOSE){panelCloseAll();return 0;}
    if(msg==WM_DESTROY){p_hwnd=(HWND)0;return 0;}
    return DefWindowProcW(hwnd,msg,wp,lp);
}
static void showTrayMenu(void) {
    if(p_hwnd){panelCloseAll();return;}
    memset(p_faces,0,sizeof(p_faces));
    PanelFace *p=&p_faces[0];p->page=LZ_PANEL_HOME;p->hover=-1;p->active=1;panelBuild(p);
    POINT mouse={0,0};GetCursorPos(&mouse);
    int mon=monitorUnderCursor(mouse);
    RECT work={0,0,1920,1080};
    if(mon>=0)work=g_displays[mon].work;
    int rootX=mouse.x-205,rootY=mouse.y-p->height-8;
    rootX=panelMax(work.left,panelMin(work.right-p->width,rootX));
    rootY=panelMax(work.top,panelMin(work.bottom-p->height,rootY));
    p_direction=(rootX+LZ_PANEL_CANVAS_W<=work.right)?1:-1;
    /* Canvas is fixed while open: submenu hover cannot activate or move a
       separate popup HWND. Position the visual root at the cursor anchor. */
    int canvasX=p_direction>0?rootX:rootX-(LZ_PANEL_CANVAS_W-p->width);
    canvasX=panelMax(work.left,panelMin(work.right-LZ_PANEL_CANVAS_W,canvasX));
    int canvasY=rootY-70;
    canvasY=panelMax(work.top,panelMin(work.bottom-LZ_PANEL_CANVAS_H,canvasY));
    p_root_x=rootX-canvasX;p_root_y=rootY-canvasY;
    panelPositionFaces();
    /* The popup is one layered native HWND; unpainted magenta pixels do not
       intercept pointer hits, so only visible menu rectangles are clickable. */
    p_hwnd=CreateWindowExW(WS_EX_TOPMOST|WS_EX_TOOLWINDOW|WS_EX_LAYERED,
        kClassPanel,lz_text(L"LiteZones · 轻格"),WS_POPUP,canvasX,canvasY,
        LZ_PANEL_CANVAS_W,LZ_PANEL_CANVAS_H,
        (HWND)0,(HMENU)0,g_instance,(void*)0);
    if(p_hwnd) {
        SetLayeredWindowAttributes(p_hwnd,COLORREF_(255,0,255),255,LWA_COLORKEY);
        ShowWindow(p_hwnd,SW_SHOW);SetForegroundWindow(p_hwnd);
    }
}
