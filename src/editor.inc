/* Lightweight Win32 profile editor.  Back-buffered painting prevents flicker;
   the preview is proportional to the monitor's actual work-area resolution. */
static int e_monitor=-1,e_profile=0,e_selected=0,e_divider=-1,e_hoverDivider=-1;
static LZRect e_dragParent;
static LZCustom e_layout;
static int e_w=1070,e_h=740;
static int e_toolbarX=0,e_toolbarW=0;
static HWND e_nameEdit=(HWND)0,e_widthEdit=(HWND)0,e_heightEdit=(HWND)0;
static HWND g_editorTools=(HWND)0;
/* Independent opaque foreground for zone size labels. The underlying editor
   retains its adjustable opacity; this ephemeral mouse-transparent overlay
   keeps resolution numbers fully opaque and readable at any opacity level. */
static HWND g_editorLabels=(HWND)0;
static const wchar_t kClassEditorLabels[]=L"LiteZonesEditorZoneLabels";
static HGDIOBJ e_zoneNameFont=(HGDIOBJ)0,e_zoneSizeFont=(HGDIOBJ)0;
static const wchar_t kClassEditorTools[]=L"LiteZonesEditorFloatingTools";
#define LZ_EDITOR_TOOLS_Y 22
#define LZ_EDITOR_TOOLS_H 233
static int e_pendingPaint=0;
static wchar_t e_notice[160]=L"选择一个区域，然后拆分；拖动粉色手柄可以修改宽度和高度。";
static HDC e_bufferDC=(HDC)0;
static HGDIOBJ e_bufferBitmap=(HGDIOBJ)0,e_bufferOld=(HGDIOBJ)0;
static int e_bufferW=0,e_bufferH=0;
static HGDIOBJ e_smallFont=(HGDIOBJ)0;
static void editorInvalidate(void);
#define WM_ERASEBKGND 0x0014u
#define WM_SETFONT 0x0030u
#define WS_VISIBLE 0x10000000u
#define WS_CLIPCHILDREN 0x02000000u
#define WS_BORDER 0x00800000u
#define ES_AUTOHSCROLL 0x0080u
#define ES_NUMBER 0x2000u
#define WM_KEYDOWN 0x0100u
#define VK_RETURN 0x0Du
#define SRCCOPY 0x00CC0020u

static void editorProfileSection(wchar_t *dst,int cap,const Display *d,int slot) {
    copyW(dst,cap,L"Custom_");appendW(dst,cap,d->configKey);
    appendW(dst,cap,L"_");appendInt(dst,cap,slot);
}
static void editorNamesSection(wchar_t *dst,int cap,const Display *d) {
    copyW(dst,cap,L"Profiles_");appendW(dst,cap,d->configKey);
}
/* Store Unicode names as UTF-16 code units encoded in ASCII hex. Windows INI
   files may be ANSI on older Windows configurations; this avoids replacing
   CJK user-defined profile names with '?' on non-Chinese system locales. */
