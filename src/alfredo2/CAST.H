/* The cast: Alfredo (the classic white stick figure), ravers, the fish,
 * and props. Figures are posed from small tables and can be scaled and
 * rotated (he spends part of the episode going round a vortex).
 */
typedef struct { signed char a[8],l[8]; } Pose;
/* a: left elbow, left hand, right elbow, right hand (x,y from the shoulder)
   l: left knee, left foot, right knee, right foot (x,y from the hip) */
#define P_STAND 0
#define P_WALK0 1
#define P_DJ0 5
#define P_SIT 11
#define P_HOLD 12
#define P_WIND 13
#define P_CAST 14
#define P_REEL 15
#define P_LIFT 16
#define P_WAVE 17
#define P_FLAIL 18
#define P_PHONES 19
#define P_HATON 20
#define P_REACH 21
#define P_LOOK 22
#define P_SITBACK 23
#define P_COUNT 24
static const Pose poses[P_COUNT]={
 {{-4,6,-6,12,4,6,6,12},{-2,6,-3,12,2,6,3,12}},          /* stand */
 {{-3,6,-1,12,3,6,5,11},{-3,6,-6,12,3,5,5,11}},          /* walk 0..3 */
 {{-2,6,1,12,2,6,1,12},{0,6,-2,12,1,6,2,12}},
 {{3,6,5,11,-3,6,-1,12},{3,5,5,11,-3,6,-6,12}},
 {{2,6,1,12,-2,6,1,12},{1,6,2,12,0,6,-2,12}},
 {{-8,6,-13,11,8,6,13,11},{-2,6,-3,12,2,6,3,12}},        /* DJ: hands on decks */
 {{-8,6,-13,11,6,-5,8,-14},{-2,6,-3,12,2,6,3,12}},       /* fist pump */
 {{-8,-3,-9,-12,8,-3,9,-12},{-2,6,-3,12,2,6,3,12}},      /* raise the roof */
 {{-7,1,-4,-6,8,6,13,11},{-2,6,-3,12,2,6,3,12}},         /* hand on phones */
 {{-8,6,-13,11,7,-3,15,-8},{-2,6,-3,12,2,6,3,12}},       /* point at crowd */
 {{-7,-5,-11,-11,8,6,13,11},{-2,6,-3,12,2,6,3,12}},      /* wave */
 {{3,6,8,5,4,5,9,4},{6,-1,8,6,7,-1,10,6}},               /* sit, holding rod */
 {{4,6,9,4,5,5,10,3},{-2,6,-3,12,2,6,3,12}},             /* stand holding rod */
 {{-5,-2,-9,-7,-4,-3,-8,-8},{-3,6,-5,12,3,6,5,12}},      /* cast wind-up */
 {{6,-2,11,-2,6,-1,12,0},{-1,6,-4,12,4,6,7,12}},         /* cast forward */
 {{-6,3,-2,4,-5,4,-1,6},{3,6,6,12,6,5,10,11}},           /* reel, leaning back */
 {{-5,-5,-4,-14,5,-5,4,-14},{-3,6,-5,12,3,6,5,12}},      /* lift overhead */
 {{-4,6,-6,12,7,-4,9,-12},{-2,6,-3,12,2,6,3,12}},        /* wave goodbye */
 {{-7,-6,-11,-1,7,-6,12,-12},{-5,4,-9,8,5,5,8,10}},      /* flail */
 {{-6,0,-4,-7,6,0,4,-7},{-2,6,-3,12,2,6,3,12}},          /* both hands to head */
 {{-4,6,-6,12,6,-2,3,-10},{-2,6,-3,12,2,6,3,12}},        /* hat on */
 {{-4,6,-6,12,9,2,16,4},{-2,6,-3,12,2,6,3,12}},          /* reach */
 {{-4,6,-6,12,4,6,6,12},{-2,6,-3,12,2,6,3,12}},          /* look (head tilt) */
 {{-6,6,-9,4,6,6,9,4},{6,-1,8,6,7,-1,10,6}}              /* sit back, seen from behind */
};
/* 16-step circle, x1000. */
static const int sin16[16]={0,383,707,924,1000,924,707,383,0,-383,-707,-924,-1000,-924,-707,-383};
static const int sin256[16]={0,98,181,237,256,237,181,98,0,-98,-181,-237,-256,-237,-181,-98};
static int f_x,f_y,f_s,f_ang;   /* hip, scale/4, rotation step */
static void fpt(int dx,int dy,int *ox,int *oy)
{
    /* 16-bit only: |dx|,|dy| < 32 and scale <= 8, sines in 1/256. */
    int x=(dx*f_s)>>2,y=(dy*f_s)>>2,c,s,t;
    if(f_ang) {
        c=sin256[(f_ang+4)&15];s=sin256[f_ang&15];
        t=(x*c-y*s)>>8;y=(x*s+y*c)>>8;x=t;
    }
    *ox=f_x+x;*oy=f_y+y;
}
static void fseg(int x0,int y0,int x1,int y1,unsigned c)
{
    int a,b,d,e;
    fpt(x0,y0,&a,&b);fpt(x1,y1,&d,&e);line(a,b,d,e,c);
}
#define ACC_HAT 1
#define ACC_PHONES 2
#define ACC_NECK 4      /* headphones round the neck */
#define ACC_BACK 8      /* seen from behind: no face */
#define ACC_TILT 16
static int hand_x[2],hand_y[2];
static unsigned phones_c=13;
/* Alfredo (or anyone): hip at x,y; scale in quarters (4 = 24 px tall). */
static void fig(int x,int y,int pose,int scale,int ang,unsigned acc,unsigned c)
{
    const Pose *p=&poses[pose];int hx,hy,r,k,sx,sy,tilt=(acc&ACC_TILT)?2:0;
    f_x=x;f_y=y;f_s=scale;f_ang=ang&15;
    fseg(0,0,0,-10,c);                                   /* torso */
    fseg(0,-8,p->a[0],p->a[1]-8,c);fseg(p->a[0],p->a[1]-8,p->a[2],p->a[3]-8,c);
    fseg(0,-8,p->a[4],p->a[5]-8,c);fseg(p->a[4],p->a[5]-8,p->a[6],p->a[7]-8,c);
    fseg(0,0,p->l[0],p->l[1],c);fseg(p->l[0],p->l[1],p->l[2],p->l[3],c);
    fseg(0,0,p->l[4],p->l[5],c);fseg(p->l[4],p->l[5],p->l[6],p->l[7],c);
    fpt(p->a[2],p->a[3]-8,&hand_x[0],&hand_y[0]);fpt(p->a[6],p->a[7]-8,&hand_x[1],&hand_y[1]);
    fpt(tilt,-15,&hx,&hy);
    r=4*scale/4;ring(hx,hy,r,c);
    if(acc&ACC_HAT) {
        /* Bucket hat: brim, crown, and the lucky fly. */
        fseg(tilt-7,-19,tilt+7,-19,c);fseg(tilt-4,-19,tilt-3,-23,c);
        fseg(tilt+4,-19,tilt+3,-23,c);fseg(tilt-3,-23,tilt+3,-23,c);
        fpt(tilt+4,-21,&sx,&sy);dot(sx,sy,12);
    }
    if(acc&ACC_PHONES) {
        fseg(tilt-5,-17,tilt-3,-20,phones_c);fseg(tilt-3,-20,tilt+3,-20,phones_c);fseg(tilt+3,-20,tilt+5,-17,phones_c);
        for(k=-16;k<=-13;k++){fseg(tilt-6,k,tilt-5,k,phones_c);fseg(tilt+5,k,tilt+6,k,phones_c);}
    }
    if(acc&ACC_NECK) {
        fseg(-4,-10,0,-8,phones_c);fseg(0,-8,4,-10,phones_c);
        fseg(-5,-11,-4,-9,phones_c);fseg(4,-9,5,-11,phones_c);
    }
}
/* Where a pose puts a hand (0 left, 1 right), without drawing. */
static void fig_hand(int x,int y,int pose,int scale,int k,int *hx,int *hy)
{
    const Pose *p=&poses[pose];
    f_x=x;f_y=y;f_s=scale;f_ang=0;
    fpt(p->a[k*4+2],p->a[k*4+3]-8,hx,hy);
}
/* Glow-stick raver: a colored stick figure, arms up on the beat. */
static void raver(int x,int y,int up,int c)
{
    int arm=up?-14:-4;
    line(x,y,x,y-9,c);ring(x,y-12,2,c);
    line(x,y-7,x-4,y-7+arm/2,c);line(x-4,y-7+arm/2,x-5,y-7+arm,c);
    line(x,y-7,x+4,y-7+arm/2,c);line(x+4,y-7+arm/2,x+5,y-7+arm,c);
    line(x,y,x-3,y+8,c);line(x,y,x+3,y+8,c);
    obj();
}
/* The fish. s: size in pixels (half length); dir: 1 right, -1 left.
   Mutant flag gives the warm-water special: three eyes and a glow. */
