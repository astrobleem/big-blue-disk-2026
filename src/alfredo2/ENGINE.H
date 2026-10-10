/* Alfredo: After Hours - picture engine. C89, MSC6 /G0 /AS small model.
 *
 * Tandy mode 9: 320x200, 16 colors, four interleaved 8K banks, two pixels a
 * byte with the left pixel in the high nibble. Two far 32K buffers use the
 * same bank layout as video memory, so any byte copies straight across:
 *   bg  - the scene's painted backdrop (drawn once per scene, plus the pond)
 *   frm - backdrop plus this frame's moving actors
 * Each frame restores last frame's actor rectangles from bg, draws actors
 * into frm while recording their bounding boxes, then copies last and this
 * frame's rectangles to the screen. Nothing else on screen is touched, so an
 * 8088 only moves the bytes that actually change.
 *
 * Fades, sunrise, strobes and water shimmer are palette changes, written in
 * vertical retrace.
 */
#define SW 320
#define SH 200
#define BUFSZ 32768U
static void audio_poll(void);  /* keeps music on time during long draws */
static unsigned char far *video;
static unsigned char far *bg,far *frm,far *dst;
static unsigned row_ofs[SH];
static unsigned long vram_bytes;

static void gfx_init(void)
{
    unsigned y;
    for(y=0;y<SH;y++)row_ofs[y]=(y&3)*8192+(y>>2)*160;
}

/* ---- dirty rectangles and actors -------------------------------------- */
/* A rectangle is a byte-column span and a row span, inclusive. Flags say
   whether it goes to the screen this picture (COMMIT) and whether bg must
   be copied back over it at the start of the next one (RESTORE). */
typedef struct { unsigned char c0,c1,y0,y1,fl; } Rect;
#define RF_COMMIT 1
#define RF_RESTORE 2
#define MAXR 80
static Rect rl[2][MAXR];
static unsigned rn[2],rcur,rect_peak,rect_merges;
static int bx0,by0,bx1,by1,track;
static void bbox_reset(void){bx0=SW;by0=SH;bx1=-1;by1=-1;}
static void grow(int x0,int y0,int x1,int y1)
{
    int t;
    if(!track)return;
    if(x0>x1){t=x0;x0=x1;x1=t;}
    if(y0>y1){t=y0;y0=y1;y1=t;}
    if(x0<bx0)bx0=x0;
    if(y0<by0)by0=y0;
    if(x1>bx1)bx1=x1;
    if(y1>by1)by1=y1;
}
static void push_rect(int x0,int y0,int x1,int y1,unsigned fl)
{
    Rect *r;unsigned n=rn[rcur];
    if(x0<0)x0=0;
    if(y0<0)y0=0;
    if(x1>SW-1)x1=SW-1;
    if(y1>SH-1)y1=SH-1;
    if(x0>x1 || y0>y1)return;
    if(n==MAXR) {
        /* Out of slots: widen the last rectangle. Still correct, just more bytes. */
        r=&rl[rcur][n-1];rect_merges++;
        if((x0>>1)<r->c0)r->c0=(unsigned char)(x0>>1);
        if((x1>>1)>r->c1)r->c1=(unsigned char)(x1>>1);
        if(y0<r->y0)r->y0=(unsigned char)y0;
        if(y1>r->y1)r->y1=(unsigned char)y1;
        r->fl|=(unsigned char)fl;
        return;
    }
    r=&rl[rcur][n];
    r->c0=(unsigned char)(x0>>1);r->c1=(unsigned char)(x1>>1);
    r->y0=(unsigned char)y0;r->y1=(unsigned char)y1;r->fl=(unsigned char)fl;
    rn[rcur]=n+1;
    if(n+1>rect_peak)rect_peak=n+1;
}
/* Something drawn into frm for this picture only. */
static void mark(int x0,int y0,int x1,int y1){push_rect(x0,y0,x1,y1,RF_COMMIT|RF_RESTORE);}

/* Actors with a key are left alone while their key stays the same: no
   erase, no redraw, no copy. A raver who only moves on the beat costs
   nothing between beats. Anything that disturbs the pixels under a kept
   actor (a restore, the pond) clears its key so it is drawn again. */