static void profileHexEncode(const wchar_t *name,wchar_t out[300]) {
    static const wchar_t digits[]=L"0123456789ABCDEF";
    int n=lengthW(name);if(n>63)n=63;
    for(int i=0;i<n;i++) {
        unsigned int v=(unsigned short)name[i];
        out[i*4]=digits[(v>>12)&15];out[i*4+1]=digits[(v>>8)&15];
        out[i*4+2]=digits[(v>>4)&15];out[i*4+3]=digits[v&15];
    }
    out[n*4]=0;
}
static int profileHexDigit(wchar_t ch) {
    if(ch>=L'0'&&ch<=L'9')return (int)(ch-L'0');
    if(ch>=L'A'&&ch<=L'F')return (int)(ch-L'A'+10);
    if(ch>=L'a'&&ch<=L'f')return (int)(ch-L'a'+10);
    return -1;
}
static int profileHexDecode(const wchar_t *hex,wchar_t out[64]) {
    int n=lengthW(hex);
    if(n<4||n>252||(n%4))return 0;
    for(int i=0;i<n/4;i++) {
        unsigned int v=0;
        for(int j=0;j<4;j++) {
            int d=profileHexDigit(hex[i*4+j]);
            if(d<0)return 0;
            v=(v<<4)|(unsigned int)d;
        }
        if(v==0)return 0;
        out[i]=(wchar_t)v;
    }
    out[n/4]=0;
    return 1;
}
static void saveProfile(Display *d,int slot,const LZCustom *layout,const wchar_t *name) {
    wchar_t section[80],names[80],key[32];
    editorProfileSection(section,80,d,slot);
    setConfigNumber(section,L"Nodes",layout->count);
    setConfigNumber(section,L"Precision",2);
    for(int i=0;i<layout->count;i++) {
        copyW(key,32,L"N");appendInt(key,32,i);
        int base=lengthW(key);
        copyW(key+base,32-base,L"Split");setConfigNumber(section,key,layout->nodes[i].split);
        copyW(key+base,32-base,L"Ratio");setConfigNumber(section,key,layout->nodes[i].ratio);
        copyW(key+base,32-base,L"Left");setConfigNumber(section,key,layout->nodes[i].left);
        copyW(key+base,32-base,L"Right");setConfigNumber(section,key,layout->nodes[i].right);
    }
    editorNamesSection(names,80,d);
    copyW(key,32,L"Name");appendInt(key,32,slot);
    WritePrivateProfileStringW(names,key,name,g_configPath);
    copyW(key,32,L"HexName");appendInt(key,32,slot);
    wchar_t asciiEncoded[300];profileHexEncode(name,asciiEncoded);
    WritePrivateProfileStringW(names,key,asciiEncoded,g_configPath);
    setConfigNumber(names,L"Active",slot);
}
static int readCustomSection(const wchar_t *section,LZCustom *output) {
    LZCustom input={0};
    int count=(int)GetPrivateProfileIntW(section,L"Nodes",0,g_configPath);
    if(count<1||count>LZ_MAX_NODES)return 0;
    input.count=count;
    int precise=(int)GetPrivateProfileIntW(section,L"Precision",1,g_configPath)==2;
    for(int i=0;i<count;i++) {
        wchar_t key[32]=L"N";
        appendInt(key,32,i);
        int base=lengthW(key);
        copyW(key+base,32-base,L"Split");input.nodes[i].split=(int)GetPrivateProfileIntW(section,key,0,g_configPath);
        copyW(key+base,32-base,L"Ratio");input.nodes[i].ratio=(int)GetPrivateProfileIntW(section,key,precise?5000:50,g_configPath);
        if(!precise) input.nodes[i].ratio*=100;
        copyW(key+base,32-base,L"Left");input.nodes[i].left=(int)GetPrivateProfileIntW(section,key,-1,g_configPath);
        copyW(key+base,32-base,L"Right");input.nodes[i].right=(int)GetPrivateProfileIntW(section,key,-1,g_configPath);
    }
    if(!lz_custom_validate(&input))return 0;
    *output=input;return 1;
}
static void loadCustom(Display *d) {
    wchar_t section[80],names[80],key[32];
    editorNamesSection(names,80,d);
    d->activeProfile=(int)GetPrivateProfileIntW(names,L"Active",0,g_configPath);
    if(d->activeProfile<0||d->activeProfile>=LZ_PROFILE_COUNT)d->activeProfile=0;
    for(int slot=0;slot<LZ_PROFILE_COUNT;slot++) {
        lz_custom_default(&d->profiles[slot]);
        editorProfileSection(section,80,d,slot);
        if(!readCustomSection(section,&d->profiles[slot])&&slot==0) {
            /* Preserve existing custom partitions as profile 1. */
            copyW(section,80,L"Custom_");appendW(section,80,d->configKey);
            readCustomSection(section,&d->profiles[slot]);
        }
        wchar_t defaultName[64];copyW(defaultName,64,lz_text(L"自定义 "));appendInt(defaultName,64,slot+1);
        copyW(key,32,L"Name");appendInt(key,32,slot);
        GetPrivateProfileStringW(names,key,defaultName,d->profileNames[slot],64,g_configPath);
        copyW(key,32,L"HexName");appendInt(key,32,slot);
        wchar_t encoded[300]={0},decoded[64]={0};
        GetPrivateProfileStringW(names,key,L"",encoded,300,g_configPath);
        if(profileHexDecode(encoded,decoded))copyW(d->profileNames[slot],64,decoded);
        if(!d->profileNames[slot][0])copyW(d->profileNames[slot],64,defaultName);
    }
}
static int editorApply(void) {
    if(e_monitor<0||e_monitor>=g_displayCount||!lz_custom_validate(&e_layout))return 0;
    Display *d=&g_displays[e_monitor];
    LZRect check[LZ_MAX_ZONES];
    int count=lz_custom_zones(workRect(d),&e_layout,g_gap,check,(int*)0);
    for(int i=0;i<count;i++) if(check[i].right-check[i].left<120||check[i].bottom-check[i].top<80) {
        copyW(e_notice,160,L"无法保存：有区域小于 120 × 80 像素，请调大分区。");
        editorInvalidate();return 0;
    }
    wchar_t newName[64]=L"";
    if(e_nameEdit)GetWindowTextW(e_nameEdit,newName,64);
    if(!newName[0]) {
        copyW(e_notice,160,L"请先为这个自定义布局输入名称。");
        editorInvalidate();return 0;
    }
    copyW(d->profileNames[e_profile],64,newName);
    d->profiles[e_profile]=e_layout;
    d->activeProfile=e_profile;
    saveProfile(d,e_profile,&e_layout,newName);
    d->layout=LZ_LAYOUT_CUSTOM;
    setConfigNumber(L"Monitors",d->configKey,LZ_LAYOUT_CUSTOM);
    forgetMonitor(d->device);
    refreshZones(d);
    return 1;
}
static void drawBox(HDC dc,LZRect box,DWORD color,DWORD edge) {
    HGDIOBJ fill=(HGDIOBJ)CreateSolidBrush(color);
    HGDIOBJ line=(HGDIOBJ)CreatePen(0,1,edge);
    HGDIOBJ beforeFill=SelectObject(dc,fill);
    HGDIOBJ beforeLine=SelectObject(dc,line);
    RoundRect(dc,box.left,box.top,box.right,box.bottom,14,14);
    SelectObject(dc,beforeLine);SelectObject(dc,beforeFill);
    DeleteObject(line);DeleteObject(fill);
}
static void drawLabelFont(HDC dc,const wchar_t *label,LZRect box,DWORD color,HGDIOBJ font,int align) {
    RECT r={box.left,box.top,box.right,box.bottom};
    HGDIOBJ prev=(HGDIOBJ)0;
    if(font)prev=SelectObject(dc,font);
    SetBkMode(dc,TRANSPARENT);SetTextColor(dc,color);
    DrawTextW(dc,lz_text(label),-1,&r,align|DT_VCENTER|DT_SINGLELINE);
    if(font)SelectObject(dc,prev);
}
static void editorInvalidate(void) {
    if(g_editor)InvalidateRect(g_editor,(RECT*)0,0);
    if(g_editorTools)InvalidateRect(g_editorTools,(RECT*)0,0);
    if(g_editorLabels)InvalidateRect(g_editorLabels,(RECT*)0,0);
}
static void editorMeasureToolbar(void) {
    e_toolbarW=e_w-32;
    if(e_toolbarW>1060)e_toolbarW=1060;
    if(e_toolbarW<510)e_toolbarW=510;
    e_toolbarX=(e_w-e_toolbarW)/2;
}
static LZRect editorButton(int i) {
    int gap=7;
    int width=(e_toolbarW-48-gap*5)/6;
    int x=e_toolbarX+24+(width+gap)*i;
    return (LZRect){x,106,x+width,140};
}
static const wchar_t *editorButtonLabel(int i) {
    static const wchar_t *labels[]={L"＋ 左右拆分",L"＋ 上下拆分",L"合并相邻区",L"恢复三栏",L"保存并启用",L"取消"};
    return labels[i];
}
/* 1:1 physical-work-area editor: the editor's client area is exactly the
   chosen display's work area, not a scaled-down illustration. */
