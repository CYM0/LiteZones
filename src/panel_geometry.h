#ifndef LZ_PANEL_GEOMETRY_H
#define LZ_PANEL_GEOMETRY_H
/* Pure, Win32-independent hit-test used by the single-HWND flyout menu. */
typedef struct {
    int active,x,y,width,height,header,row_height,row_count;
} LZPanelHitRect;
static inline int lz_panel_hit(const LZPanelHitRect *panels,int count,int x,int y,int *row) {
    for(int i=count-1;i>=0;i--) {
        const LZPanelHitRect *p=&panels[i];
        if(!p->active || x<p->x || x>=p->x+p->width || y<p->y || y>=p->y+p->height)continue;
        int r=y-p->y>=p->header?(y-p->y-p->header)/p->row_height:-1;
        if(r<0 || r>=p->row_count)r=-1;
        *row=r;
        return i;
    }
    *row=-1;
    return -1;
}
#endif
