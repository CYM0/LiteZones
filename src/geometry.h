#ifndef LITEZONES_GEOMETRY_H
#define LITEZONES_GEOMETRY_H

typedef struct LZRect { int left, top, right, bottom; } LZRect;
#define LZ_MAX_ZONES 12
#define LZ_MAX_NODES 31
#define LZ_BUILTIN_COUNT 6
#define LZ_LAYOUT_CUSTOM 6
#define LZ_LAYOUT_COUNT 7
/* A binary partition: split=0 (leaf), 1 (left/right), 2 (top/bottom).
   Ratios are basis points (10000 = 100%), valid 1000..9000; subpixel precision before display rounding. */
typedef struct { int left, right, split, ratio; } LZNode;
typedef struct { int count; LZNode nodes[LZ_MAX_NODES]; } LZCustom;

int lz_compute_zones(LZRect work, int layout, int gap, LZRect out[LZ_MAX_ZONES]);
int lz_pick_zone(LZRect work, const LZRect *zones, int count, int x, int y);
void lz_custom_default(LZCustom *c);
int lz_custom_validate(const LZCustom *c);
int lz_custom_zones(LZRect work, const LZCustom *c, int gap, LZRect out[LZ_MAX_ZONES], int ids[LZ_MAX_ZONES]);
int lz_custom_split(LZCustom *c,int leafId,int orientation);
int lz_custom_merge(LZCustom *c,int leafId);
int lz_custom_set_ratio(LZCustom *c,int nodeId,int basisPoints);
/* Returns deepest matching divider node, with its exact parent rectangle. */
int lz_custom_divider(LZRect work,const LZCustom *c,int x,int y,int tolerance,LZRect *parent);
#endif
