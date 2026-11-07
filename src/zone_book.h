#ifndef LITEZONES_ZONE_BOOK_H
#define LITEZONES_ZONE_BOOK_H
/* one record per managed WINDOW, not per zone.
   Covered records remain attached to their zone. A zone's current foreground
   is the highest activity serial among its records. This is an internal
   *logical* stack; Windows retains the real z-order and focus behavior. */
#include "geometry.h"
#define LZ_BOOK_CAPACITY 96
typedef unsigned long long LZWindowID;
typedef struct {
    LZWindowID id;
    int monitor, zone;
    LZRect preSnap;
    int hasPreSnap;
    unsigned long long activity;
} LZMember;
typedef struct {
    LZMember members[LZ_BOOK_CAPACITY];
    unsigned long long clock;
} LZBook;
static inline int lz_book_slot(const LZBook *b,LZWindowID id) {
    if(!id)return -1;
    for(int i=0;i<LZ_BOOK_CAPACITY;i++)if(b->members[i].id==id)return i;
    return -1;
}
static inline int lz_book_can_add(const LZBook *b,LZWindowID id) {
    if(lz_book_slot(b,id)>=0)return 1;
    for(int i=0;i<LZ_BOOK_CAPACITY;i++)if(!b->members[i].id)return 1;
    return 0;
}
static inline void lz_book_remove(LZBook *b,LZWindowID id) {
    int i=lz_book_slot(b,id);
    if(i>=0)b->members[i].id=0;
}
static inline void lz_book_clear_monitor(LZBook *b,int monitor) {
    for(int i=0;i<LZ_BOOK_CAPACITY;i++)
        if(b->members[i].id && b->members[i].monitor==monitor)b->members[i].id=0;
}
static inline void lz_book_clear(LZBook *b) {
    for(int i=0;i<LZ_BOOK_CAPACITY;i++)b->members[i].id=0;
}
static inline void lz_book_touch(LZBook *b,LZWindowID id) {
    int i=lz_book_slot(b,id);
    if(i>=0) {
        /* Rebase rarely to avoid a sequence wrap on long-running systems. */
        if(b->clock>0xffffffffffff0000ULL) {
            for(int j=0;j<LZ_BOOK_CAPACITY;j++)b->members[j].activity>>=1;
            b->clock>>=1;
        }
        b->members[i].activity=++b->clock;
    }
}
/* Record/move a window; do NOT discard the other windows in the target zone. */
static inline int lz_book_place(LZBook *b,LZWindowID id,int monitor,int zone,const LZRect *preSnap) {
    if(!id)return 0;
    int i=lz_book_slot(b,id);
    if(i<0) {
        for(int j=0;j<LZ_BOOK_CAPACITY;j++)if(!b->members[j].id){i=j;break;}
        if(i<0)return 0;
        b->members[i].hasPreSnap=preSnap!=0;
        if(preSnap)b->members[i].preSnap=*preSnap;
    }
    b->members[i].id=id;
    b->members[i].monitor=monitor;
    b->members[i].zone=zone;
    lz_book_touch(b,id);
    return 1;
}
static inline LZWindowID lz_book_top(const LZBook *b,int monitor,int zone,LZWindowID except) {
    LZWindowID answer=0;
    unsigned long long newest=0;
    for(int i=0;i<LZ_BOOK_CAPACITY;i++) {
        const LZMember *p=&b->members[i];
        if(p->id && p->id!=except && p->monitor==monitor && p->zone==zone &&
           (!answer || p->activity>newest)) {answer=p->id;newest=p->activity;}
    }
    return answer;
}
/* 'Empty' means there is no record, including records hidden behind a top
   window or temporarily minimized. Such zones are NOT free. */
static inline int lz_book_empty(const LZBook *b,int monitor,int zone) {
    return lz_book_top(b,monitor,zone,0)==0;
}
/* Deterministic Euclidean center distance, using doubled coordinates so
   odd-sized rectangles are precise and ties favor the lowest zone index. */
static inline int lz_book_nearest_empty(const LZBook *b,int monitor,int from,
                                        const LZRect *zones,int count) {
    int best=-1;
    unsigned long long distance=0;
    if(!zones || from<0 || from>=count)return -1;
    long long ax=(long long)zones[from].left+zones[from].right;
    long long ay=(long long)zones[from].top+zones[from].bottom;
    for(int i=0;i<count;i++) {
        if(i==from || !lz_book_empty(b,monitor,i))continue;
        long long dx=ax-((long long)zones[i].left+zones[i].right);
        long long dy=ay-((long long)zones[i].top+zones[i].bottom);
        unsigned long long d=(unsigned long long)(dx*dx+dy*dy);
        if(best<0 || d<distance) {best=i;distance=d;}
    }
    return best;
}
static inline int lz_book_swap(LZBook *b,LZWindowID a,LZWindowID c) {
    int ai=lz_book_slot(b,a),ci=lz_book_slot(b,c);
    if(ai<0||ci<0||ai==ci)return 0;
    int monitor=b->members[ai].monitor,zone=b->members[ai].zone;
    b->members[ai].monitor=b->members[ci].monitor;
    b->members[ai].zone=b->members[ci].zone;
    b->members[ci].monitor=monitor;
    b->members[ci].zone=zone;
    lz_book_touch(b,a);
    lz_book_touch(b,c);
    return 1;
}
typedef enum {
    LZ_MOVE_FREE, LZ_MOVE_SAME, LZ_MOVE_SWAP, LZ_MOVE_DISPLACE, LZ_MOVE_COVER
} LZMovePlan;
static inline LZMovePlan lz_book_plan(int owned,int sameZone,int targetHasTop,int emptyZone) {
    if(sameZone)return LZ_MOVE_SAME;
    if(!targetHasTop)return LZ_MOVE_FREE;
    if(owned)return LZ_MOVE_SWAP;
    return emptyZone>=0 ? LZ_MOVE_DISPLACE : LZ_MOVE_COVER;
}
#endif

