/* Zone state: covered windows retain their zone records. Only a zone's
   current logical top (most recent foreground activation) is exchanged.
   Foreground events update rank; they don't move or resize windows. */
#define MAX_PLACEMENTS LZ_BOOK_CAPACITY
static LZBook g_book;
static LZWindowID windowId(HWND hwnd) {return (LZWindowID)(UINT_PTR)hwnd;}
static HWND idWindow(LZWindowID id) {return (HWND)(UINT_PTR)id;}
static int sameW(const wchar_t *a,const wchar_t *b) {
    int i=0;while(a[i]&&b[i]&&a[i]==b[i])i++;
    return a[i]==b[i];
}
static int placeSlot(HWND hwnd) {return lz_book_slot(&g_book,windowId(hwnd));}
static void forgetWindow(HWND hwnd) {lz_book_remove(&g_book,windowId(hwnd));}
static void forgetMonitor(const wchar_t *name) {
    for(int mi=0;mi<g_displayCount;mi++)
        if(sameW(name,g_displays[mi].device))lz_book_clear_monitor(&g_book,mi);
}
static void pruneDeadPlacements(void) {
    for(int i=0;i<MAX_PLACEMENTS;i++) {
        LZMember *p=&g_book.members[i];
        if(p->id&&!IsWindow(idWindow(p->id)))p->id=0;
    }
}
static int knownSource(HWND hwnd,int *mi,int *zi) {
    int slot=placeSlot(hwnd);
    if(slot<0)return 0;
    if(!IsWindow(hwnd)) {forgetWindow(hwnd);return 0;}
    int m=g_book.members[slot].monitor,z=g_book.members[slot].zone;
    if(m>=0&&m<g_displayCount&&z>=0&&z<g_displays[m].count) {
        *mi=m;*zi=z;return 1;
    }
    forgetWindow(hwnd);
    return 0;
}
static int recordPlacement(HWND hwnd,int mi,int zi,const RECT *preSnap) {
    LZRect saved={0,0,0,0};
    if(preSnap)saved=(LZRect){preSnap->left,preSnap->top,preSnap->right,preSnap->bottom};
    return lz_book_place(&g_book,windowId(hwnd),mi,zi,preSnap?&saved:(LZRect*)0);
}
/* Foreground activation is recorded for optional diagnostics / fallback; it
   is NOT an authoritative physical stacking signal. Windows Z-order is. */
static void activatePlacement(HWND hwnd) {lz_book_touch(&g_book,windowId(hwnd));}
/* EnumWindows visits top-level windows in front-to-back desktop Z-order on
   supported Windows desktops. Collect a short snapshot of tracked IDs while
   leaving offscreen/unmanaged windows alone. Never move/hide them. */