static void fish(int x,int y,int s,int dir,unsigned body,unsigned fin,int mutant,int wag)
{
    int t=x-dir*s,h=s/2;
    tri(t,y,t-dir*(s/2),y-h+wag,t-dir*(s/2),y+h+wag,fin);
    oval(x,y,s,h,body);
    tri(x-dir*(s/4),y-h+1,x+dir*(s/4),y-h+1,x-dir*(s/3),y-h-s/3,fin);
    if(mutant) {
        disc(x+dir*(s*2/3),y-h/3,2,15);dot(x+dir*(s*2/3),y-h/3,0);
        disc(x+dir*(s/2),y-h/2-1,2,15);dot(x+dir*(s/2),y-h/2-1,0);
        disc(x+dir*(s*5/6),y-h/2,1,15);dot(x+dir*(s*5/6),y-h/2,0);
    } else {dot(x+dir*(s*2/3),y-h/3,15);}
    line(x+dir*(s-2),y+h/4,x+dir*(s-5),y+h/4,0);
}
static void chair(int x,int y,unsigned c)
{
    line(x-7,y,x+7,y-12,c);line(x+7,y,x-7,y-12,c);
    line(x-9,y-12,x+9,y-12,c);line(x-9,y-12,x-11,y-24,c);
}
static void bobber(int x,int y)
{
    fill(x-1,y-3,3,3,12);fill(x-1,y,3,2,15);dot(x,y-4,15);
}
/* Rod from the hand: straight, or bowed toward a pull point. */
static void rod(int hx,int hy,int tx,int ty,int bend,unsigned c)
{
    int mx=(hx+tx)/2,my=(hy+ty)/2+bend;
    line(hx,hy,mx,my,c);line(mx,my,tx,ty,c);
}
/* Speech balloon with a tail toward (px,py). */
static void balloon(int x,int y,const char *s,int px,int py)
{
    int w=text_w(s,1)+8;
    fill(x,y,w,12,15);rect(x,y,w,12,0);
    tri(x+w/2-3,y+11,x+w/2+3,y+11,px,py,15);
    text(x+4,y+3,s,0);
}