static LZRect editorCanvas(void) { return (LZRect){0,0,e_w,e_h}; }
static int editorInt(const wchar_t *s) {
    int v=0,n=0;while(*s==L' ')s++;
    for(;*s>=L'0'&&*s<=L'9'&&n<7;s++,n++)v=v*10+(*s-L'0');
    return n?v:0;
}
static int editorSelectedRectWithGap(LZRect *out,int gap) {
    LZRect z[LZ_MAX_ZONES];int ids[LZ_MAX_ZONES];
    int count=lz_custom_zones(editorCanvas(),&e_layout,gap,z,ids);
    for(int i=0;i<count;i++) if(ids[i]==e_selected){*out=z[i];return 1;}
    return 0;
}
static int editorSelectedRect(LZRect *out) {return editorSelectedRectWithGap(out,g_gap);}
static void editorSyncDimensions(void) {
    LZRect z;if(!editorSelectedRect(&z))return;
    wchar_t n[24]=L"";
    appendInt(n,24,z.right-z.left);
    if(e_widthEdit)SetWindowTextW(e_widthEdit,n);
    n[0]=0;appendInt(n,24,z.bottom-z.top);
    if(e_heightEdit)SetWindowTextW(e_heightEdit,n);
}
static int editorContainsLeaf(int id,int leaf) {
    if(id==leaf)return 1;
    LZNode *n=&e_layout.nodes[id];
    return n->split && (editorContainsLeaf(n->left,leaf)||editorContainsLeaf(n->right,leaf));
}
/* The deepest ancestor splitting the selected leaf along the requested axis
   owns that leaf's width/height. Recalculate its divider from an exact px input. */