#define MAXACT 16
#define NOKEY 0xffffU
static Rect act_r[MAXACT];
static unsigned act_k[MAXACT];
static unsigned char act_live[MAXACT],act_seen[MAXACT];
static int act_id;
static unsigned long act_kept,act_drawn;
static int overlaps(const Rect *a,const Rect *b)
{
    return a->c0<=b->c1 && b->c0<=a->c1 && a->y0<=b->y1 && b->y0<=a->y1;
}
static void act_disturb(const Rect *r,int self)
{
    int i;
    for(i=1;i<MAXACT;i++)if(i!=self && act_live[i] && overlaps(r,&act_r[i]))act_k[i]=NOKEY;
}
static void act_forget(void)
{
    int i;
    for(i=0;i<MAXACT;i++){act_live[i]=0;act_k[i]=NOKEY;}
}
/* Ends one actor: its bounding box becomes a rectangle. */
static unsigned obj_n;
static void obj(void)
{
    Rect *r;
    if(bx1>=bx0) {
        if(act_id) {
            push_rect(bx0,by0,bx1,by1,RF_COMMIT);
            r=&act_r[act_id];
            r->c0=(unsigned char)((bx0<0?0:bx0)>>1);r->c1=(unsigned char)((bx1>SW-1?SW-1:bx1)>>1);
            r->y0=(unsigned char)(by0<0?0:by0);r->y1=(unsigned char)(by1>SH-1?SH-1:by1);
            act_live[act_id]=1;
        } else mark(bx0,by0,bx1,by1);
    }
    act_id=0;
    bbox_reset();
    if((++obj_n&3)==0)audio_poll();
}
#if defined(HOST) || defined(NO_ASM)
static void copy_rect(unsigned char far *to,unsigned char far *from,const Rect *r)
{
    unsigned y,w=(unsigned)r->c1-r->c0+1,o;
    for(y=r->y0;y<=r->y1;y++){o=row_ofs[y]+r->c0;_fmemcpy(to+o,from+o,w);}
}
#else
/* The same copy as one REP MOVSW per row with the bank step done in
   registers: a library call per row cost more than the bytes it moved. */
static void copy_rect(unsigned char far *to,unsigned char far *from,const Rect *r)
{
    unsigned w=(unsigned)r->c1-r->c0+1,o=row_ofs[r->y0]+r->c0,rows=(unsigned)r->y1-r->y0+1,ph=r->y0&3;
    _asm {
        push ds
        push si
        push di
        mov bx,o
        mov dx,rows
        mov ax,ph
        les di,to
        lds si,from
    cr_row:
        push si
        push di
        add si,bx
        add di,bx
        mov cx,w
        shr cx,1
        rep movsw
        adc cx,cx
        rep movsb
        pop di
        pop si
        inc ax
        test ax,3
        jz cr_wrap
        add bx,8192
        jmp cr_next
    cr_wrap:
        sub bx,24416
    cr_next:
        dec dx
        jnz cr_row
        pop di
        pop si
        pop ds
    }
}
#endif
static void frame_begin(void)
{
    unsigned i,p=rcur^1;Rect *r;
    rn[rcur]=0;
    for(i=0;i<rn[p];i++) {
        r=&rl[p][i];
        if(!(r->fl&RF_RESTORE))continue;
        copy_rect(frm,bg,r);act_disturb(r,0);
        push_rect(r->c0*2,r->y0,r->c1*2+1,r->y1,RF_COMMIT);
        if((i&7)==7)audio_poll();
    }
    for(i=0;i<MAXACT;i++)act_seen[i]=0;
    dst=frm;track=1;act_id=0;bbox_reset();
}
/* Before drawing actor id: returns 0 if it is unchanged and stays as it is. */
static int act_off;   /* host check: redraw every actor, compare pictures */
static unsigned long act_conflicts;
#ifndef ACT_NOTE
#define ACT_NOTE(a,b)
#endif
/* Rule for scenes: draw keyed actors before anonymous ones, and never let
   two keyed actors overlap (combine them). Erasing a changed actor must not
   wipe pixels already drawn this picture; the host check counts any case. */
