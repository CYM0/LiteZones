#ifndef LZ_RESTORE_H
#define LZ_RESTORE_H
#include "geometry.h"
/* Compute Windows-Snap-style drag-out: restore the original size near the
   current cursor instead of teleporting the window back to its old location.
   Pure calculation; does not depend on Win32. */
static inline LZRect lz_restored_bounds(LZRect original,LZRect snap,LZRect work,
                                       int startX,int startY,int releaseX,int releaseY) {
    int width=original.right-original.left;
    int height=original.bottom-original.top;
    int snapWidth=snap.right-snap.left;
    if(width<120||height<80||snapWidth<=0)return snap;
    int relative=startX-snap.left;
    if(relative<0)relative=0;
    if(relative>snapWidth)relative=snapWidth;
    int x=releaseX-(int)((long long)relative*width/snapWidth);
    int yOffset=startY-snap.top;
    if(yOffset<0)yOffset=0;
    if(yOffset>36)yOffset=36;
    int y=releaseY-yOffset;
    if(width<=work.right-work.left) {
        if(x<work.left)x=work.left;
        if(x+width>work.right)x=work.right-width;
    }
    if(y<work.top)y=work.top;
    if(height<=work.bottom-work.top&&y+height>work.bottom)
        y=work.bottom-height;
    LZRect r={x,y,x+width,y+height};
    return r;
}
#endif