static int editorApplyAxisPx(int axis,int desired) {
    int parentId=0,chosen=-1,first=0;
    LZRect parent=editorCanvas(),chosenParent=parent;
    while(parentId>=0&&parentId<e_layout.count) {
        LZNode *n=&e_layout.nodes[parentId];if(!n->split)break;
        int isFirst=editorContainsLeaf(n->left,e_selected);
        if(n->split==axis) {chosen=parentId;chosenParent=parent;first=isFirst;}
        int mid;
        if(n->split==1) {
            mid=parent.left+(int)(((long long)(parent.right-parent.left)*n->ratio)/10000);
            if(isFirst)parent.right=mid;else parent.left=mid;
        } else {
            mid=parent.top+(int)(((long long)(parent.bottom-parent.top)*n->ratio)/10000);
            if(isFirst)parent.bottom=mid;else parent.top=mid;
        }
        parentId=isFirst?n->left:n->right;
    }
    if(chosen<0)return 0;
    int total=axis==1?chosenParent.right-chosenParent.left:chosenParent.bottom-chosenParent.top;
    if(total<=0||desired<=0)return 0;
    int firstPixels=first?desired:total-desired;
    int ratio=(int)(((long long)firstPixels*10000+total/2)/total);
    return lz_custom_set_ratio(&e_layout,chosen,ratio);
}
static void editorApplyPixelInputs(void) {
    wchar_t w[24]={0},h[24]={0};LZRect before,raw;
    if(!editorSelectedRect(&before)||!editorSelectedRectWithGap(&raw,0))return;
    GetWindowTextW(e_widthEdit,w,24);GetWindowTextW(e_heightEdit,h,24);
    int width=editorInt(w),height=editorInt(h);
    if(width>=120 && width!=before.right-before.left)
        editorApplyAxisPx(1,width+(raw.right-raw.left)-(before.right-before.left));
    if(height>=80 && height!=before.bottom-before.top)
        editorApplyAxisPx(2,height+(raw.bottom-raw.top)-(before.bottom-before.top));
    copyW(e_notice,160,L"已按像素更新分割位置（受相邻区域最小占比限制）。");
    editorSyncDimensions();editorInvalidate();
}
static LZRect editorPixelButton(void) {return (LZRect){e_toolbarX+563,156,e_toolbarX+679,189};}
static void editorDividerDraw(HDC dc,LZRect parent,int id,DWORD accent) {
    if(id<0||id>=e_layout.count)return;
    LZNode node=e_layout.nodes[id];
    if(!node.split)return;
    LZRect a=parent,b=parent;
    int x=0,y=0;
    if(node.split==1) {
        x=parent.left+(int)(((long long)(parent.right-parent.left)*node.ratio)/10000);
        a.right=x;b.left=x;
    } else {
        y=parent.top+(int)(((long long)(parent.bottom-parent.top)*node.ratio)/10000);
        a.bottom=y;b.top=y;
    }
    editorDividerDraw(dc,a,node.left,accent);
    editorDividerDraw(dc,b,node.right,accent);
    int active=(id==e_divider||id==e_hoverDivider);
    DWORD edge=active?accent:lz_mix(accent,COLORREF_(255,255,255),18);
    HBRUSH brush=CreateSolidBrush(edge);
    RECT line;
    if(node.split==1)line=(RECT){x-1,parent.top,x+1,parent.bottom};
    else line=(RECT){parent.left,y-1,parent.right,y+1};
    FillRect(dc,&line,brush);DeleteObject((HGDIOBJ)brush);
    LZRect handle;
    if(node.split==1){y=(parent.top+parent.bottom)/2;handle=(LZRect){x-15,y-25,x+15,y+25};}
    else {x=(parent.left+parent.right)/2;handle=(LZRect){x-25,y-15,x+25,y+15};}
    drawBox(dc,handle,active?accent:COLORREF_(255,255,255),edge);
    drawLabelFont(dc,node.split==1?L"⋮":L"⋯",handle,active?COLORREF_(255,255,255):accent,g_titleFont,DT_CENTER);
}
static void editorPaintInto(HDC dc) {
    DWORD accent=lz_accent(),edge=lz_mix(accent,COLORREF_(255,255,255),77);
    RECT all={0,0,e_w,e_h};
    HBRUSH tint=CreateSolidBrush(COLORREF_(246,241,246));
    FillRect(dc,&all,tint);DeleteObject(tint);
    LZRect canvas=editorCanvas(),zones[LZ_MAX_ZONES];int ids[LZ_MAX_ZONES];
    int count=lz_custom_zones(canvas,&e_layout,g_gap,zones,ids);
    for(int i=0;i<count;i++) {
        LZRect z=zones[i];int selected=ids[i]==e_selected;
        DWORD fill=selected?lz_mix(accent,COLORREF_(255,255,255),65):lz_mix(accent,COLORREF_(255,255,255),88);
        /* On zero-spacing layouts only a one-pixel guide is drawn in the
           editor; the actual snapped windows do not inherit this padding. */
        drawBox(dc,(LZRect){z.left+1,z.top+1,z.right-1,z.bottom-1},fill,selected?accent:edge);
    }
    editorDividerDraw(dc,canvas,0,accent);
    /* Glass surface participates in editor-wide transparency; its lettering
       is painted separately in a color-keyed, fully opaque foreground HWND. */
    editorMeasureToolbar();
    drawBox(dc,(LZRect){e_toolbarX,LZ_EDITOR_TOOLS_Y,
        e_toolbarX+e_toolbarW,LZ_EDITOR_TOOLS_Y+185},COLORREF_(255,251,253),edge);
    if(e_notice[0])drawBox(dc,(LZRect){e_toolbarX+20,216,
        e_toolbarX+e_toolbarW-20,250},COLORREF_(255,252,254),edge);
}
/* Opaque 1:1 zone labels above the translucent editor. Only the little
   labels draw pixels; the remainder uses color-key and passes mouse input
   through to the underlying editor. This avoids multiplying text opacity. */