static void act_check(const Rect *r,int id)
{
    unsigned i;
    for(i=1;i<MAXACT;i++)if((int)i!=id && act_seen[i] && act_live[i] && overlaps(r,&act_r[i]))
        {act_conflicts++;ACT_NOTE(id,(int)i);}
    for(i=0;i<rn[rcur];i++)if((rl[rcur][i].fl&RF_RESTORE) && overlaps(r,&rl[rcur][i]))
        {act_conflicts++;ACT_NOTE(id,-1);}
}
static int act(int id,unsigned key)
{
    Rect *r=&act_r[id];
    obj();
    act_seen[id]=1;
    if(act_live[id] && act_k[id]==key && !act_off){act_kept++;return 0;}
    if(act_live[id]) {
        act_check(r,id);
        copy_rect(frm,bg,r);act_disturb(r,id);
        push_rect(r->c0*2,r->y0,r->c1*2+1,r->y1,RF_COMMIT);
        act_live[id]=0;
    }
    act_k[id]=key;act_id=id;act_drawn++;
    return 1;
}
/* Pixels changed in bg and frm alike (the pond): to the screen once, and
   any actor standing there must be drawn again. */
static void mark_bg(int x0,int y0,int x1,int y1)
{
    Rect r;
    push_rect(x0,y0,x1,y1,RF_COMMIT);
    r.c0=(unsigned char)(x0>>1);r.c1=(unsigned char)(x1>>1);r.y0=(unsigned char)y0;r.y1=(unsigned char)y1;
    act_disturb(&r,0);
}
static unsigned long rect_area(const Rect *r)
{
    return (unsigned long)((unsigned)r->c1-r->c0+1)*((unsigned)r->y1-r->y0+1);
}
static void frame_commit(void)
{
    unsigned i;Rect *r;
    obj();track=0;
    /* Actors that were not drawn this picture have left the stage. */
    for(i=1;i<MAXACT;i++)if(act_live[i] && !act_seen[i]) {
        r=&act_r[i];copy_rect(frm,bg,r);act_live[i]=0;act_k[i]=NOKEY;
        push_rect(r->c0*2,r->y0,r->c1*2+1,r->y1,RF_COMMIT);
    }
    for(i=0;i<rn[rcur];i++) {
        r=&rl[rcur][i];
        if(r->fl&RF_COMMIT){copy_rect(video,frm,r);vram_bytes+=rect_area(r);}
        if((i&7)==7)audio_poll();
    }
    rcur^=1;
}
/* New backdrop: frm and the screen take all of bg; no stale actors remain. */
static void present_full(void)
{
    unsigned b;
    for(b=0;b<16;b++) {
        _fmemcpy(frm+(b>>2)*8192+(b&3)*2000,bg+(b>>2)*8192+(b&3)*2000,2000);
        _fmemcpy(video+(b>>2)*8192+(b&3)*2000,frm+(b>>2)*8192+(b&3)*2000,2000);
        audio_poll();
    }
    vram_bytes+=32000;rn[0]=rn[1]=0;act_forget();
}
static void begin_backdrop(void){dst=bg;track=0;}

