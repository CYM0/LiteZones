#ifndef LZ_SEAM_H
#define LZ_SEAM_H
#include "geometry.h"
typedef struct {int left,right,top,bottom;} LZBleed;
/* Visual overlap. Applied ONLY when placing a real window, not
   to the logical zones. Thus hit testing and swaps remain deterministic. */
static inline LZRect lz_expand_zone(LZRect zone,LZBleed bleed) {
    zone.left-=bleed.left;
    zone.right+=bleed.right;
    zone.top-=bleed.top;
    zone.bottom+=bleed.bottom;
    return zone;
}
#endif
