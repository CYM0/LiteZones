#ifndef LZ_FRAME_MATH_H
#define LZ_FRAME_MATH_H
#include "geometry.h"
/* Pure geometry for the post-SetWindowPos DWM visible-frame correction. */
typedef struct {int left,top,right,bottom;} LZFrameDelta;
static inline LZFrameDelta lz_frame_delta(LZRect goal,LZRect actual) {
    LZFrameDelta d={goal.left-actual.left,goal.top-actual.top,
                    goal.right-actual.right,goal.bottom-actual.bottom};
    return d;
}
static inline int lz_frame_delta_small(LZFrameDelta d) {
    return d.left>=-24&&d.left<=24&&d.top>=-24&&d.top<=24 &&
           d.right>=-24&&d.right<=24&&d.bottom>=-24&&d.bottom<=24;
}
#endif