/* ---- primitives on dst ----------------------------------------------- */
/* Clip window for actors that pass behind scenery (the DJ booth, a slot). */
static int clx0=0,cly0=0,clx1=SW-1,cly1=SH-1;
static void clip(int x0,int y0,int x1,int y1){clx0=x0;cly0=y0;clx1=x1;cly1=y1;}
static void noclip(void){clx0=0;cly0=0;clx1=SW-1;cly1=SH-1;}
static void px(int x,int y,unsigned c)
{
    unsigned char far *p;
    if(x<clx0 || x>clx1 || y<cly0 || y>cly1)return;
    p=dst+row_ofs[y]+(x>>1);
    *p=(unsigned char)((x&1)?(*p&0xf0)|c:(*p&0x0f)|(c<<4));
}
static void dot(int x,int y,unsigned c){grow(x,y,x,y);px(x,y,c);}
/* Horizontal run; a!=b gives a checkerboard dither of the two colors. */
static void span2(int x,int y,int w,unsigned a,unsigned b)
{
    unsigned char v;unsigned n;
    if(y<cly0 || y>cly1)return;
    if(x<clx0){w-=clx0-x;x=clx0;}
    if(x+w>clx1+1)w=clx1+1-x;
    if(w<=0)return;
    if(y&1){n=a;a=b;b=n;}
    if(x&1){px(x,y,b);x++;w--;}
    v=(unsigned char)((a<<4)|b);
    n=(unsigned)w>>1;
    if(n)_fmemset(dst+row_ofs[y]+(x>>1),v,n);
    if(w&1)px(x+w-1,y,a);
}
static void fill2(int x,int y,int w,int h,unsigned a,unsigned b)
{
    int i;
    grow(x,y,x+w-1,y+h-1);
    for(i=0;i<h;i++){span2(x,y+i,w,a,b);if((i&31)==31)audio_poll();}
}
static void fill(int x,int y,int w,int h,unsigned c){fill2(x,y,w,h,c,c);}
static void line(int x0,int y0,int x1,int y1,unsigned c)
{
    int dx,dy,sx,sy,e,t;unsigned char far *p;unsigned char hi=(unsigned char)(c<<4),lo=(unsigned char)c;
    grow(x0,y0,x1,y1);
    dx=x1>x0?x1-x0:x0-x1;dy=y1>y0?y0-y1:y1-y0;
    sx=x0<x1?1:-1;sy=y0<y1?1:-1;e=dx+dy;
    if(x0<clx0 || x1<clx0 || x0>clx1 || x1>clx1 || y0<cly0 || y1<cly0 || y0>cly1 || y1>cly1) {
        for(;;) {
            px(x0,y0,c);
            if(x0==x1 && y0==y1)break;
            t=e*2;
            if(t>=dy){e+=dy;x0+=sx;}
            if(t<=dx){e+=dx;y0+=sy;}
        }
        return;
    }
    /* Wholly inside the clip: write nibbles directly, no per-pixel call. */
    for(;;) {
        p=dst+row_ofs[y0]+(x0>>1);
        *p=(unsigned char)((x0&1)?(*p&0xf0)|lo:(*p&0x0f)|hi);
        if(x0==x1 && y0==y1)break;
        t=e*2;
        if(t>=dy){e+=dy;x0+=sx;}
        if(t<=dx){e+=dx;y0+=sy;}
    }
}
static void rect(int x,int y,int w,int h,unsigned c)
{
    line(x,y,x+w-1,y,c);line(x,y+h-1,x+w-1,y+h-1,c);
    line(x,y,x,y+h-1,c);line(x+w-1,y,x+w-1,y+h-1,c);
}
/* 64*sqrt(1-(k/64)^2): circle widths without square roots or longs. */
static const unsigned char circ64[65]={
 64,64,64,64,64,64,64,64,63,63,63,63,63,63,62,62,62,
 62,61,61,61,60,60,60,59,59,58,58,58,57,57,56,55,55,
 54,54,53,52,51,51,50,49,48,47,46,46,44,43,42,41,40,
 39,37,36,34,33,31,29,27,25,22,19,16,11,0};
/* Filled ellipse, rx by ry. */
static void oval2(int cx,int cy,int rx,int ry,unsigned a,unsigned b)
{
    int dy,w,k;
    grow(cx-rx,cy-ry,cx+rx,cy+ry);
    for(dy=-ry;dy<=ry;dy++) {
        k=ry?((dy<0?-dy:dy)*64)/ry:0;
        w=(rx*circ64[k])>>6;
        span2(cx-w,cy+dy,w*2+1,a,b);
        if((dy&15)==15)audio_poll();
    }
}
static void oval(int cx,int cy,int rx,int ry,unsigned c){oval2(cx,cy,rx,ry,c,c);}
static void disc(int cx,int cy,int r,unsigned c){oval2(cx,cy,r,r,c,c);}
static void ring(int cx,int cy,int r,unsigned c)
{
    int x=r,y=0,e=1-r;
    grow(cx-r,cy-r,cx+r,cy+r);
    while(x>=y) {
        px(cx+x,cy+y,c);px(cx-x,cy+y,c);px(cx+x,cy-y,c);px(cx-x,cy-y,c);
        px(cx+y,cy+x,c);px(cx-y,cy+x,c);px(cx+y,cy-x,c);px(cx-y,cy-x,c);
        y++;
        if(e<0)e+=2*y+1;
        else{x--;e+=2*(y-x)+1;}
    }
}
/* Ellipse outline from 16 chords (splash rings, orbits). */
static const int ring16x[16]={256,237,181,98,0,-98,-181,-237,-256,-237,-181,-98,0,98,181,237};
static void ellipse(int cx,int cy,int rx,int ry,unsigned c)
{
    int k,n;
    for(k=0;k<16;k++) {
        n=(k+1)&15;
        line(cx+((rx*ring16x[k])>>8),cy+((ry*ring16x[(k+12)&15])>>8),
             cx+((rx*ring16x[n])>>8),cy+((ry*ring16x[(n+12)&15])>>8),c);
    }
}
/* Filled triangle by scanlines: each edge steps in 16.16 fixed point, one
   long divide per edge instead of one per row. */
