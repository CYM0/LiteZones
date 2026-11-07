/* Lightweight native palette. RGB values stored in Windows COLORREF order. */
#define LZ_THEME_SYSTEM 0
#define LZ_THEME_CHERRY 1
#define LZ_THEME_LAVENDER 2
#define LZ_THEME_MINT 3
#define LZ_THEME_SKY 4
#define LZ_THEME_CUSTOM 5
#define LZ_THEME_CLASSIC 6
static int g_themeMode=LZ_THEME_CHERRY,g_overlayOpacity=108;
static DWORD g_customAccent=COLORREF_(241,125,166);
static DWORD g_colorChoices[16]={0};
static HGDIOBJ g_uiFont=(HGDIOBJ)0,g_titleFont=(HGDIOBJ)0;
static DWORD lz_mix(DWORD a,DWORD b,int percentOfB) {
    int inv=100-percentOfB;
    int r=(((a&255)*inv+(b&255)*percentOfB)+50)/100;
    int g=((((a>>8)&255)*inv+((b>>8)&255)*percentOfB)+50)/100;
    int bl=((((a>>16)&255)*inv+((b>>16)&255)*percentOfB)+50)/100;
    return COLORREF_(r,g,bl);
}
static DWORD lz_accent(void) {
    if(g_themeMode==LZ_THEME_CHERRY)return COLORREF_(243,111,157);
    if(g_themeMode==LZ_THEME_LAVENDER)return COLORREF_(153,124,214);
    if(g_themeMode==LZ_THEME_MINT)return COLORREF_(66,164,134);
    if(g_themeMode==LZ_THEME_SKY)return COLORREF_(76,151,213);
    if(g_themeMode==LZ_THEME_CUSTOM)return g_customAccent;
    if(g_themeMode==LZ_THEME_CLASSIC)return COLORREF_(44,117,208);
    DWORD argb=0;BOOL opaque=0;
    if(DwmGetColorizationColor(&argb,&opaque)==0) {
        (void)opaque;
        return COLORREF_((argb>>16)&255,(argb>>8)&255,argb&255);
    }
    return GetSysColor(13); /* COLOR_HIGHLIGHT fallback */
}
static DWORD lz_softColor(int selected) {
    return lz_mix(lz_accent(),COLORREF_(255,255,255),selected?37:76);
}
static void lz_repaintTheme(void) {
    for(int i=0;i<g_displayCount;i++)if(g_displays[i].overlay)
        InvalidateRect(g_displays[i].overlay,(const RECT*)0,0);
    if(g_editor)InvalidateRect(g_editor,(const RECT*)0,0);
}
static void lz_applyOpacity(void) {
    for(int i=0;i<g_displayCount;i++)if(g_displays[i].overlay)
        SetLayeredWindowAttributes(g_displays[i].overlay,COLORREF_(255,0,255),(BYTE)g_overlayOpacity,LWA_COLORKEY|LWA_ALPHA);
    lz_repaintTheme();
}
static void lz_setTheme(int mode) {
    if(mode<LZ_THEME_SYSTEM||mode>LZ_THEME_CLASSIC)return;
    g_themeMode=mode;setConfigNumber(L"Appearance",L"Theme",mode);
    lz_repaintTheme();
}
static void lz_pickCustomColor(void) {
    CHOOSECOLORW cc;memset(&cc,0,sizeof(cc));
    cc.lStructSize=sizeof(cc);cc.hwndOwner=g_editor?g_editor:g_main;
    cc.rgbResult=g_customAccent;cc.lpCustColors=g_colorChoices;
    cc.Flags=0x00000001u|0x00000002u; /* CC_RGBINIT | CC_FULLOPEN */
    if(ChooseColorW(&cc)) {
        g_customAccent=cc.rgbResult;
        setConfigNumber(L"Appearance",L"CustomColor",(int)g_customAccent);
        lz_setTheme(LZ_THEME_CUSTOM);
    }
}