typedef struct {LZWindowID ids[MAX_PLACEMENTS]; int count;} LZOrderView;
static BOOL WINCALL collectManagedZ(HWND hwnd,LPARAM user) {
    LZOrderView *v=(LZOrderView*)(UINT_PTR)user;
    if(v->count>=MAX_PLACEMENTS)return 0;
    if(placeSlot(hwnd)>=0 && IsWindowVisible(hwnd) && !IsIconic(hwnd))
        v->ids[v->count++]=windowId(hwnd);
    return 1;
}
static LZOrderView desktopOrder(void) {
    LZOrderView v;memset(&v,0,sizeof(v));
    EnumWindows(collectManagedZ,(LPARAM)(UINT_PTR)&v);
    return v;
}
static HWND occupying(int mi,int zi,HWND except) {
    LZOrderView order=desktopOrder();
    return idWindow(lz_zone_z_top(&g_book,mi,zi,windowId(except),order.ids,order.count));
}
static int nearestVacant(int mi,int zi) {
    pruneDeadPlacements();
    return lz_book_nearest_empty(&g_book,mi,zi,g_displays[mi].zones,g_displays[mi].count);
}
static void revertMoved(HWND hwnd,const RECT *previous) {
    if(previous)SetWindowPos(hwnd,(HWND)0,previous->left,previous->top,
        previous->right-previous->left,previous->bottom-previous->top,
        SWP_NOZORDER|SWP_NOACTIVATE);
}
static void snapWithExchange(HWND dragging,int dstMi,int dstZi,int srcMi,int srcZi,const RECT *before) {
    Display *dest=&g_displays[dstMi];
    int owned=srcMi>=0&&srcZi>=0;
    int same=owned && srcMi==dstMi && srcZi==dstZi;
    /* The HWND being physically dragged is explicitly chosen by the user.
       Do NOT reject it because an asynchronous activation or a stale Z-order
       snapshot temporarily reports another member as its source-zone top.
       During a successful exchange we promote the dragged window to the
       front of its normal Z-order band before placing its counterpart behind. */
    HWND old=same?(HWND)0:occupying(dstMi,dstZi,dragging);
    int empty=(!owned&&old)?nearestVacant(dstMi,dstZi):-1;
    LZMovePlan action=lz_book_plan(owned,same,old!=(HWND)0,empty);
    if(!lz_book_can_add(&g_book,windowId(dragging)))return;
    if(action==LZ_MOVE_SWAP) {
        /* "Swap" means geometry + book ownership; Z-order is a separate,
           best-effort VISUAL follow-up and never vetoes an otherwise valid
           exchange. Only the user-dragged and current visible target member
           move; covered members retain their zone assignments and positions. */
        if(!moveToZone(old,&g_displays[srcMi],srcZi))return;
        if(!moveToZone(dragging,dest,dstZi)) {
            if(!moveToZone(old,dest,dstZi))forgetWindow(old);
            revertMoved(dragging,before);
            return;
        }
        lz_book_swap(&g_book,windowId(old),windowId(dragging));
        restackSwappedOwners(old,dragging);
        return;
    }
    if(action==LZ_MOVE_DISPLACE) {
        /* External newcomer. Move only the CURRENT TOP to the nearest truly
           empty zone; older covered members stay in the original zone. */
        if(moveToZone(old,dest,empty)) {
            if(moveToZone(dragging,dest,dstZi)) {
                recordPlacement(old,dstMi,empty,(RECT*)0);
                recordPlacement(dragging,dstMi,dstZi,before);
                raiseWindowWithoutActivation(old);
                raiseWindowWithoutActivation(dragging);
                return;
            }
            /* Failed second move: restore the old top to its previous zone.
               If rollback itself fails, record where it physically remains. */
            if(!moveToZone(old,dest,dstZi))recordPlacement(old,dstMi,empty,(RECT*)0);
            revertMoved(dragging,before);
            return;
        }
        /* An app that refuses the free zone must not silently change this
           relocation into a cover. Abort and leave both records intact. */
        return;
    }
    if(!moveToZone(dragging,dest,dstZi))return;
    recordPlacement(dragging,dstMi,dstZi,owned?(RECT*)0:before);
    raiseWindowWithoutActivation(dragging);
}
static void detachAndRestore(HWND hwnd,const POINT *release,const POINT *begin,const RECT *snappedAtStart,int restore) {
    int slot=placeSlot(hwnd);
    if(slot<0)return;
    LZMember old=g_book.members[slot];
    forgetWindow(hwnd);
    if(!restore || !old.hasPreSnap || !release || !begin || !snappedAtStart)return;
    LZRect snap={snappedAtStart->left,snappedAtStart->top,
                 snappedAtStart->right,snappedAtStart->bottom};
    LZRect work=snap;
    int mi=monitorUnderCursor(*release);
    if(mi>=0)work=workRect(&g_displays[mi]);
    LZRect computed=lz_restored_bounds(old.preSnap,snap,work,
        begin->x,begin->y,release->x,release->y);
    SetWindowPos(hwnd,(HWND)0,computed.left,computed.top,
        computed.right-computed.left,computed.bottom-computed.top,
        SWP_NOZORDER|SWP_NOACTIVATE);
}

