#include "geometry.h"

static int clampi(int value, int lo, int hi) {
    if (value < lo) return lo;
    if (value > hi) return hi;
    return value;
}

static LZRect inset_cell(LZRect cell, LZRect work, int gap) {
    int h = gap / 2;
    int rem = gap - h;
    if (cell.left > work.left) cell.left += h;
    if (cell.top > work.top) cell.top += h;
    if (cell.right < work.right) cell.right -= rem;
    if (cell.bottom < work.bottom) cell.bottom -= rem;
    if (cell.right < cell.left) cell.right = cell.left;
    if (cell.bottom < cell.top) cell.bottom = cell.top;
    return cell;
}

int lz_compute_zones(LZRect work, int layout, int gap, LZRect out[LZ_MAX_ZONES]) {
    int w = work.right - work.left, h = work.bottom - work.top;
    int count = 0, i;
    if (!out || w <= 0 || h <= 0) return 0;
    layout = clampi(layout, 0, LZ_BUILTIN_COUNT - 1);
    gap = clampi(gap, 0, 40);
    if (layout <= 2) {
        count = layout + 2;
        for (i = 0; i < count; ++i) {
            LZRect r = { work.left + w * i / count, work.top,
                         work.left + w * (i+1) / count, work.bottom };
            out[i] = inset_cell(r, work, gap);
        }
    } else if (layout == 3) {
        int midX = work.left + w * 60 / 100;
        int midY = work.top + h / 2;
        LZRect raw[3] = {
            {work.left, work.top, midX, work.bottom},
            {midX, work.top, work.right, midY},
            {midX, midY, work.right, work.bottom}
        };
        count = 3;
        for (i = 0; i < count; ++i) out[i] = inset_cell(raw[i], work, gap);
    } else if (layout == 4) {
        int midX = work.left + w / 2;
        int midY = work.top + h / 2;
        LZRect raw[4] = {
            {work.left,work.top,midX,midY},
            {midX,work.top,work.right,midY},
            {work.left,midY,midX,work.bottom},
            {midX,midY,work.right,work.bottom}
        };
        count = 4;
        for (i = 0; i < count; ++i) out[i] = inset_cell(raw[i], work, gap);
    } else {
        LZRect raw[3] = {
            {work.left,work.top,work.left+w/4,work.bottom},
            {work.left+w/4,work.top,work.left+w*3/4,work.bottom},
            {work.left+w*3/4,work.top,work.right,work.bottom}
        };
        count=3;
        for(i=0;i<count;i++)out[i]=inset_cell(raw[i],work,gap);
    }
    return count;
}

int lz_pick_zone(LZRect work, const LZRect *zones, int count, int x, int y) {
    int i, best = -1;
    long long bestDist = 0;
    if (x < work.left || x >= work.right || y < work.top || y >= work.bottom) return -1;
    if (!zones || count < 1 || count > LZ_MAX_ZONES) return -1;
    for (i=0; i<count; i++) {
        const LZRect *r = &zones[i];
        long long dx = 0, dy = 0, dist;
        if (x < r->left) dx = (long long)r->left - x;
        if (x >= r->right) dx = (long long)x - r->right + 1;
        if (y < r->top) dy = (long long)r->top - y;
        if (y >= r->bottom) dy = (long long)y - r->bottom + 1;
        dist = dx*dx + dy*dy;
        if (best < 0 || dist < bestDist) { best = i; bestDist = dist; }
        if (dist == 0) return i;
    }
    return best;
}

/* Custom layouts use a constrained binary split tree, which ensures that
   every pixel belongs to exactly one raw zone before applying window gaps. */