static void tri2(int x0,int y0,int x1,int y1,int x2,int y2,unsigned a,unsigned b)
{
    int t,y,xa,xb;long ea,eb,sa,sb;
    if(y0>y1){t=y0;y0=y1;y1=t;t=x0;x0=x1;x1=t;}
    if(y1>y2){t=y1;y1=y2;y2=t;t=x1;x1=x2;x2=t;}
    if(y0>y1){t=y0;y0=y1;y1=t;t=x0;x0=x1;x1=t;}
    grow(x0<x1?(x0<x2?x0:x2):(x1<x2?x1:x2),y0,x0>x1?(x0>x2?x0:x2):(x1>x2?x1:x2),y2);
    sa=y2>y0?((long)(x2-x0)<<16)/(y2-y0):0;
    sb=y1>y0?((long)(x1-x0)<<16)/(y1-y0):0;
    ea=((long)x0<<16)+32768L;eb=y1>y0?ea:((long)x1<<16)+32768L;
    for(y=y0;y<=y2 && y<SH;y++) {
        if(y==y1){sb=y2>y1?((long)(x2-x1)<<16)/(y2-y1):0;eb=((long)x1<<16)+32768L;}
        if(y>=0) {
            xa=(int)(ea>>16);xb=(int)(eb>>16);
            if(xa>xb){t=xa;xa=xb;xb=t;}
            span2(xa,y,xb-xa+1,a,b);
        }
        ea+=sa;eb+=sb;
        if((y&31)==31)audio_poll();
    }
}
static void tri(int x0,int y0,int x1,int y1,int x2,int y2,unsigned c){tri2(x0,y0,x1,y1,x2,y2,c,c);}