static void editorPaintLabels(HWND hwnd) {
    PAINTSTRUCT ps;HDC dc=BeginPaint(hwnd,&ps);
    RECT all={0,0,e_w,e_h};
    HBRUSH key=CreateSolidBrush(COLORREF_(255,0,255));
    FillRect(dc,&all,key);DeleteObject(key);
    LZRect zones[LZ_MAX_ZONES];int ids[LZ_MAX_ZONES];
    int count=lz_custom_zones(editorCanvas(),&e_layout,g_gap,zones,ids);
    DWORD accent=lz_accent();
    DWORD bg=COLORREF_(255,253,254),ink=COLORREF_(48,45,63);
    for(int i=0;i<count;i++) {
        LZRect z=zones[i];
        int zw=z.right-z.left,zh=z.bottom-z.top;
        if(zw<122||zh<90)continue;
        int bw=zw-26;if(bw>254)bw=254;
        int bh=zh>=130?78:62;
        int cx=(z.left+z.right)/2,cy=(z.top+z.bottom)/2;
        LZRect card={cx-bw/2,cy-bh/2,cx+(bw+1)/2,cy+(bh+1)/2};
        DWORD border=ids[i]==e_selected?accent:lz_mix(accent,bg,67);
        drawBox(dc,card,bg,border);
        wchar_t name[40];copyW(name,40,lz_text(L"区域 "));appendInt(name,40,i+1);
        wchar_t size[48]=L"";appendInt(size,48,zw);
        appendW(size,48,L" × ");appendInt(size,48,zh);
        drawLabelFont(dc,name,(LZRect){card.left+6,card.top+6,card.right-6,card.top+30},
            ids[i]==e_selected?accent:COLORREF_(111,96,116),e_zoneNameFont,DT_CENTER);
        drawLabelFont(dc,size,(LZRect){card.left+4,card.top+27,card.right-4,card.bottom-4},
            ink,e_zoneSizeFont,DT_CENTER);
    }
    EndPaint(hwnd,&ps);
}
static LRESULT WINCALL editorLabelsProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp) {
    (void)wp;
    if(msg==WM_NCHITTEST)return HTTRANSPARENT;
    if(msg==WM_ERASEBKGND)return 1;
    if(msg==WM_PAINT){editorPaintLabels(hwnd);return 0;}
    if(msg==WM_DESTROY){g_editorLabels=(HWND)0;return 0;}
    return DefWindowProcW(hwnd,msg,wp,lp);
}
static void editorPaintToolbarContent(HDC dc) {
    DWORD accent=lz_accent(),edge=lz_mix(accent,COLORREF_(255,255,255),77);
    DWORD ink=COLORREF_(53,50,64),muted=COLORREF_(110,105,122);
    /* All editor controls live in one centered floating toolbar. */
    editorMeasureToolbar();
    drawBox(dc,(LZRect){e_toolbarX+15,33,e_toolbarX+241,73},
        COLORREF_(255,250,252),lz_mix(accent,COLORREF_(255,255,255),82));
    drawLabelFont(dc,L"LiteZones  /  轻格",(LZRect){e_toolbarX+27,34,e_toolbarX+236,72},ink,g_titleFont,0);
    wchar_t display[128];copyW(display,128,lz_text(L"显示器 "));appendInt(display,128,e_monitor+1);
    appendW(display,128,lz_text(L"   ·   工作区 "));appendInt(display,128,e_w);
    appendW(display,128,L" × ");appendInt(display,128,e_h);appendW(display,128,lz_text(L" px   ·   1:1 实际预览"));
    drawLabelFont(dc,display,(LZRect){e_toolbarX+254,37,e_toolbarX+e_toolbarW-260,71},muted,e_smallFont,DT_CENTER);
    drawLabelFont(dc,L"布局名称",(LZRect){e_toolbarX+e_toolbarW-256,39,e_toolbarX+e_toolbarW-164,73},muted,e_smallFont,0);
    for(int i=0;i<6;i++) {
        LZRect b=editorButton(i);
        DWORD bg=i==4?accent:(i==5?COLORREF_(255,247,249):COLORREF_(255,251,253));
        DWORD stroke=i==4?accent:lz_mix(accent,COLORREF_(255,255,255),58);
        drawBox(dc,b,bg,stroke);
        drawLabelFont(dc,editorButtonLabel(i),b,
            i==4?COLORREF_(255,255,255):ink,g_uiFont,DT_CENTER);
    }
    drawLabelFont(dc,L"当前区域",(LZRect){e_toolbarX+25,158,e_toolbarX+130,191},ink,g_uiFont,0);
    drawLabelFont(dc,L"宽",(LZRect){e_toolbarX+140,158,e_toolbarX+171,189},muted,g_uiFont,0);
    drawLabelFont(dc,L"高",(LZRect){e_toolbarX+348,158,e_toolbarX+380,189},muted,g_uiFont,0);
    drawBox(dc,(LZRect){e_toolbarX+175,156,e_toolbarX+310,189},COLORREF_(255,255,255),edge);
    drawBox(dc,(LZRect){e_toolbarX+386,156,e_toolbarX+521,189},COLORREF_(255,255,255),edge);
    LZRect apply=editorPixelButton();drawBox(dc,apply,accent,accent);
    drawLabelFont(dc,L"应用尺寸",apply,COLORREF_(255,255,255),g_uiFont,DT_CENTER);
    if(e_toolbarW>820)
        drawLabelFont(dc,L"Esc 退出 · 拖动粉色手柄实时调整",(LZRect){e_toolbarX+691,158,e_toolbarX+e_toolbarW-12,188},muted,e_smallFont,0);
    if(e_notice[0]) {
        LZRect hint={e_toolbarX+20,216,e_toolbarX+e_toolbarW-20,250};
        drawBox(dc,hint,COLORREF_(255,252,254),edge);
        drawLabelFont(dc,e_notice,hint,muted,e_smallFont,DT_CENTER);
    }
}

