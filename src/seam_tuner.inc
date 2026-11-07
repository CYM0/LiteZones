/* Per-edge, opt-in visible frame expansion (0..12px). No system hooks added. */
static const wchar_t kClassSeamTuner[]=L"LiteZonesSeamTuner";
static HWND g_seamTuner=(HWND)0;
static int st_values[4]={0,0,0,0},st_drag=-1;
static const wchar_t *st_names[4]={L"左边扩展",L"右边扩展",L"上边扩展",L"下边扩展"};
static LZRect st_rail(int index){int y=119+index*54;return (LZRect){31,y,299,y+5};}
static void st_store(void) {
    g_bleedLeft=st_values[0];g_bleedRight=st_values[1];
    g_bleedTop=st_values[2];g_bleedBottom=st_values[3];
    setConfigNumber(L"Seam",L"Left",g_bleedLeft);
    setConfigNumber(L"Seam",L"Right",g_bleedRight);
    setConfigNumber(L"Seam",L"Top",g_bleedTop);
    setConfigNumber(L"Seam",L"Bottom",g_bleedBottom);
    reapplyManagedZones();
}
static void st_setPoint(int index,int x) {
    LZRect rail=st_rail(index);
    int delta=x-rail.left, width=rail.right-rail.left;
    if(delta<0)delta=0;if(delta>width)delta=width;
    st_values[index]=(delta*12+width/2)/width;
    InvalidateRect(g_seamTuner,(const RECT*)0,0);
}
static void st_paint(HWND hwnd) {
    PAINTSTRUCT ps;HDC dc=BeginPaint(hwnd,&ps);
    DWORD accent=lz_accent(),ink=COLORREF_(51,45,60),muted=COLORREF_(126,113,130);
    HBRUSH white=CreateSolidBrush(COLORREF_(255,251,253));
    RECT all={0,0,390,359};FillRect(dc,&all,white);DeleteObject(white);
    drawBox(dc,(LZRect){1,1,389,358},COLORREF_(255,251,253),lz_mix(accent,COLORREF_(255,255,255),78));
    drawLabelFont(dc,L"无间隙 · 边界微调",(LZRect){26,12,322,49},ink,g_titleFont,0);
    drawLabelFont(dc,L"×",(LZRect){349,11,376,49},muted,g_titleFont,DT_CENTER);
    drawLabelFont(dc,L"仅影响吸附后的可见窗口边缘",(LZRect){28,52,367,78},muted,g_uiFont,0);
    for(int i=0;i<4;i++) {
        int y=87+i*54;LZRect track=st_rail(i);
        drawLabelFont(dc,st_names[i],(LZRect){30,y,265,y+27},ink,g_uiFont,0);
        int pos=track.left+(st_values[i]*(track.right-track.left)+6)/12;
        DWORD pale=lz_mix(accent,COLORREF_(255,255,255),88);
        drawBox(dc,(LZRect){track.left,track.top-2,track.right,track.bottom+2},pale,pale);
        drawBox(dc,(LZRect){track.left,track.top-2,pos+1,track.bottom+2},accent,accent);
        drawBox(dc,(LZRect){pos-7,track.top-7,pos+7,track.bottom+7},COLORREF_(255,255,255),accent);
        wchar_t value[22]=L"";appendInt(value,22,st_values[i]);appendW(value,22,L" px");
        drawLabelFont(dc,value,(LZRect){315,track.top-13,379,track.bottom+15},ink,g_uiFont,DT_CENTER);
    }
    drawLabelFont(dc,L"建议先试左右 1–3 px；过大可能遮盖内容",(LZRect){25,315,374,349},muted,g_uiFont,DT_CENTER);
    EndPaint(hwnd,&ps);
}
static LRESULT WINCALL seamTunerProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp) {
    if(msg==WM_ERASEBKGND)return 1;
    if(msg==WM_PAINT){st_paint(hwnd);return 0;}
    if(msg==WM_ACTIVATE && (wp&65535u)==0){st_store();DestroyWindow(hwnd);return 0;}
    if(msg==WM_KEYDOWN&&wp==VK_ESCAPE){st_store();DestroyWindow(hwnd);return 0;}
    if(msg==WM_LBUTTONDOWN) {
        int x=(short)(lp&65535),y=(short)((lp>>16)&65535);
        if(y<55&&x>=339){st_store();DestroyWindow(hwnd);return 0;}
        for(int i=0;i<4;i++)if(y>=st_rail(i).top-19&&y<=st_rail(i).bottom+21){
            st_drag=i;SetCapture(hwnd);st_setPoint(i,x);return 0;
        }
    }
    if(msg==WM_MOUSEMOVE&&st_drag>=0){st_setPoint(st_drag,(short)(lp&65535));return 0;}
    if(msg==WM_LBUTTONUP&&st_drag>=0){ReleaseCapture();st_drag=-1;st_store();return 0;}
    if(msg==WM_CAPTURECHANGED){st_drag=-1;return 0;}
    if(msg==WM_CLOSE){st_store();DestroyWindow(hwnd);return 0;}
    if(msg==WM_DESTROY){g_seamTuner=(HWND)0;return 0;}
    return DefWindowProcW(hwnd,msg,wp,lp);
}
static void openSeamTuner(void) {
    if(g_seamTuner){SetForegroundWindow(g_seamTuner);return;}
    st_values[0]=g_bleedLeft;st_values[1]=g_bleedRight;
    st_values[2]=g_bleedTop;st_values[3]=g_bleedBottom;st_drag=-1;
    POINT p={0,0};GetCursorPos(&p);
    int monitor=monitorUnderCursor(p),x=p.x-190,y=p.y-180;
    if(monitor>=0) {
        RECT r=g_displays[monitor].work;
        if(x<r.left)x=r.left;if(x+390>r.right)x=r.right-390;
        if(y<r.top)y=r.top;if(y+359>r.bottom)y=r.bottom-359;
    }
    g_seamTuner=CreateWindowExW(WS_EX_TOPMOST|WS_EX_TOOLWINDOW,kClassSeamTuner,
        lz_text(L"LiteZones · 边界补偿"),WS_POPUP,x,y,390,359,(HWND)0,(HMENU)0,g_instance,(void*)0);
    if(g_seamTuner){ShowWindow(g_seamTuner,SW_SHOW);SetForegroundWindow(g_seamTuner);}
}