/* ---- text -------------------------------------------------------------- */
/* 5x7 font, ASCII 32..95 (from Radio Shack Rave), bit 4 is the left pixel. */
static const unsigned char font5[64][7]={
 {0,0,0,0,0,0,0},{4,4,4,4,4,0,4},{10,10,0,0,0,0,0},{10,31,10,10,31,10,0},
 {4,15,20,14,5,30,4},{24,25,2,4,8,19,3},{12,18,20,8,21,18,13},{4,4,8,0,0,0,0},
 {2,4,8,8,8,4,2},{8,4,2,2,2,4,8},{0,4,21,14,21,4,0},{0,4,4,31,4,4,0},
 {0,0,0,0,12,4,8},{0,0,0,31,0,0,0},{0,0,0,0,0,12,12},{1,1,2,4,8,16,16},
 {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},{31,2,4,2,1,17,14},
 {2,6,10,18,31,2,2},{31,16,30,1,1,17,14},{6,8,16,30,17,17,14},{31,1,2,4,8,8,8},
 {14,17,17,14,17,17,14},{14,17,17,15,1,2,12},{0,12,12,0,12,12,0},{0,12,12,0,12,4,8},
 {2,4,8,16,8,4,2},{0,0,31,0,31,0,0},{8,4,2,1,2,4,8},{14,17,1,2,4,0,4},
 {14,17,23,21,23,16,14},{14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
 {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},{14,17,16,23,17,17,15},
 {17,17,17,31,17,17,17},{14,4,4,4,4,4,14},{7,2,2,2,2,18,12},{17,18,20,24,20,18,17},
 {16,16,16,16,16,16,31},{17,27,21,21,17,17,17},{17,17,25,21,19,17,17},{14,17,17,17,17,17,14},
 {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},{15,16,16,14,1,1,30},
 {31,4,4,4,4,4,4},{17,17,17,17,17,17,14},{17,17,17,17,17,10,4},{17,17,17,21,21,21,10},
 {17,17,10,4,10,17,17},{17,17,10,4,4,4,4},{31,1,2,4,8,16,31},{14,8,8,8,8,8,14},
 {16,16,8,4,2,1,1},{14,2,2,2,2,2,14},{4,10,17,0,0,0,0},{0,0,0,0,0,0,31}
};
static const unsigned char *glyph(int ch)
{
    if(ch>='a' && ch<='z')ch-=32;
    if(ch<32 || ch>95)ch=32;
    return font5[ch-32];
}
static int text_w(const char *s,int sc)
{
    int n=0;
    while(s[n])n++;
    return n?n*6*sc-sc:0;
}
/* Transparent text at scale sc. rows: optional 7 colors, one per glyph row.
   Passes: drop shadow (offset sc/2+1), one-pixel outline, then face. */
#define NOC 255
static void text_pass(int x,int y,const char *s,int sc,int grow_px,int ox,int oy,
    unsigned c,const unsigned char *rows)
{
    int r,k,k0;const unsigned char *g;unsigned col,bits;
    if(sc==1 && !grow_px && !rows) {
        /* Plain small text: straight to pixels. */
        x+=ox;y+=oy;grow(x,y,x+text_w(s,1),y+6);
        for(;*s;s++,x+=6) {
            g=glyph(*s);
            if(*s==' ')continue;
            for(r=0;r<7;r++)if((bits=g[r])!=0)for(k=0;k<5;k++)if(bits&(16>>k))px(x+k,y+r,c);
        }
        return;
    }
    for(;*s;s++,x+=6*sc) {
        g=glyph(*s);
        audio_poll();
        for(r=0;r<7;r++) {
            col=rows?rows[r]:c;
            for(k=0;k<5;) {
                if(!(g[r]&(16>>k))){k++;continue;}
                for(k0=k;k<5 && (g[r]&(16>>k));k++);
                fill(x+k0*sc-grow_px+ox,y+r*sc-grow_px+oy,(k-k0)*sc+2*grow_px,sc+2*grow_px,col);
            }
        }
    }
}
static void textx(int x,int y,const char *s,int sc,unsigned c,unsigned outline,
    unsigned shadow,const unsigned char *rows)
{
    if(shadow!=NOC)text_pass(x,y,s,sc,outline!=NOC,sc>2?2:1,sc>2?2:1,shadow,0);
    if(outline!=NOC)text_pass(x,y,s,sc,1,0,0,outline,0);
    text_pass(x,y,s,sc,0,0,0,c,rows);
}
static void text(int x,int y,const char *s,unsigned c){textx(x,y,s,1,c,NOC,NOC,0);}
static int cx_of(const char *s,int sc){return (SW-text_w(s,sc))/2;}
/* Opaque small text, three whole bytes per glyph row: for text that is
   redrawn every picture (scrolling credits). x is rounded down to even. */
static void text_fast(int x,int y,const char *s,unsigned fg,unsigned bk)
{
    unsigned char lut[4];unsigned r,bits;const unsigned char *g;unsigned char far *p;int w;
    lut[0]=(unsigned char)(bk|(bk<<4));lut[1]=(unsigned char)((bk<<4)|fg);
    lut[2]=(unsigned char)((fg<<4)|bk);lut[3]=(unsigned char)(fg|(fg<<4));
    x&=~1;w=text_w(s,1)+1;
    if(x<0 || x+w>SW)return;
    grow(x,y,x+w,y+6);
    for(;*s;s++,x+=6) {
        g=glyph(*s);
        if(((x>>1)&15)==0)audio_poll();
        for(r=0;r<7;r++) {
            if((int)r+y<cly0 || (int)r+y>cly1)continue;
            bits=g[r];p=dst+row_ofs[y+r]+(x>>1);
            p[0]=lut[(bits>>3)&3];p[1]=lut[(bits>>1)&3];p[2]=lut[(bits&1)<<1];
        }
    }
}
static void ctext(int y,const char *s,int sc,unsigned c,unsigned outline,unsigned shadow,
    const unsigned char *rows)
{
    textx(cx_of(s,sc),y,s,sc,c,outline,shadow,rows);
}
/* Big titles take three passes of scaled runs, too slow to redo every
   frame on an 8088. The first time one is drawn over the still backdrop its
   finished bytes are kept; after that a frame just copies them back. */
#define TC_BYTES 2600
static unsigned char tc_buf[TC_BYTES];
static Rect tc_rect;
static int tc_ok;
static void ctext_cached(int y,const char *s,int sc,unsigned c,unsigned ol,unsigned sh)
{
    unsigned w,h,r,o;
    obj();
    if(tc_ok) {
        w=(unsigned)tc_rect.c1-tc_rect.c0+1;
        for(r=tc_rect.y0;r<=tc_rect.y1;r++){o=row_ofs[r]+tc_rect.c0;
            _fmemcpy(frm+o,(void far *)(tc_buf+(r-tc_rect.y0)*w),w);}
        mark(tc_rect.c0*2,tc_rect.y0,tc_rect.c1*2+1,tc_rect.y1);
        return;
    }
    ctext(y,s,sc,c,ol,sh,0);
    if(bx1>=bx0 && bx0>=0 && by0>=0 && bx1<SW && by1<SH) {
        tc_rect.c0=(unsigned char)(bx0>>1);tc_rect.c1=(unsigned char)(bx1>>1);
        tc_rect.y0=(unsigned char)by0;tc_rect.y1=(unsigned char)by1;
        w=(unsigned)tc_rect.c1-tc_rect.c0+1;h=(unsigned)tc_rect.y1-tc_rect.y0+1;
        if(w*h<=TC_BYTES) {
            for(r=0;r<h;r++){o=row_ofs[tc_rect.y0+r]+tc_rect.c0;
                _fmemcpy((void far *)(tc_buf+r*w),frm+o,w);}
            tc_ok=1;
        }
    }
    obj();
}
/* Rasterise a big title into the cache ahead of time (while the screen is
   still dark from the scene change), leaving frm as it was. */
static void tc_prepare(int y,const char *s,int sc,unsigned c,unsigned ol,unsigned sh)
{
    int t=track;unsigned char far *d=dst;
    dst=frm;track=1;bbox_reset();tc_ok=0;
    ctext_cached(y,s,sc,c,ol,sh);
    rn[rcur]--;                       /* that picture-rectangle was never shown */
    if(tc_ok){copy_rect(frm,bg,&tc_rect);act_disturb(&tc_rect,0);}
    dst=d;track=t;bbox_reset();
}
/* Caption card: black box with a colored rule, centred text. */
static void caption(int y,const char *s,unsigned c,unsigned rule)
{
    int w=text_w(s,1)+12,x=(SW-w)/2;
    fill(x,y,w,13,0);rect(x,y,w,13,rule);text(x+6,y+3,s,c);
}

/* ---- palette ------------------------------------------------------------ */
/* Logical colors map to the 16 fixed Tandy colors through palette registers
   (video array index 10h+n at 3DAh, value at 3DEh). Scenes set what they
   want; the fade level dims everything toward black. */
static unsigned char pal_want[16],pal_hw[16];
static int pal_fade,pal_flash;
static unsigned long pal_writes;
/* Four steps of darkening for each of the 16 colors, brightest first. */
static const unsigned char dimtab[16][4]={
 {0,0,0,0},{1,1,0,0},{2,2,0,0},{3,3,0,0},{4,4,0,0},{5,5,0,0},{6,6,0,0},{7,8,8,0},
 {8,8,0,0},{9,1,1,0},{10,2,2,0},{11,3,3,0},{12,4,4,0},{13,5,5,0},{14,6,6,0},{15,7,8,0}};
static void pal_set(unsigned i,unsigned c){pal_want[i&15]=(unsigned char)(c&15);}
static unsigned pal_out(unsigned i)
{
    if(pal_flash)return 15;
    if(pal_fade>=4)return 0;
    return dimtab[pal_want[i]][pal_fade<0?0:pal_fade];
}
