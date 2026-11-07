#ifndef LZ_ZONE_ZORDER_H
#define LZ_ZONE_ZORDER_H
#include "zone_book.h"
/* Given a snapshot of desktop top-level HWND IDs in FRONT->BACK order,
   select the frontmost tracked member in a zone.  Do not use timestamps to
   substitute for the OS Z-order: focused and frontmost are not synonyms. */
static inline LZWindowID lz_zone_z_top(const LZBook *b,int monitor,int zone,
                                       LZWindowID except,const LZWindowID *frontToBack,int count) {
    if(!frontToBack||count<=0)return 0;
    for(int j=0;j<count;j++) {
        LZWindowID id=frontToBack[j];
        if(!id||id==except)continue;
        int slot=lz_book_slot(b,id);
        if(slot>=0 && b->members[slot].monitor==monitor && b->members[slot].zone==zone)
            return id;
    }
    return 0;
}
#endif
