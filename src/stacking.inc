/* preserve physical ordering without turning Z-order into a swap
   precondition. The window the user actively drags remains in front; the
   passive participant goes immediately behind it ("top - 1"), with no
   activation/focus change. A special always-on-top mismatch is best effort:
   never silently change another app's topmost setting. */
static BOOL raiseWindowWithoutActivation(HWND hwnd) {
    return SetWindowPos(hwnd,(HWND)0,0,0,0,0,
        SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
}
static BOOL restackSwappedOwners(HWND displaced,HWND dragging) {
    if(displaced==dragging || !IsWindow(displaced) || !IsWindow(dragging))return 0;
    /* Both HWNDs in the same normal/topmost Z-order band: first make the
       user-dragged window the frontmost member of that band, then anchor the
       passive window directly BEHIND its actual HWND, not a guessed index. */
    int passiveTop=(GetWindowLongPtrW(displaced,GWL_EXSTYLE)&WS_EX_TOPMOST)!=0;
    int draggingTop=(GetWindowLongPtrW(dragging,GWL_EXSTYLE)&WS_EX_TOPMOST)!=0;
    if(passiveTop==draggingTop) {
        if(!raiseWindowWithoutActivation(dragging))return 0;
        if(SetWindowPos(displaced,dragging,0,0,0,0,
            SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE))return 1;
        /* Fallback if Windows refuses a relative anchor: try the ordinary
           front-of-band ordering once. Never roll back the geometry swap. */
        raiseWindowWithoutActivation(displaced);
        return raiseWindowWithoutActivation(dragging);
    }
    /* Cross-band / special windows cannot be guaranteed adjacent without
       altering WS_EX_TOPMOST. Do not turn that limitation into a cancelled
       geometry swap. Keep their native priority and do the best we can. */
    raiseWindowWithoutActivation(displaced);
    return raiseWindowWithoutActivation(dragging);
}

