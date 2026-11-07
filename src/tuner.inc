/* Compact, ephemeral two-slider popover. No background settings window. */
#define WM_ACTIVATE 0x0006u
static HWND g_tuner=(HWND)0;
static int t_gap=0,t_opacity=108,t_drag=-1;
static LZRect tunerRail(int track) {
    int y=track==0?113:181;
    return (LZRect){29,y,301,y+5};
}
static void tunerPoint(int track,int x) {
    LZRect rail=tunerRail(track);
    int offset=x-rail.left;
    int length=rail.right-rail.left;
    if(offset<0)offset=0;if(offset>length)offset=length;
    if(track==0)t_gap=(offset*40+length/2)/length;
    else {
        t_opacity=(offset*255+length/2)/length;
        if(t_opacity!=g_overlayOpacity) {g_overlayOpacity=t_opacity;lz_applyOpacity();}
    }
    if(g_tuner)InvalidateRect(g_tuner,(RECT*)0,0);
}
static void tunerCommit(void) {
    if(g_gap!=t_gap) {
        g_gap=t_gap;setConfigNumber(L"General",L"Gap",g_gap);
        for(int i=0;i<g_displayCount;i++) {
            forgetMonitor(g_displays[i].device);
            refreshZones(&g_displays[i]);
        }
    }
    if(g_overlayOpacity!=t_opacity) {g_overlayOpacity=t_opacity;lz_applyOpacity();}
    setConfigNumber(L"Appearance",L"Opacity",g_overlayOpacity);
}
static void tunerPaint(HWND hwnd) {
    PAINTSTRUCT ps;HDC dc=BeginPaint(hwnd,&ps);
    DWORD accent=lz_accent(),pale=lz_mix(accent,COLORREF_(255,255,255),89);
    DWORD dark=COLORREF_(57,52,68),muted=COLORREF_(120,114,133);
    RECT all={0,0,391,244};
    HBRUSH white=CreateSolidBrush(COLORREF_(255,253,254));
    FillRect(dc,&all,white);DeleteObject(white);
    drawBox(dc,(LZRect){0,0,390,243},COLORREF_(255,253,254),lz_mix(accent,COLORREF_(255,255,255),72));
    drawLabelFont(dc,L"细节调节",(LZRect){28,18,300,54},dark,g_titleFont,0);
    drawLabelFont(dc,L"×",(LZRect){344,19,372,50},muted,g_titleFont,DT_CENTER);
    drawLabelFont(dc,L"窗口间距",(LZRect){29,70,220,105},dark,g_uiFont,0);
    drawLabelFont(dc,L"预览透明度",(LZRect){29,138,240,173},dark,g_uiFont,0);
    for(int i=0;i<2;i++) {
        int v=i==0?t_gap:t_opacity;
        int maximum=i==0?40:255;
        LZRect rail=tunerRail(i);
        int thumb=rail.left+(v*(rail.right-rail.left)+maximum/2)/maximum;
        drawBox(dc,(LZRect){rail.left,rail.top-2,rail.right,rail.bottom+2},pale,pale);
        drawBox(dc,(LZRect){rail.left,rail.top-2,thumb+1,rail.bottom+2},accent,accent);
        drawBox(dc,(LZRect){thumb-8,rail.top-7,thumb+8,rail.bottom+7},COLORREF_(255,255,255),accent);
        wchar_t value[24]=L"";
        appendInt(value,24,i==0?t_gap:(t_opacity*100+127)/255);
        appendW(value,24,i==0?L" px":L"%");
        drawLabelFont(dc,value,(LZRect){310,rail.top-14,383,rail.bottom+15},dark,g_uiFont,DT_CENTER);
    }
    drawLabelFont(dc,L"拖动即可调节  ·  按像素及 1% 精度保存",(LZRect){29,203,377,237},muted,g_uiFont,0);
    EndPaint(hwnd,&ps);
}
static LRESULT WINCALL tunerProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp) {
    if(msg==WM_ERASEBKGND)return 1;
    if(msg==WM_PAINT){tunerPaint(hwnd);return 0;}
    if(msg==WM_ACTIVATE && (wp&65535u)==0) {tunerCommit();DestroyWindow(hwnd);return 0;}
    if(msg==WM_KEYDOWN && wp==VK_ESCAPE){tunerCommit();DestroyWindow(hwnd);return 0;}
    if(msg==WM_LBUTTONDOWN) {
        int x=(short)(lp&65535),y=(short)((lp>>16)&65535);
        if(x>=333&&y<64){tunerCommit();DestroyWindow(hwnd);return 0;}
        if(y>=92&&y<=134)t_drag=0;
        else if(y>=160&&y<=201)t_drag=1;
        if(t_drag>=0){SetCapture(hwnd);tunerPoint(t_drag,x);}
        return 0;
    }
    if(msg==WM_MOUSEMOVE && t_drag>=0){tunerPoint(t_drag,(short)(lp&65535));return 0;}
    if(msg==WM_LBUTTONUP && t_drag>=0){ReleaseCapture();t_drag=-1;tunerCommit();return 0;}
    if(msg==WM_CAPTURECHANGED){t_drag=-1;return 0;}
    if(msg==WM_CLOSE){tunerCommit();DestroyWindow(hwnd);return 0;}
    if(msg==WM_DESTROY){g_tuner=(HWND)0;return 0;}
    return DefWindowProcW(hwnd,msg,wp,lp);
}
static void openTuner(void) {
    if(g_tuner){SetForegroundWindow(g_tuner);return;}
    t_gap=g_gap;t_opacity=g_overlayOpacity;t_drag=-1;
    POINT p={0,0};GetCursorPos(&p);
    int mon=monitorUnderCursor(p),x=p.x-180,y=p.y-115;
    if(mon>=0) {
        RECT work=g_displays[mon].work;
        if(x<work.left)x=work.left;if(x+391>work.right)x=work.right-391;
        if(y<work.top)y=work.top;if(y+244>work.bottom)y=work.bottom-244;
    }
    g_tuner=CreateWindowExW(WS_EX_TOPMOST|WS_EX_TOOLWINDOW,kClassTuner,lz_text(L"LiteZones · 细节调节"),
        WS_POPUP,x,y,391,244,(HWND)0,(HMENU)0,g_instance,(void*)0);
    if(g_tuner){ShowWindow(g_tuner,SW_SHOW);SetForegroundWindow(g_tuner);}
}