static LZRect part_rect(LZRect r,int split,int ratio,int first) {
    if(split==1) {
        int mid=r.left+(int)(((long long)(r.right-r.left)*ratio)/10000);
        if(first) r.right=mid; else r.left=mid;
    } else {
        int mid=r.top+(int)(((long long)(r.bottom-r.top)*ratio)/10000);
        if(first) r.bottom=mid; else r.top=mid;
    }
    return r;
}
void lz_custom_default(LZCustom *c) {
    if(!c)return;
    for(int i=0;i<LZ_MAX_NODES;i++) c->nodes[i]=(LZNode){0,0,0,5000};
    c->count=5;
    c->nodes[0]=(LZNode){1,2,1,2500};
    c->nodes[2]=(LZNode){3,4,1,6667};
}
static int valid_visit(const LZCustom *c,int id,unsigned int *seen,int *leaves,int depth) {
    if(id<0||id>=c->count||depth>LZ_MAX_NODES||(*seen&(1u<<id)))return 0;
    *seen |= (1u<<id);
    const LZNode *n=&c->nodes[id];
    if(n->split==0){++*leaves;return *leaves<=LZ_MAX_ZONES;}
    if(n->split!=1&&n->split!=2)return 0;
    if(n->ratio<1000||n->ratio>9000)return 0;
    return valid_visit(c,n->left,seen,leaves,depth+1)&&valid_visit(c,n->right,seen,leaves,depth+1);
}
int lz_custom_validate(const LZCustom *c) {
    unsigned int seen=0;int leaves=0;
    return c&&c->count>0&&c->count<=LZ_MAX_NODES&&valid_visit(c,0,&seen,&leaves,0);
}
static int zones_visit(LZRect work,LZRect r,const LZCustom *c,int id,int gap,LZRect *out,int *ids,int count) {
    const LZNode *n=&c->nodes[id];
    if(!n->split) {
        if(count>=LZ_MAX_ZONES)return count;
        out[count]=inset_cell(r,work,gap);
        if(ids)ids[count]=id;
        return count+1;
    }
    count=zones_visit(work,part_rect(r,n->split,n->ratio,1),c,n->left,gap,out,ids,count);
    return zones_visit(work,part_rect(r,n->split,n->ratio,0),c,n->right,gap,out,ids,count);
}
int lz_custom_zones(LZRect work,const LZCustom *c,int gap,LZRect out[LZ_MAX_ZONES],int ids[LZ_MAX_ZONES]) {
    if(!out||work.right<=work.left||work.bottom<=work.top||!lz_custom_validate(c))return 0;
    return zones_visit(work,work,c,0,clampi(gap,0,40),out,ids,0);
}
static int leaves_count(const LZCustom *c) {
    LZRect out[LZ_MAX_ZONES];
    return lz_custom_zones((LZRect){0,0,1000,1000},c,0,out,0);
}
/* Compact before allocating so repeated edits never exhaust node slots. */
static int clone_compact(const LZCustom *old,LZCustom *fresh,int id) {
    int dst=fresh->count++;
    LZNode n=old->nodes[id];
    fresh->nodes[dst]=n;
    if(n.split) {
        int first=clone_compact(old,fresh,n.left);
        int second=clone_compact(old,fresh,n.right);
        fresh->nodes[dst].left=first;
        fresh->nodes[dst].right=second;
    }
    return dst;
}
static void compact(LZCustom *c) {
    LZCustom t={0};
    clone_compact(c,&t,0);
    *c=t;
}
int lz_custom_split(LZCustom *c,int leafId,int orientation) {
    if(!lz_custom_validate(c)||leafId<0||leafId>=c->count||c->nodes[leafId].split ||(orientation!=1&&orientation!=2))return 0;
    if(leaves_count(c)>=LZ_MAX_ZONES)return 0;
    /* Preserve the selected index by splitting first and compacting afterwards. */
    if(c->count+2>LZ_MAX_NODES) {
        compact(c);
        /* Caller-provided IDs cannot survive compaction; all reachable trees
           with <=12 leaves use <=23 nodes, so this path is not needed. */
        return 0;
    }
    int a=c->count++,b=c->count++;
    c->nodes[a]=(LZNode){0,0,0,5000};c->nodes[b]=c->nodes[a];
    c->nodes[leafId]=(LZNode){a,b,orientation,5000};
    return 1;
}
static int find_parent(const LZCustom *c,int child) {
    for(int i=0;i<c->count;i++) if(c->nodes[i].split&&(c->nodes[i].left==child||c->nodes[i].right==child))return i;
    return -1;
}
int lz_custom_merge(LZCustom *c,int leafId) {
    if(!lz_custom_validate(c)||leafId<0||leafId>=c->count||c->nodes[leafId].split)return 0;
    int parent=find_parent(c,leafId);
    if(parent<0)return 0;
    int sibling=c->nodes[parent].left==leafId?c->nodes[parent].right:c->nodes[parent].left;
    if(c->nodes[sibling].split)return 0;
    c->nodes[parent]=(LZNode){0,0,0,5000};
    compact(c);return 1;
}
int lz_custom_set_ratio(LZCustom *c,int nodeId,int basisPoints) {
    if(!lz_custom_validate(c)||nodeId<0||nodeId>=c->count||!c->nodes[nodeId].split)return 0;
    c->nodes[nodeId].ratio=clampi(basisPoints,1000,9000);return 1;
}
static int divider_visit(LZRect r,const LZCustom *c,int id,int x,int y,int tol,LZRect *out) {
    const LZNode *n=&c->nodes[id];
    if(!n->split)return -1;
    LZRect a=part_rect(r,n->split,n->ratio,1),b=part_rect(r,n->split,n->ratio,0);
    int match=n->split==1 ? (x>=r.left&&x<r.right&&y>=r.top&&y<r.bottom && x>=a.right-tol && x<=a.right+tol)
                           : (x>=r.left&&x<r.right&&y>=r.top&&y<r.bottom && y>=a.bottom-tol&&y<=a.bottom+tol);
    int inner=divider_visit(a,c,n->left,x,y,tol,out);
    if(inner>=0)return inner;
    inner=divider_visit(b,c,n->right,x,y,tol,out);
    if(inner>=0)return inner;
    if(match){*out=r;return id;}
    return -1;
}
int lz_custom_divider(LZRect work,const LZCustom *c,int x,int y,int tolerance,LZRect *parent) {
    if(!lz_custom_validate(c)||!parent)return -1;
    return divider_visit(work,c,0,x,y,tolerance,parent);
}