static void editorReleaseBuffer(void) {
    if(e_bufferDC) {
        if(e_bufferOld)SelectObject(e_bufferDC,e_bufferOld);
        if(e_bufferBitmap)DeleteObject(e_bufferBitmap);
        DeleteDC(e_bufferDC);
    }
    e_bufferDC=(HDC)0;e_bufferBitmap=(HGDIOBJ)0;e_bufferOld=(HGDIOBJ)0;e_bufferW=0;e_bufferH=0;
}
static void editorPaint(HWND hwnd) {
    PAINTSTRUCT ps;HDC surface=BeginPaint(hwnd,&ps);
    if(!e_bufferDC || e_bufferW!=e_w || e_bufferH!=e_h) {
        editorReleaseBuffer();
        HDC off=CreateCompatibleDC(surface);
        HGDIOBJ bmp=CreateCompatibleBitmap(surface,e_w,e_h);
        if(off && bmp) {
            e_bufferOld=SelectObject(off,bmp);
            e_bufferDC=off;e_bufferBitmap=bmp;e_bufferW=e_w;e_bufferH=e_h;
        } else {
            if(bmp)DeleteObject(bmp);
            if(off)DeleteDC(off);
        }
    }
    if(e_bufferDC){editorPaintInto(e_bufferDC);BitBlt(surface,0,0,e_w,e_h,e_bufferDC,0,0,SRCCOPY);}
    else editorPaintInto(surface);
    EndPaint(hwnd,&ps);
}
static int editorFirstLeaf(void) {
    int node=0;
    while(node>=0&&node<e_layout.count && e_layout.nodes[node].split)node=e_layout.nodes[node].left;
    return node;
}
static int editorHitZone(int x,int y) {
    LZRect area=editorCanvas(),zones[LZ_MAX_ZONES];int ids[LZ_MAX_ZONES];
    int count=lz_custom_zones(area,&e_layout,0,zones,ids);
    int z=lz_pick_zone(area,zones,count,x,y);
    return z>=0?ids[z]:-1;
}
static void editorPositionName(void) {
    editorMeasureToolbar();
    if(e_nameEdit)MoveWindow(e_nameEdit,e_toolbarW-154,41-LZ_EDITOR_TOOLS_Y,131,27,1);
    if(e_widthEdit)MoveWindow(e_widthEdit,179,159-LZ_EDITOR_TOOLS_Y,128,27,1);
    if(e_heightEdit)MoveWindow(e_heightEdit,390,159-LZ_EDITOR_TOOLS_Y,128,27,1);
    if(e_monitor>=0 && e_monitor<g_displayCount) {
        RECT r=g_displays[e_monitor].work;
        if(g_editorLabels)MoveWindow(g_editorLabels,r.left,r.top,e_w,e_h,1);
        if(g_editorTools)MoveWindow(g_editorTools,r.left+e_toolbarX,r.top+LZ_EDITOR_TOOLS_Y,
                                   e_toolbarW,LZ_EDITOR_TOOLS_H,1);
    }
}
static int editorHandleToolbarClick(int x,int y) {
        for(int i=0;i<6;i++) {
            LZRect b=editorButton(i);
            if(x<b.left||x>=b.right||y<b.top||y>=b.bottom)continue;
            if(i==0||i==1) {
                int selected=e_selected;
                if(lz_custom_split(&e_layout,selected,i==0?1:2)) {
                    e_selected=e_layout.nodes[selected].left;
                    copyW(e_notice,160,L"已拆分。可以拖动粉色手柄调整比例。");
                } else copyW(e_notice,160,L"最多 12 区域；请先选择未拆分的小区域。");
            } else if(i==2) {
                if(lz_custom_merge(&e_layout,e_selected)) {
                    e_selected=editorFirstLeaf();
                    copyW(e_notice,160,L"两个同级区域已合并。");
                } else copyW(e_notice,160,L"只能合并同一分割线两侧的两个完整区域。");
            } else if(i==3) {
                lz_custom_default(&e_layout);e_selected=editorFirstLeaf();
                copyW(e_notice,160,L"已重置为 25% / 50% / 25%，保存后生效。");
            } else if(i==4){if(editorApply())DestroyWindow(g_editor);return 1;}
            else {DestroyWindow(g_editor);return 1;}
            editorSyncDimensions();editorInvalidate();return 1;
        }
        LZRect pixelButton=editorPixelButton();
        if(x>=pixelButton.left&&x<pixelButton.right&&y>=pixelButton.top&&y<pixelButton.bottom) {
            editorApplyPixelInputs();return 1;
        }
    return 0;
}
static LRESULT WINCALL editorToolsProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp) {
    (void)wp;
    if(msg==WM_ERASEBKGND)return 1;
    if(msg==WM_PAINT) {
        PAINTSTRUCT ps;HDC dc=BeginPaint(hwnd,&ps);
        HBRUSH key=CreateSolidBrush(COLORREF_(255,0,255));
        RECT r={0,0,e_toolbarW,LZ_EDITOR_TOOLS_H};FillRect(dc,&r,key);DeleteObject(key);
        POINT before={0,0};
        SetViewportOrgEx(dc,-e_toolbarX,-LZ_EDITOR_TOOLS_Y,&before);
        editorPaintToolbarContent(dc);
        SetViewportOrgEx(dc,before.x,before.y,(POINT*)0);
        EndPaint(hwnd,&ps);return 0;
    }
    if(msg==WM_LBUTTONUP) {
        int x=(short)(lp&65535)+e_toolbarX;
        int y=(short)((lp>>16)&65535)+LZ_EDITOR_TOOLS_Y;
        editorHandleToolbarClick(x,y);return 0;
    }
    if(msg==WM_KEYDOWN&&wp==VK_ESCAPE){DestroyWindow(g_editor);return 0;}
    if(msg==WM_CLOSE){if(g_editor)DestroyWindow(g_editor);return 0;}
    if(msg==WM_DESTROY){g_editorTools=(HWND)0;return 0;}
    return DefWindowProcW(hwnd,msg,wp,lp);
}
static LRESULT WINCALL editorProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp) {
    (void)wp;
    if(msg==WM_ERASEBKGND)return 1;
    if(msg==WM_SIZE) {
        RECT r;if(GetClientRect(hwnd,&r)){e_w=r.right;e_h=r.bottom;}
        editorPositionName();
        InvalidateRect(hwnd,(RECT*)0,0);return 0;
    }
    if(msg==WM_PAINT){editorPaint(hwnd);return 0;}
    if(msg==WM_TIMER && wp==53) {KillTimer(hwnd,53);e_pendingPaint=0;InvalidateRect(hwnd,(RECT*)0,0);return 0;}
    if(msg==WM_KEYDOWN && wp==VK_ESCAPE) {DestroyWindow(hwnd);return 0;}
    if(msg==WM_LBUTTONDOWN) {
        int x=(short)(lp&65535),y=(short)((lp>>16)&65535);
        LZRect parent;
        int node=lz_custom_divider(editorCanvas(),&e_layout,x,y,16,&parent);
        if(node>=0){e_divider=node;e_dragParent=parent;SetCapture(hwnd);InvalidateRect(hwnd,(RECT*)0,0);return 0;}
        LZRect canvas=editorCanvas();
        if(x>=canvas.left&&x<canvas.right&&y>=canvas.top&&y<canvas.bottom) {
            int zone=editorHitZone(x,y);
            if(zone>=0){e_selected=zone;editorSyncDimensions();editorInvalidate();}
        }
        return 0;
    }
    if(msg==WM_MOUSEMOVE) {
        int x=(short)(lp&65535),y=(short)((lp>>16)&65535);
        if(e_divider>=0) {
            LZNode *node=&e_layout.nodes[e_divider];
            int numerator=node->split==1?x-e_dragParent.left:y-e_dragParent.top;
            int denominator=node->split==1?e_dragParent.right-e_dragParent.left:e_dragParent.bottom-e_dragParent.top;
            if(denominator>0) {
                int proposed=(int)(((long long)numerator*10000+denominator/2)/denominator);
                if(proposed!=node->ratio) {
                    lz_custom_set_ratio(&e_layout,e_divider,proposed);
                    if(!e_pendingPaint) {e_pendingPaint=1;SetTimer(hwnd,53,12,(void*)0);}
                }
            }
            return 0;
        }
        LZRect parent;
        int hovered=lz_custom_divider(editorCanvas(),&e_layout,x,y,16,&parent);
        if(hovered!=e_hoverDivider) {
            e_hoverDivider=hovered;InvalidateRect(hwnd,(RECT*)0,0);
        }
        return 0;
    }
    if(msg==WM_LBUTTONUP) {
        if(e_divider>=0){e_divider=-1;ReleaseCapture();editorSyncDimensions();editorInvalidate();}
        return 0;
    }
    if(msg==WM_CAPTURECHANGED){e_divider=-1;return 0;}
    if(msg==WM_CLOSE){DestroyWindow(hwnd);return 0;}
    if(msg==WM_DESTROY) {
        KillTimer(hwnd,53);e_pendingPaint=0;
        if(g_editorTools){DestroyWindow(g_editorTools);g_editorTools=(HWND)0;}
        if(g_editorLabels){DestroyWindow(g_editorLabels);g_editorLabels=(HWND)0;}
        e_nameEdit=(HWND)0;e_widthEdit=(HWND)0;e_heightEdit=(HWND)0;editorReleaseBuffer();
        if(e_zoneNameFont){DeleteObject(e_zoneNameFont);e_zoneNameFont=(HGDIOBJ)0;}
        if(e_zoneSizeFont){DeleteObject(e_zoneSizeFont);e_zoneSizeFont=(HGDIOBJ)0;}
        if(e_smallFont) {DeleteObject(e_smallFont);e_smallFont=(HGDIOBJ)0;}
        g_editor=(HWND)0;e_monitor=-1;e_divider=-1;return 0;
    }
    return DefWindowProcW(hwnd,msg,wp,lp);
}
static void openEditor(int monitor) {
    if(monitor<0||monitor>=g_displayCount)return;
    if(g_editor){SetForegroundWindow(g_editor);return;}
    e_monitor=monitor;e_profile=g_displays[monitor].activeProfile;
    e_layout=g_displays[monitor].profiles[e_profile];
    if(!lz_custom_validate(&e_layout))lz_custom_default(&e_layout);
    e_selected=1;e_divider=-1;e_hoverDivider=-1;e_pendingPaint=0;
    copyW(e_notice,160,L"实时、按屏幕像素编辑。按 Esc 或“取消”退出。");
    RECT work=g_displays[monitor].work;
    int width=work.right-work.left,height=work.bottom-work.top;
    /* The whole editor spans the display's usable desktop, exactly 1:1. */
    g_editor=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_TOPMOST|WS_EX_LAYERED,kClassEditor,lz_text(L"LiteZones · 自定义分屏布局"),
      WS_POPUP|WS_CLIPCHILDREN,work.left,work.top,width,height,
      (HWND)0,(HMENU)0,g_instance,(void*)0);
    if(!g_editor){e_monitor=-1;return;}
    e_w=width;e_h=height;
    e_smallFont=CreateFontW(-16,0,0,0,500,0,0,0,1,0,0,5,0,L"Segoe UI");
    e_zoneNameFont=CreateFontW(-17,0,0,0,600,0,0,0,1,0,0,5,0,L"Segoe UI");
    e_zoneSizeFont=CreateFontW(-24,0,0,0,650,0,0,0,1,0,0,5,0,L"Segoe UI");
    editorMeasureToolbar();
    g_editorLabels=CreateWindowExW(WS_EX_TOPMOST|WS_EX_TOOLWINDOW|WS_EX_LAYERED|
        WS_EX_TRANSPARENT|WS_EX_NOACTIVATE,kClassEditorLabels,lz_text(L"LiteZones · 区域标签"),
        WS_POPUP,work.left,work.top,width,height,
        (HWND)0,(HMENU)0,g_instance,(void*)0);
    if(!g_editorLabels){DestroyWindow(g_editor);return;}
    SetLayeredWindowAttributes(g_editorLabels,COLORREF_(255,0,255),255,LWA_COLORKEY);
    g_editorTools=CreateWindowExW(WS_EX_TOPMOST|WS_EX_TOOLWINDOW|WS_EX_LAYERED,
        kClassEditorTools,lz_text(L"LiteZones · 编辑工具栏"),WS_POPUP,
        work.left+e_toolbarX,work.top+LZ_EDITOR_TOOLS_Y,e_toolbarW,LZ_EDITOR_TOOLS_H,
        (HWND)0,(HMENU)0,g_instance,(void*)0);
    if(!g_editorTools){DestroyWindow(g_editor);return;}
    SetLayeredWindowAttributes(g_editorTools,COLORREF_(255,0,255),255,LWA_COLORKEY);
    e_nameEdit=CreateWindowExW(0,L"EDIT",g_displays[monitor].profileNames[e_profile],
        WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,
        0,0,100,27,g_editorTools,(HMENU)0,g_instance,(void*)0);
    e_widthEdit=CreateWindowExW(0,L"EDIT",L"",
        WS_CHILD|WS_VISIBLE|ES_NUMBER,
        0,0,100,27,g_editorTools,(HMENU)0,g_instance,(void*)0);
    e_heightEdit=CreateWindowExW(0,L"EDIT",L"",
        WS_CHILD|WS_VISIBLE|ES_NUMBER,
        0,0,100,27,g_editorTools,(HMENU)0,g_instance,(void*)0);
    if(e_nameEdit)SendMessageW(e_nameEdit,WM_SETFONT,(WPARAM)g_uiFont,1);
    if(e_widthEdit)SendMessageW(e_widthEdit,WM_SETFONT,(WPARAM)g_uiFont,1);
    if(e_heightEdit)SendMessageW(e_heightEdit,WM_SETFONT,(WPARAM)g_uiFont,1);
    editorPositionName();
    editorSyncDimensions();
    SetLayeredWindowAttributes(g_editor,0,(BYTE)g_overlayOpacity,LWA_ALPHA);
    ShowWindow(g_editor,SW_SHOW);
    ShowWindow(g_editorLabels,SW_SHOWNOACTIVATE);
    ShowWindow(g_editorTools,SW_SHOW);
    SetForegroundWindow(g_editorTools);
}

