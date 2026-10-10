/* "Alfredo: After Hours" - the episode.
 *
 * 4:00 AM at the Radio Shack Rave. DJ Alfredo plays the last set, clocks out,
 * and walks past the MOS Labs data center to his favorite fishing spot: the
 * warm-water OUTFLOW (last month the same plant's INTAKE nearly had him).
 * The pond is a live Flow-88 fluid simulation. At 5:00 AM the training run
 * finishes and the plant flushes its heat.
 *
 * Each scene paints a backdrop once (bg_*) and draws its actors every frame
 * (fg_*). Scene time su is in 1/16 BIOS tick; DS(n) is n tenths of a second.
 */
#define DS(n) ((unsigned)((n)*2913UL/100UL))
#define SC_TITLE 0
#define SC_RAVE 1
#define SC_LAST 2
#define SC_CLOCK 3
#define SC_WALK 4
#define SC_SPOT 5
#define SC_CAST 6
#define SC_POND 7
#define SC_FIGHT 8
#define SC_CATCH 9
#define SC_ALARM 10
#define SC_SURGE 11
#define SC_CALM 12
#define SC_CREDITS 13
#define SC_END 14
#define SC_FISH 15
#define SC_COUNT 16
#define BG_TITLE 0
#define BG_CLUB 1
#define BG_CLOCK 2
#define BG_STREET 3
#define BG_SIDE 4
#define BG_TOP 5
#define BG_LABS 6
#define BG_CREDITS 7
#define BG_END 8
typedef struct { unsigned ticks;signed char cue;unsigned char bgid,cue_end; } Scene;
static const Scene scenes[SC_COUNT]={
 {109,CUE_STING,BG_TITLE,0},  {273,CUE_RAVE,BG_CLUB,0},     {146,CUE_LIGHTS,BG_CLUB,0},
 {118,CUE_PUNCH,BG_CLOCK,0},  {291,CUE_WALK,BG_STREET,0},   {164,-1,BG_SIDE,0},
 {91,CUE_POND,BG_SIDE,0},     {437,-1,BG_TOP,0},            {182,CUE_TENSION,BG_SIDE,0},
 {146,CUE_FANFARE,BG_SIDE,0}, {127,CUE_ALARM,BG_LABS,0},    {328,CUE_SURGE,BG_TOP,130},
 {200,CUE_TROMBONE,BG_SIDE,0},{364,CUE_CREDITS,BG_CREDITS,0},{0,-1,BG_END,0},
 {0,CUE_POND,BG_TOP,0}};
typedef struct { unsigned char sc,ds,id; } SfxCue;
static const SfxCue sfxq[]={
 {SC_LAST,0,SFX_SCRATCH},{SC_CLOCK,45,SFX_CHUNK},{SC_SPOT,48,SFX_PLOP},
 {SC_CAST,20,SFX_WHOOSH},{SC_CAST,30,SFX_PLOP},
 {SC_POND,140,SFX_PLOP},{SC_POND,165,SFX_PLOP},{SC_POND,200,SFX_SPLASH},
 {SC_FIGHT,8,SFX_REEL},{SC_FIGHT,35,SFX_SPLASH},{SC_FIGHT,52,SFX_REEL},
 {SC_FIGHT,70,SFX_SPLASH},{SC_FIGHT,97,SFX_THUD},
 {SC_CATCH,15,SFX_FLASH},{SC_CATCH,66,SFX_SPLASH},
 {SC_ALARM,20,SFX_KLAXON},{SC_ALARM,31,SFX_KLAXON},{SC_ALARM,42,SFX_KLAXON},
 {SC_SURGE,0,SFX_SURGE},{SC_SURGE,32,SFX_SPLASH},{SC_SURGE,128,SFX_GLUG},
 {SC_CALM,96,SFX_BLIP}};
#define NSFXQ (sizeof(sfxq)/sizeof(sfxq[0]))

static unsigned su;
static unsigned long E,sc_start;
static int scene_no=-1,scene_bg=-1;
static unsigned long beat_len=2184;       /* 128 BPM, fine units per beat */
static int lerp(unsigned a,unsigned b,int v0,int v1)
{
    if(su<=a)return v0;
    if(su>=b)return v1;
    /* 16-bit when the product fits (almost always): long math is a
       library call on the 8088. */
    {unsigned d=b-a,t=su-a,q;int r=v1-v0;unsigned ar=(unsigned)(r<0?-r:r);
     if(ar<1024) {
         while(t>255 || (ar>255 && t>63)){t>>=1;d>>=1;}
         q=d?ar*t/d:ar;
         return v0+(r<0?-(int)q:(int)q);
     }}
    return v0+(int)((long)(v1-v0)*(long)(su-a)/(long)(b-a));
}
static int during(unsigned a,unsigned b){return su>=a && su<b;}
/* True on the first picture at or after scene time t (pictures come every
   24 units, so exact equality would usually miss). */
static unsigned su_prev;
static int crossed(unsigned t){return su>=t && su_prev<t;}
#define SIN(n,k) (((n)*sin256[(k)&15])>>8)
/* h*4*t*(n-t) passes 32767 for a tall arc: long here, it is rare. */
static int arc_y(int t,int n,int h){return (int)((long)h*4*t*(n-t)/((long)n*n));}
static unsigned seed;
/* 16-bit generator: identical on the 8088 and in the host check. */
static unsigned rnd(unsigned n){seed=(seed*25173U+13849U)&0xffffU;return (seed>>4)%n;}
static void pal_load(const unsigned char *p)
{
    unsigned i;
    for(i=0;i<16;i++)pal_want[i]=p[i];
}
static void stars(int n,int y1,unsigned a,unsigned b)
{
    int k,x,y;
    for(k=0;k<n;k++){x=(int)rnd(SW);y=(int)rnd((unsigned)y1);px(x,y,k%3==0?15:(k&1?a:b));}
}
/* Fine time when the music/half-beat clock of the current scene began. */
static unsigned long scene_fine(void){return E-sc_start;}

/* ================================================================ title */
static const unsigned char pal_title[16]={0,0,0,0,0,1,0,8,7,0,10,11,6,13,14,15};
static const unsigned char title_rows[7]={2,2,3,3,4,4,4};
static void bg_title(void)
{
    seed=7;stars(70,200,7,8);
    ctext(18,"BIG BLUE DISK 2026",1,1,NOC,NOC,0);
    ctext(29,"PRESENTS",1,1,NOC,NOC,0);
    ctext(48,"ALFREDO",5,2,5,NOC,title_rows);
    ctext(96,"EPISODE 2",1,6,NOC,NOC,0);
    ctext(108,"AFTER HOURS",3,9,6,NOC,0);
    /* The man himself: hat on, phones round his neck, rod in hand. */
    fig(160,170,P_HOLD,4,0,ACC_HAT|ACC_NECK,15);
    rod(hand_x[1],hand_y[1],hand_x[1]+26,hand_y[1]-26,0,12);
    line(0,182,319,182,8);
}
static void fg_title(void)
{
    pal_load(pal_title);
    if(su>=DS(4))pal_set(1,9);
    if(su>=DS(8)){pal_set(2,15);pal_set(3,11);pal_set(4,9);pal_set(5,1);}
    if(during(DS(8),DS(9))){pal_set(2,15);pal_set(3,15);pal_set(4,15);}
    if(su>=DS(25)){pal_set(6,13);pal_set(9,14);}
    pal_set(7,(su>>5)&1?8:15);pal_set(8,(su>>5)&1?15:7);
}

/* ================================================================= club */
/* 1-4 floor tiles, 5/6 beams, 9 mirror-ball sparkle, 10 speaker cones,
   11 walls (black until the house lights come up). */
static const unsigned char pal_club[16]={0,13,9,14,10,5,1,7,8,15,7,0,12,13,14,15};
static void bg_club(void)
{
    int r,i,y0,y1,xa,xb,xc,xd;
    fill(0,0,SW,150,11);
    /* Light beams from the truss, dithered so they read as haze. */
    tri2(40,12,64,150,104,150,5,11);tri2(280,12,216,150,256,150,5,11);
    tri2(40,12,120,150,150,150,6,11);tri2(280,12,170,150,200,150,6,11);
    line(0,10,319,10,8);line(0,12,319,12,8);
    for(i=0;i<10;i++)fill(14+i*32,13,6,4,7);
    /* Mirror ball. */
    line(160,12,160,20,7);oval2(160,30,10,10,8,7);
    for(i=0;i<9;i++){px(153+i*2,26+(i*5)%9,9);px(152+(i*7)%16,30+(i&3),9);}
    /* Banner. */
    ctext(44,"RADIO SHACK RAVE",2,14,12,NOC,0);
    /* Speakers with cones. */
    for(i=0;i<2;i++) {
        r=i?272:8;
        fill(r,72,40,78,8);rect(r,72,40,78,0);
        disc(r+20,92,10,0);disc(r+20,92,7,10);disc(r+20,92,2,0);
        disc(r+20,126,14,0);disc(r+20,126,10,10);disc(r+20,126,3,0);
    }
    /* Wall clock. */
    fill(232,72,32,13,0);rect(232,72,32,13,7);text(236,75,"4:00",12);
    /* Dance floor: four rows of tiles in perspective, palette-cycled. */
    for(r=0;r<4;r++) {
        y0=150+r*(4+r*2)+r*4;y1=150+(r+1)*(4+(r+1)*2)+(r+1)*4;
        if(y1>199)y1=199;
        for(i=-5;i<5;i++) {
            xa=160+i*(36+r*10);xb=xa+36+r*10;
            xc=160+i*(36+(r+1)*10);xd=xc+36+(r+1)*10;
            tri(xa,y0,xb,y0,xc,y1,1+((i+r+10)&3));
            tri(xb,y0,xc,y1,xd,y1,1+((i+r+10)&3));
            line(xa,y0,xc,y1,0);
        }
        line(0,y0,319,y0,0);
    }
    /* DJ booth: decks, mixer and a Tandy 1000 doing the visuals. */
    fill(96,104,128,32,8);rect(96,104,128,32,7);
    textx(124,114,"DJ ALFREDO",1,9,NOC,NOC,0);
    fill(92,100,136,4,7);
    oval(122,98,14,3,0);ring(122,98,2,7);oval(198,98,14,3,0);ring(198,98,2,7);
    fill(100,84,22,13,7);fill(102,86,18,9,0);fill(104,88,6,2,13);fill(110,90,6,2,11);
    fill(98,97,26,3,7);
}
static const unsigned char dj_seq[16]={0,1,0,1,0,3,0,3,2,2,4,4,5,0,5,0};
static int beat_of(unsigned long f){return (int)(f/beat_len);}
static int bob_of(unsigned long f){return (f%beat_len)<beat_len/4?2:0;}
/* Keyed actors (see act()): ids are per scene. */
#define A_DJ 1
#define A_CROWD 2       /* ..9 */
#define A_BALLOON 10
#define A_CAPTION 11
#define A_PROP 12
#define A_PROP2 13
#define A_HERO 14
#define A_CHAIR 15
static void dj(int pose,int bob,unsigned acc,int say)
{
    if(!act(A_DJ,(unsigned)(pose*64+bob*16+(int)acc+say*8)))return;
    clip(0,0,SW-1,103);
    fig(160,96+bob,pose,4,0,acc,15);
    noclip();
    if(say)balloon(176,60,"GOOD NIGHT, RAVERS!",166,76);
    obj();
}
static const unsigned char floor_cols[4]={13,9,14,10};
static void club_lights(int beat)
{
    int k;
    for(k=0;k<4;k++)pal_set(1+k,floor_cols[(k+beat)&3]);
    pal_set(5,(beat>>1)&1?13:11);pal_set(6,beat&1?14:5);
    pal_set(9,(beat*3)&1?15:7);
}
static void fg_rave(void)
{
    unsigned long f=scene_fine();int beat=beat_of(f),half=(int)(f*2/beat_len),k,up,x,y;
    pal_load(pal_club);club_lights(beat);
    pal_set(10,(f%beat_len)<beat_len/4?15:7);
    dj(P_DJ0+dj_seq[half&15],bob_of(f),ACC_PHONES,0);
    for(k=0;k<8;k++) {
        up=((beat+k)&1) && beat>=4;
        x=26+k*38+((k*7)&7);y=182+(k&1)*7-(up?3:0);
        if(act(A_CROWD+k,(unsigned)up))raver(x,y,up,12+(k%3));
    }
    /* The lead comes in at bar 5: LETS GO! */
    if(f>=64UL*546 && f<80UL*546 && (half&1)==0) {
        ctext_cached(16,"LETS GO!",3,14,12,0);
    }
}
static void fg_last(void)
{
    int k,x,hx,hy,t;
    pal_load(pal_club);club_lights(31);
    /* House lights: beams and the floor go dead, the walls come up. */
    if(su>=DS(7)) {
        pal_set(5,lerp(DS(7),DS(12),1,0)?8:11);pal_set(6,0);
        for(k=1;k<=4;k++)pal_set(k,su>=DS(10)?7:8);
        pal_set(11,su>=DS(10)?8:0);pal_set(9,7);pal_set(5,0);
    }
    if(su<DS(30))dj(P_DJ0+3,0,ACC_PHONES,0);
    else if(su<DS(40))dj(P_PHONES,0,ACC_PHONES,0);
    else if(su<DS(45))dj(P_STAND,0,ACC_NECK,0);
    else if(su<DS(60)) {
        dj(P_HATON,0,ACC_NECK|(su>=DS(55)?ACC_HAT:0),0);
        if(su<DS(55)) {
            /* The hat comes out from under the decks. */
            t=lerp(DS(45),DS(55),0,16);hx=200-40*t/16;hy=98-21*t/16-arc_y(t,16,24);
            line(hx-7,hy,hx+7,hy,15);rect(hx-4,hy-4,9,5,15);obj();
        }
    }
    else {
        dj(P_WAVE,0,ACC_NECK|ACC_HAT,1);
    }
    /* The crowd drifts out to the right. */
    for(k=0;k<8;k++) {
        x=26+k*38+((k*7)&7)+lerp(DS(12+k*3),DS(30+k*3),0,340);
        if(x<330)raver(x,182+(k&1)*7-(int)((su>>3)&1),0,su>=DS(10)?8:12+(k%3));
    }
    if(su>=DS(20) && act(A_CAPTION,1)){caption(4,"4:12 AM. LAST CALL.",15,12);obj();}
}

/* ============================================================ time clock */
static const unsigned char pal_clock[16]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
static const int sin60[16]={0,105,208,309,407,500,588,669,743,809,866,914,951,978,995,1000};
static int s60(int k)
{
    k%=60;if(k<0)k+=60;
    if(k<=15)return sin60[k];
    if(k<=30)return sin60[30-k];
    if(k<=45)return -sin60[k-30];
    return -sin60[60-k];
}
#define CLK_Y 58
static void hand(int k,int len,unsigned c)
{
    line(160,CLK_Y,160+len*s60(k)/1000,CLK_Y-len*s60(k+15)/1000,c);
}
static void bg_clock(void)
{
    int k;
    fill2(0,0,SW,SH,1,0);
    for(k=0;k<SH;k+=16)line(0,k,319,k,0);
    fill(92,16,136,166,7);rect(92,16,136,166,8);rect(94,18,132,162,15);
    disc(160,CLK_Y,32,15);ring(160,CLK_Y,32,0);ring(160,CLK_Y,33,8);
    for(k=0;k<60;k+=5)line(160+28*s60(k)/1000,CLK_Y-28*s60(k+15)/1000,
        160+31*s60(k)/1000,CLK_Y-31*s60(k+15)/1000,0);
    fill(130,124,60,8,0);rect(128,122,64,12,8);
    ctext(142,"PUNCH-O-MATIC",1,8,NOC,NOC,0);
    ctext(156,"INSERT CARD",1,4,NOC,NOC,0);
    /* The other night owls' cards. */
    fill(250,40,56,120,6);rect(250,40,56,120,0);
    for(k=0;k<4;k++){fill(256,46+k*28,44,22,15);text(258,52+k*28,k==0?"DOT":k==1?"BOB":k==2?"MEL":"",0);}
}
static void card(int x,int y,int stamped)
{
    fill(x,y,40,44,15);rect(x,y,40,44,8);
    text(x+3,y+4,"ALFREDO",0);line(x+3,y+13,x+36,y+13,1);
    text(x+3,y+16,"DJ",1);
    if(stamped){text(x+5,y+26,"04:17",12);text(x+5,y+35,"OUT",12);}
}
static void fg_clock(void)
{
    int x,y;
    pal_load(pal_clock);
    if(act(A_PROP,su/DS(10))){hand(20+17/12,18,0);hand(17,26,0);hand((int)(su/DS(10)),28,12);
        disc(160,CLK_Y,2,0);obj();}
    if(su<DS(46)) {
        x=lerp(DS(0),DS(15),330,140);y=lerp(DS(0),DS(15),60,74);
        if(su>=DS(15))y=lerp(DS(18),DS(43),74,118);
        clip(0,0,SW-1,123);card(x,y,0);noclip();
        /* His hand: thumb and fingers on the card, arm off to the right. */
        if(su<DS(43)){fill(x+30,y+14,14,10,15);rect(x+30,y+14,14,10,0);
            line(x+44,y+16,330,y+6,15);line(x+44,y+23,330,y+14,15);}
        obj();
    } else {
        y=lerp(DS(48),DS(53),118,70);
        if(su>=DS(48)){card(140,y,1);obj();}
        if(su>=DS(52) && act(A_CAPTION,1)){caption(186,"SHIFT OVER.",14,6);obj();}
    }
    if(during(DS(45),DS(47)))pal_set(15,14);
}

/* ================================================================ street */
/* 1 sky, 3/4 stars, 5/6 rack LEDs, 9 antenna beacon, 10 sign. */
static const unsigned char pal_street[16]={0,1,1,7,15,10,6,7,8,12,10,11,12,2,14,15};
static void bg_street(void)
{
    int k;
    fill(0,0,SW,48,1);fill2(0,48,SW,40,1,0);
    seed=99;stars(60,90,3,4);
    disc(40,28,10,15);disc(45,25,9,1);
    tri2(-40,170,60,118,160,170,8,0);tri2(120,170,230,128,330,170,8,0);
    /* The club, dark now. */
    fill(0,104,72,66,8);rect(0,104,72,66,7);
    fill(6,112,28,22,0);fill(40,112,26,22,0);fill(26,140,20,30,0);
    text(10,96,"RADIO SHACK",7);
    textx(14,114,"RAVE",2,8,7,NOC,0);
    /* Streetlight and its pool of light. */
    line(116,92,116,170,7);line(116,92,128,92,7);fill(124,92,10,3,14);
    tri2(129,95,104,170,156,170,14,0);
    /* MOS Labs: the AI data center from last month. */
    fill(180,66,124,104,8);rect(180,66,124,104,7);
    for(k=0;k<12;k++){fill(188+(k%6)*19,84+(k/6)*20,13,12,0);
        fill(190+(k%6)*19,86+(k/6)*20,3,2,5);fill(195+(k%6)*19,90+(k/6)*20,3,2,13);fill(190+(k%6)*19,93+(k/6)*20,5,1,5);}
    textx(186,48,"MOS LABS",2,10,0,NOC,0);
    text(200,128,"AI DATA CENTER",11);
    fill(196,138,92,12,0);rect(196,138,92,12,7);
    /* Cooling tower and antenna. */
    tri(276,66,300,66,284,40,7);tri(276,66,284,40,272,40,7);fill(272,38,30,3,8);
    line(190,66,190,40,7);
    /* The pipe runs off toward the pond. */
    fill(296,156,24,8,7);line(296,156,319,156,15);line(296,163,319,163,8);
    text(266,176,"TO POND",7);
    /* Road. */
    fill(0,170,SW,30,8);line(0,170,319,170,7);
    for(k=0;k<8;k++)fill(k*44,184,22,2,14);
    text(4,190,"4:30 AM",14);
}
static void fg_walk(void)
{
    int x,pose,hip=158,step=(int)((su/DS(3))&3),k,t;
    pal_load(pal_street);
    pal_set(3,(su>>4)&1?15:7);pal_set(4,(su>>4)&1?7:15);
    pal_set(5,(su>>3)&1?10:2);pal_set(13,(su>>3)&1?2:10);phones_c=12;
    pal_set(9,(su>>5)&1?12:4);
    if(act(A_PROP,1)){fill(186,38,5,3,9);obj();}
    /* Training progress ticker. */
    t=lerp(0,DS(160),95,99);
    if(act(A_PROP2,(unsigned)t)){char s[20];sprintf(s,"TRAINING %d%%",t);fill(198,140,88,8,0);text(200,141,s,10);obj();}
    if(su<DS(85)){x=lerp(0,DS(85),-12,232);pose=P_WALK0+step;}
    else if(su<DS(105)){x=232;pose=P_LOOK;}
    else{x=lerp(DS(105),DS(160),232,340);pose=P_WALK0+step;}
    if(pose!=P_LOOK && (step&1))hip--;
    /* MOS Labs' camera keeps an eye on him. */
    k=x<140?0:(x<200?1:2);
    if(act(A_BALLOON,(unsigned)k)) {
        line(176,74,179,74,7);fill(168,71,8,5,7);
        if(k==0)line(167,74,162,77,12);
        else if(k==1)line(171,76,171,80,12);
        else line(176,76,180,80,12);
        obj();
    }
    fig(x,hip,pose,4,0,ACC_HAT|ACC_NECK|(pose==P_LOOK?ACC_TILT:0),15);
    /* Rod over the shoulder, tackle box in hand. */
    line(x+1,hip-10,x-14,hip-40,6);
    fill(hand_x[0]-3,hand_y[0],7,5,6);obj();
    if(pose==P_LOOK){balloon(x-8,hip-52,"?",x+2,hip-34);obj();}
    /* Cooling tower steam. */
    for(k=0;k<3;k++) {
        t=(int)((su+(unsigned)k*90)%270);
        oval2(286+t/30-k*3,36-t/6,3+t/40,2+t/60,7,0);obj();
    }
}

/* =========================================================== pond, side */
/* 1-3 sky top/mid/horizon, 4 water, 5 shimmer, 9 pipe stream, 13 sun,
   12 red (bobber, and the fish's glow). */
static const unsigned char pal_side[16]={0,0,1,5,1,9,6,7,8,11,2,11,12,0,14,15};
#define WATER_Y 118
static void bg_side(void)
{
    int k;
    fill(0,0,SW,36,1);fill2(0,36,SW,14,1,2);fill(0,50,SW,22,2);fill2(0,72,SW,12,2,3);fill(0,84,SW,34,3);
    disc(250,96,16,13);
    tri(-20,104,40,88,110,104,8);tri(60,104,140,84,220,104,8);tri(180,104,260,90,340,104,8);
    fill(0,100,SW,18,8);
    /* MOS Labs on the far shore, the pipe coming down to the water. */
    fill(274,60,40,42,8);rect(274,60,40,42,7);fill(280,66,6,3,10);fill(292,66,6,3,10);text(276,52,"MOS",10);
    fill(300,100,12,10,7);fill(296,106,24,8,7);rect(296,106,24,8,8);
    /* Water. */
    fill(60,WATER_Y,SW-60,40,4);fill2(60,WATER_Y+40,SW-60,42,4,0);
    for(k=0;k<14;k++)line(70+k*18,WATER_Y+4+(k%3)*6,80+k*18,WATER_Y+4+(k%3)*6,5);
    /* The intake grate, under water. */
    fill(150,164,38,20,8);for(k=0;k<6;k++)line(153+k*6,165,153+k*6,182,7);
    rect(150,164,38,20,11);text(151,155,"INTAKE",11);
    /* Sign. */
    line(244,74,244,WATER_Y+10,6);line(246,74,246,WATER_Y+10,6);fill(204,44,88,32,15);rect(204,44,88,32,0);
    text(218,47,"WARM WATER",0);text(227,56,"OUTFLOW",0);text(215,65,"NO SWIMMING",12);
    /* Near bank with reeds. */
    tri(0,WATER_Y,60,WATER_Y,76,200,10);fill(0,WATER_Y,62,82,10);fill2(0,WATER_Y+30,70,52,10,0);
    for(k=0;k<6;k++){line(58+k*3,WATER_Y-8+(k&1)*3,60+k*3,WATER_Y+6,2);}
}
/* Dawn, one step at a time: sky top, middle, horizon, sun (255: still
   below the hills), water. Palette indices cannot be blended, so the
   sunrise is a hand-picked sequence. */
static const unsigned char dawn_tab[10][5]={
 {0,1,1,255,1},{0,1,5,255,1},{1,1,5,255,1},{1,1,13,255,1},{1,9,13,4,1},
 {1,9,12,4,1},{1,9,12,12,1},{9,9,14,12,1},{9,11,14,14,9},{9,11,15,14,9}};
static void dawn(int a,int b,unsigned t0,unsigned t1)
{
    const unsigned char *d=dawn_tab[lerp(t0,t1,a,b)];
    pal_set(1,d[0]);pal_set(2,d[1]);pal_set(3,d[2]);pal_set(13,d[3]==255?d[2]:d[3]);pal_set(4,d[4]);
}
static void side_common(void)
{
    pal_load(pal_side);
    pal_set(5,(su>>3)%3==0?15:((su>>3)%3==1?11:9));
    pal_set(9,(su>>2)&1?15:11);
    phones_c=12;
}
/* Pipe outfall and the steam that comes off warm water. */
static void outfall_k(void)
{
    if(act(A_PROP,1)){line(300,114,302,WATER_Y,9);line(304,114,305,WATER_Y,9);line(308,114,307,WATER_Y,9);obj();}
}
static void outfall(void)
{
    int k,t;
    for(k=0;k<4;k++) {
        t=(int)((su+(unsigned)k*70)%280);
        oval2(220+k*24+t/20,WATER_Y-2-t/8,4+t/50,1+t/90,7,0);obj();
    }
}
static void chair_k(int x){if(act(A_CHAIR,(unsigned)x)){chair(x,WATER_Y,6);obj();}}
static void sitting(int x)
{
    chair_k(x);
    fig(x+1,WATER_Y-12,P_SIT,4,0,ACC_HAT|ACC_NECK,15);
}
static void fg_spot(void)
{
    int x,t;
    side_common();dawn(0,1,DS(20),DS(80));
    outfall_k();
    if(su>=DS(15) && act(A_CAPTION,1)){caption(6,"HIS FAVORITE SPOT.",15,11);obj();}
    if(su>=DS(38))chair_k(52);
    outfall();
    if(su<DS(30)){x=lerp(0,DS(30),-12,36);
        fig(x,WATER_Y-12-(int)((su/DS(3))&1),P_WALK0+(int)((su/DS(3))&3),4,0,ACC_HAT|ACC_NECK,15);
        line(x+1,WATER_Y-22,x-14,WATER_Y-52,6);fill(hand_x[0]-3,hand_y[0],7,5,6);obj();}
    else if(su<DS(45)){
        fig(36,WATER_Y-12,P_REACH,4,0,ACC_HAT|ACC_NECK,15);obj();

        line(30,WATER_Y-1,8,WATER_Y-30,6);obj();
    } else {
        sitting(52);rod(hand_x[1],hand_y[1],hand_x[1]+30,hand_y[1]-30,0,6);obj();
    }
    /* A fish jumps. */
    if(during(DS(36),DS(48))) {
        t=lerp(DS(36),DS(48),0,20);
        fish(160+t*2,WATER_Y-((t*(20-t))>>2),6,1,8,0,0,0);obj();
    }
    if(during(DS(48),DS(56))){t=lerp(DS(48),DS(56),2,12);ellipse(202,WATER_Y+1,t,t/4+1,15);obj();}
}
static void fg_cast(void)
{
    int t,bx,by,tx,ty,pose,r;
    side_common();dawn(2,3,DS(5),DS(45));
    outfall_k();chair_k(52);
    outfall();
    pose=su<DS(12)?P_HOLD:(su<DS(20)?P_WIND:P_CAST);
    if(su>=DS(42)){sitting(52);pose=-1;}
    else fig(34,WATER_Y-12,pose,4,0,ACC_HAT|ACC_NECK,15);
    if(pose==P_WIND){tx=hand_x[1]-22;ty=hand_y[1]-34;}
    else{tx=hand_x[1]+34;ty=hand_y[1]-24;}
    rod(hand_x[1],hand_y[1],tx,ty,0,6);
    if(su<DS(21)){bx=tx;by=ty+6;}
    else if(su<DS(30)){t=lerp(DS(21),DS(30),0,30);bx=tx+t*(200-tx)/30;by=ty+6+(WATER_Y-ty-6)*t/30-arc_y(t,30,40);}
    else{bx=200;by=WATER_Y+1+(int)((su>>4)&1);}
    line(tx,ty,bx,by,7);bobber(bx,by);obj();
    if(during(DS(30),DS(42))){r=lerp(DS(30),DS(42),2,18);ellipse(200,WATER_Y+2,r,r/4+1,15);
        if(r>8)ellipse(200,WATER_Y+2,r-7,(r-7)/4+1,5);
        obj();}
}

/* ============================================================ pond, top */
static const unsigned char pal_top[16]={0,1,9,3,11,15,6,7,8,2,10,11,12,13,14,15};
static const unsigned char ramp_cold[5]={1,9,3,11,15};
static const unsigned char ramp_hot[5]={1,5,12,14,15};
static int fl_primed,fl_repaint;
static void pond_prime(void)
{
    int x,y,i,dy,prof;
    fl_reset();
    /* An established plume, so the scene opens mid-flow. */
    for(y=1;y<=VY;y++)for(x=1;x<=VX;x++) {
        i=y*SVX+x;
        if(vtype[i]!=T_FLUID)continue;
        dy=y*2-7;if(dy<0)dy=-dy;
        u[i]=(dy>8?0:9-dy)*10;
    }
    for(y=1;y<=NY;y++)for(x=1;x<=NX;x++) {
        i=y*SX+x;
        if(htype[i]!=T_FLUID)continue;
        dy=y-7;if(dy<0)dy=-dy;
        prof=dy>5?0:(6-dy);
        den[i]=(prof*300-x*18)>0?prof*300-x*18:0;
        if(den[i]>DMAX)den[i]=DMAX;
    }
    fl_primed=1;
}
static void bg_top(void)
{
    int k;
    fill2(0,0,SW,POND_Y,10,9);fill2(0,POND_Y+NY*CELL,SW,SH-POND_Y-NY*CELL,10,9);
    fill2(0,POND_Y,POND_X,NY*CELL,10,9);fill2(POND_X+NX*CELL,POND_Y,SW-POND_X-NX*CELL,NY*CELL,10,9);
    for(k=0;k<7;k++){disc(10+k*48,20+(k&1)*6,9,9);disc(16+k*48,18+(k&1)*6,5,10);}
    /* Outflow pipe (inlet rows) and the intake grate. */
    fill(0,POND_Y+(INLET_Y0-1)*CELL,POND_X+2,(INLET_Y1-INLET_Y0+1)*CELL,7);
    rect(0,POND_Y+(INLET_Y0-1)*CELL,POND_X+2,(INLET_Y1-INLET_Y0+1)*CELL,8);
    text(1,POND_Y+(INLET_Y0-1)*CELL-9,"PIPE",14);
    fill(POND_X+NX*CELL-1,POND_Y+(INTAKE_Y0-1)*CELL,10,(INTAKE_Y1-INTAKE_Y0+1)*CELL,8);
    for(k=0;k<4;k++)line(POND_X+NX*CELL+1+k*2,POND_Y+(INTAKE_Y0-1)*CELL,POND_X+NX*CELL+1+k*2,POND_Y+INTAKE_Y1*CELL-1,7);
    /* "INTAKE" is wider than the 32-pixel bank beside the pond: it sits
       under the pond with a pointer up to the grate. */
    line(POND_X+NX*CELL+4,POND_Y+INTAKE_Y1*CELL+1,POND_X+NX*CELL+4,POND_Y+NY*CELL+2,11);
    line(POND_X+NX*CELL+2,POND_Y+INTAKE_Y1*CELL+3,POND_X+NX*CELL+4,POND_Y+INTAKE_Y1*CELL+1,11);
    line(POND_X+NX*CELL+6,POND_Y+INTAKE_Y1*CELL+3,POND_X+NX*CELL+4,POND_Y+INTAKE_Y1*CELL+1,11);
    textx(SW-36,POND_Y+NY*CELL+4,"INTAKE",1,11,0,NOC,0);
    /* The boulder. */
    fill(ROCK_PX,ROCK_PY,4*CELL,4*CELL,1);
    oval2(ROCK_CX,ROCK_CY,15,14,8,7);
    oval(ROCK_CX-5,ROCK_CY-5,6,3,7);
    textx(4,3,"FLOW-88",1,14,0,NOC,0);
    text(52,3,"LIVE NAVIER-STOKES POND",15);
    fl_draw(1);
}
static void ramp_set(const unsigned char *r){unsigned k;for(k=0;k<5;k++)pal_set(1+k,r[k]);}
/* Little fish swimming laps in the warm plume. */
static void shoal(int n)
{
    int k,a,x,y;
    for(k=0;k<n;k++) {
        a=(int)((su/8+(unsigned)k*5)&15);
        x=POND_X+150+k*30+SIN(30,a);y=POND_Y+56+(k-1)*16+SIN(12,(a+4)&15);
        fish(x,y,4,sin16[(a+4)&15]<0?1:-1,0,0,0,0);obj();
    }
}
#define BOB_X 188
#define BOB_Y 112
/* Alfredo on the near bank, seen from behind. He and his chair are one
   keyed actor (they sit on the grass, not the water); rod and line are
   redrawn every picture because the water under them keeps changing. */
static void bank_alfredo(int pose,unsigned acc,int tipx,int tipy,int bx,int by)
{
    int hx,hy;
    if(act(A_HERO,(unsigned)(pose*64+(int)acc))){chair(176,192,6);fig(176,178,pose,4,0,acc,15);obj();}
    if(tipx) {
        fig_hand(176,178,pose,4,1,&hx,&hy);
        rod(hx,hy,tipx,tipy,0,6);line(tipx,tipy,bx,by,7);obj();
    }
}
static void fg_pond(void)
{
    int bx=BOB_X,by=BOB_Y,dip=0,bigx,bigy;
    pal_load(pal_top);ramp_set(ramp_cold);
    fl_inlet_wobble=SIN(40,(su>>5)&15);
    if(su>=DS(200))dip=99;
    if(act(A_HERO,(unsigned)(P_SITBACK*64+(dip==99?1:0)))) {
        chair(176,192,6);fig(176,178,P_SITBACK,4,0,ACC_HAT|ACC_NECK|ACC_BACK,15);
        if(dip==99)balloon(150,140,"!!",172,152);
        obj();
    }
    shoal(3);
    /* The big one arrives at 8 s and circles the bobber. */
    if(su>=DS(80) && su<DS(200)) {
        bigx=lerp(DS(80),DS(130),300,bx+18);bigy=lerp(DS(80),DS(130),140,by+8);
        if(su>=DS(130)){bigx=bx+SIN(18,(su>>4)&15);bigy=by+SIN(10,((su>>4)+4)&15);}
        oval2(bigx,bigy,14,6,0,1);tri2(bigx+14,bigy,bigx+22,bigy-6,bigx+22,bigy+6,0,1);obj();
        if((su&15)==0)fl_push((bigx-POND_X)/CELL+1,(bigy-POND_Y)/CELL+1,-60,0);
    }
    if(dip!=99 && (during(DS(140),DS(150))||during(DS(165),DS(173))))dip=2;
    if(crossed(DS(140))||crossed(DS(165)))fl_ink(20,10,600);
    if(su>=DS(200) && su<DS(210)){fl_swirl(20,10,300);fl_ink(20,10,400);}
    {int hx,hy;fig_hand(176,178,P_SITBACK,4,1,&hx,&hy);
     if(dip!=99){rod(hx,hy,184,126,0,6);line(184,126,bx,by+dip,7);bobber(bx,by+dip);}
     else {rod(hx,hy,184,128,0,6);line(184,128,bx,BOB_Y+6,7);}
     obj();}
    part_c=7;
}

/* ======================================================= fight and catch */
static void fg_fight(void)
{
    int fx,fy,x,tx,ty,jump,t;
    side_common();dawn(4,5,DS(10),DS(90));
    outfall_k();chair_k(52);outfall();
    fx=150+SIN(40,(su>>5)&15)+lerp(0,DS(90),60,0);fy=WATER_Y+12;
    x=lerp(DS(30),DS(60),30,46);
    if(su>=DS(80))x=lerp(DS(80),DS(95),46,26);
    fig(x,WATER_Y-12,P_REEL,4,0,ACC_HAT|ACC_NECK,15);
    tx=hand_x[1]+30;ty=hand_y[1]-14+(int)((su>>2)&1);
    rod(hand_x[1],hand_y[1],tx,ty+8,10+(int)((su>>3)&3),6);
    jump=during(DS(33),DS(42))||during(DS(68),DS(77));
    if(su>=DS(92)) {
        /* Out it comes. */
        t=lerp(DS(92),DS(100),0,16);fx=fx+(70-fx)*t/16;fy=WATER_Y-arc_y(t,16,60);
        jump=1;
    }
    line(tx,ty+8,fx,jump?fy:WATER_Y,7);obj();
    if(jump){fish(fx,jump&&su<DS(92)?WATER_Y-6:fy,16,-1,12,6,1,(int)((su>>2)&1)*2);obj();}
    else {
        /* Dorsal fin cutting the surface, wake and spray. */
        tri(fx-5,WATER_Y,fx+5,WATER_Y,fx+3,WATER_Y-9,8);
        line(fx+6,WATER_Y,fx+18,WATER_Y+3,15);line(fx+6,WATER_Y+1,fx+16,WATER_Y-2,15);
        if((su>>2)&1){dot(fx-3,WATER_Y-5,15);dot(fx+1,WATER_Y-8,15);dot(fx-6,WATER_Y-3,15);}
        else {dot(fx-2,WATER_Y-7,15);dot(fx+4,WATER_Y-4,15);}
        obj();
    }
    if(su<DS(30) && ((su>>3)&1))ctext_cached(16,"FISH ON!",3,14,12,0);
}
static void fg_catch(void)
{
    int t,fx,fy;
    side_common();dawn(6,7,DS(10),DS(70));
    pal_set(12,(su>>3)%3==0?10:((su>>3)%3==1?14:12));
    outfall_k();chair_k(52);outfall();
    if(su<DS(55)) {
        fig(30,WATER_Y-12,su<DS(5)?P_REACH:P_LIFT,4,0,ACC_HAT|ACC_NECK,15);obj();
        fx=30;fy=su<DS(5)?WATER_Y-4:WATER_Y-46;
        fish(fx,fy,22,1,12,6,1,(int)((su>>3)&1)*2);obj();
        if(su>=DS(15))ctext_cached(14,"PERSONAL BEST!",2,14,6,0);
        if(during(DS(15),DS(40))){t=(int)((su>>2)&7);dot(fx-24+t*6,fy-14-(t&3)*3,15);dot(fx+20-t*5,fy-10+t,15);obj();}
    } else if(su<DS(66)) {
        /* Catch and release. */
        fig(30,WATER_Y-12,P_CAST,4,0,ACC_HAT|ACC_NECK,15);obj();
        t=lerp(DS(55),DS(66),0,20);
        fish(40+t*7,WATER_Y-46+t*3-arc_y(t,20,30),22-t/2,1,12,6,1,0);obj();
    } else {
        sitting(52);rod(hand_x[1],hand_y[1],hand_x[1]+30,hand_y[1]-30,0,6);obj();
        if(su<DS(76)){t=lerp(DS(66),DS(76),3,24);ellipse(180,WATER_Y+2,t,t/4+1,15);obj();}
        if(su>=DS(70)){balloon(58,54,"WHAT A NIGHT.",56,80);obj();}
    }
    pal_flash=during(DS(15),DS(17));
}

/* ================================================================= alarm */
static const unsigned char pal_labs[16]={0,1,9,13,4,12,6,7,8,9,10,11,12,13,14,15};
static void bg_labs(void)
{
    int k;
    fill(0,0,SW,40,1);fill2(0,40,SW,20,1,2);fill(0,60,SW,40,2);fill2(0,100,SW,20,2,3);fill(0,120,SW,30,3);
    fill(20,40,220,140,8);rect(20,40,220,140,7);
    textx(54,52,"MOS LABS",3,10,0,0,0);
    for(k=0;k<8;k++){fill(30+k*26,92,18,30,0);fill(33+k*26,96,4,2,10);fill(40+k*26,100,4,2,2);}
    fill(52,132,156,24,0);rect(50,130,160,28,7);
    /* Cooling towers. */
    tri(250,150,300,150,282,70,7);tri(250,150,282,70,262,70,7);fill(258,66,30,5,8);
    /* Outflow pipe down to the pond. */
    fill(200,160,120,14,7);rect(200,160,120,14,8);
    ring(226,167,12,6);
    fill(0,180,SW,20,10);
    text(4,4,"5:00 AM",14);
}
static void fg_alarm(void)
{
    int k,a,t,m;const char *msg;unsigned c=10;
    pal_load(pal_labs);
    if(su>=DS(20)){pal_set(5,(su>>3)&1?12:4);}
    else pal_set(5,8);
    if(act(A_PROP,1)){disc(130,30,6,5);fill(124,34,13,6,8);obj();}
    m=su<DS(20)?0:(su<DS(31)?1:(su<DS(42)?2:(su<DS(53)?3:4)));
    msg=m==0?"TRAINING RUN COMPLETE":(m==1?"HEAT FLUSH IN 3":(m==2?"HEAT FLUSH IN 2":(m==3?"HEAT FLUSH IN 1":"FLUSHING!")));
    if(m==4)c=12;
    if(act(A_PROP2,(unsigned)m)){fill(54,134,152,20,0);ctext(140,msg,1,c,NOC,NOC,0);obj();}
    a=su>=DS(53)?(int)(su>>2):0;
    for(k=0;k<4;k++){line(226,167,226+SIN(11,(a+k*4)&15),167+SIN(11,(a+k*4+4)&15),6);}
    obj();
    for(k=0;k<4;k++) {
        t=(int)((su*(su>=DS(53)?3:1)+(unsigned)k*60)%240);
        oval2(272+k*4,62-t/5,4+t/24,2+t/40,7,0);obj();
    }
    if(su>=DS(53)){t=lerp(DS(53),DS(70),0,80);fill(320-t,162,t,10,15);obj();}
}

/* ================================================================= surge */
static int al_x,al_y;            /* Alfredo in the water, screen pixels */
static int hat_x=-1,hat_y;       /* his hat, carried by the real current */
static void surge_path(void)
{
    int a;long r;
    if(su<DS(35)){al_x=176;al_y=178;return;}
    if(su<DS(50)){al_x=lerp(DS(35),DS(50),176,104);al_y=lerp(DS(35),DS(50),160,118);return;}
    if(su<DS(100)) {
        /* Twice round the boulder, tightening. */
        a=lerp(DS(50),DS(100),0,32);r=lerp(DS(50),DS(100),34,22);
        al_x=ROCK_CX+SIN((int)r,(a+12)&15);
        al_y=ROCK_CY+SIN((int)r,a&15);
        return;
    }
    al_x=lerp(DS(100),DS(128),ROCK_CX,POND_X+NX*CELL+2);
    al_y=lerp(DS(100),DS(128),ROCK_CY+26,POND_Y+(INTAKE_Y0-1)*CELL+20);
}
static void fg_surge(void)
{
    int cx,cy,pu,pv,ang;
    pal_load(pal_top);ramp_set(ramp_cold);
    if(su>=DS(8))pal_set(2,5);
    if(su>=DS(14))pal_set(3,12);
    if(su>=DS(20)){pal_set(4,14);}
    if(su>=DS(150)){pal_set(4,11);}
    if(su>=DS(160)){pal_set(3,3);pal_set(2,9);}
    fl_inlet_u=su<DS(130)?lerp(0,DS(20),150,820):lerp(DS(130),DS(170),820,150);
    fl_inlet_d=su<DS(140)?DMAX:1800;
    fl_inlet_wobble=SIN(120,(su>>4)&15);
    /* Intake pumps harder too. */
    if(su<DS(140)){int y;for(y=INTAKE_Y0;y<=INTAKE_Y1;y++)fl_push(NX,y,su<DS(20)?20:60,0);}
    surge_path();
    if(su<DS(35)) {
        bank_alfredo(su<DS(20)?P_SITBACK:P_FLAIL,ACC_HAT|ACC_NECK|ACC_BACK,su<DS(20)?184:0,126,BOB_X,BOB_Y);
        if(su>=DS(20)){balloon(150,140,"!!!",172,152);obj();}
        if(su>=DS(28)){fill2(120,148+(int)(DS(35)-su)/4,120,12,5,4);obj();}
    } else if(su<DS(128)) {
        cx=(al_x-POND_X)/CELL+1;cy=(al_y-POND_Y)/CELL+1;
        fl_swirl(cx,cy,su<DS(100)?160:220);fl_ink(cx,cy,180);
        ang=(int)(su>>3);
        fig(al_x+1,al_y+1,P_FLAIL,3,ang,0,0);
        fig(al_x,al_y,P_FLAIL,3,ang,su<DS(60)?ACC_HAT:0,15);obj();
        if(su>=DS(60) && hat_x<0){hat_x=al_x<<4;hat_y=al_y<<4;}
    } else if(su<DS(150)) {
        /* Bubbles from the grate. */
        int k;for(k=0;k<4;k++){cy=POND_Y+(INTAKE_Y0-1)*CELL+4+k*7;dot(POND_X+NX*CELL-8-(int)((su+(unsigned)k*13)&15),cy,15);}
        obj();
    }
    if(hat_x>=0) {
        pond_vel(hat_x>>4,hat_y>>4,&pu,&pv);
        hat_x+=pu/12;hat_y+=pv/12;
        if((hat_x>>4)>POND_X+NX*CELL-30)hat_x=(POND_X+NX*CELL-30)<<4;
        if((hat_x>>4)<POND_X+6)hat_x=(POND_X+6)<<4;
        if((hat_y>>4)<POND_Y+6)hat_y=(POND_Y+6)<<4;
        if((hat_y>>4)>POND_Y+NY*CELL-6)hat_y=(POND_Y+NY*CELL-6)<<4;
        ring(hat_x>>4,hat_y>>4,5,15);disc(hat_x>>4,hat_y>>4,2,15);dot((hat_x>>4)+3,(hat_y>>4)-1,12);obj();
    }
    shoal(su<DS(130)?1:3);
    if(su>=DS(40) && su<DS(130)) {
        cx=al_x+lerp(DS(40),DS(130),-40,-20)+SIN(10,(su>>3)&15);
        oval2(cx,al_y+18,12,5,0,13);tri2(cx-12,al_y+18,cx-19,al_y+13,cx-19,al_y+23,0,13);obj();
    }
    part_c=(unsigned char)(su<DS(130)?15:7);
}

/* ================================================================== calm */
static void fg_calm(void)
{
    int hy,t,dipping;
    side_common();dawn(8,9,DS(10),DS(60));
    outfall_k();chair_k(52);outfall();
    fill(40,WATER_Y-5,9,5,6);obj();
    dipping=during(DS(20),DS(28))||during(DS(30),DS(38))||during(DS(40),DS(48))||during(DS(50),DS(64));
    hy=WATER_Y-1+(int)((su>>4)&1)+(dipping?2:0);
    if(su<DS(78)) {
        line(162,hy,178,hy,15);rect(165,hy-4,10,4,15);dot(173,hy-3,12);obj();
        if(su>=DS(70)){t=(int)((su>>2)&7);dot(168+t,hy+4+(t&3),15);obj();}
    } else {
        t=lerp(DS(78),DS(88),12,-8);
        if(su>=DS(102))t=lerp(DS(102),DS(110),-8,14);
        clip(0,0,SW-1,WATER_Y+1);
        fish(170,WATER_Y+t+8,22,-1,12,6,1,0);
        line(162,WATER_Y+t-6,178,WATER_Y+t-6,15);rect(165,WATER_Y+t-10,10,4,15);
        noclip();obj();
        if(during(DS(94),DS(97))){fill(152,WATER_Y+t,10,3,12);obj();}
    }
    pal_set(12,(su>>3)&1?10:12);
}

/* =============================================================== credits */
static const char *const credit_lines[]={
 "ALFREDO: AFTER HOURS","","BIG BLUE DISK 2026","",
 "STORY, PICTURES, MUSIC","CLAUDE","",
 "POND SOLVER","FLOW-88: GEMINI, CLAUDE, CODEX","",
 "FINE CLOCK AND DRUM KIT","RADIO SHACK RAVE","",
 "PRODUCER","CHAD","","THANKS","ASTRA","",
 "NO FISH WERE HARMED.","ALFREDO WAS, AS USUAL.","","",
 "CONTINUED IN","LAST MONTH'S ISSUE."};
#define NCREDITS (sizeof(credit_lines)/sizeof(credit_lines[0]))
static void bg_credits(void)
{
    seed=3;stars(80,200,7,8);
    ctext(10,"THE END",4,15,1,NOC,0);
    fill2(0,184,SW,16,1,0);
}
static void fg_credits(void)
{
    int k,y,top=lerp(DS(10),DS(195),186,186-(int)NCREDITS*12-60),x;
    pal_load(pal_title);pal_set(1,1);pal_set(7,(su>>5)&1?8:15);pal_set(8,(su>>5)&1?15:7);
    clip(0,48,SW-1,181);
    for(k=0;k<(int)NCREDITS;k++) {
        y=top+k*12;
        if(y<40 || y>182)continue;
        text_fast(cx_of(credit_lines[k],1),y,credit_lines[k],k==0?14:((k>0 && credit_lines[k-1][0]==0)?11:15),0);obj();
    }
    noclip();
    x=lerp(0,DS(200),-30,350);
    fish(x,191,10,1,12,6,1,(int)((su>>3)&1));line(x-4,180,x+6,180,15);rect(x-2,177,6,3,15);obj();
    pal_set(12,(su>>3)&1?10:12);
}

/* ============================================================== end card */
static void bg_end(void)
{
    seed=5;stars(60,200,7,8);
    ctext(14,"ALFREDO",4,15,1,NOC,0);
    ctext(48,"AFTER HOURS",2,14,6,NOC,0);
    text(70,96,"R      WATCH AGAIN",15);
    text(70,112,"F      STAY AND FISH",15);
    text(70,121,"       (THE FLOW-88 POND)",7);
    text(70,137,"ESC    BACK TO THE DISK",15);
    text(70,96,"R",14);text(70,112,"F",14);text(70,137,"ESC",14);
    line(0,182,319,182,8);
    fill(250,170,44,12,8);
}
static void fg_end(void)
{
    unsigned long f=scene_fine();int half=(int)(f*2/beat_len);
    pal_load(pal_title);pal_set(1,1);pal_set(7,(su>>5)&1?8:15);pal_set(8,(su>>5)&1?15:7);
    if(act(A_DJ,(unsigned)(dj_seq[half&15]*4+bob_of(f)))){fig(272,168+bob_of(f),P_DJ0+dj_seq[half&15],4,0,ACC_PHONES,15);obj();}

}

/* ============================================================= fish mode */
static int lure_x=180,lure_y=100,catches,hooked_until;
static unsigned fish_su;
static int bigfx=280,bigfy=140;
static void fg_fish(void)
{
    char s[24];int dx,dy;
    pal_load(pal_top);ramp_set(fl_inlet_u>300?ramp_hot:ramp_cold);
    fl_inlet_wobble=SIN(40,(su>>5)&15);
    shoal(3);
    /* The big one follows the lure. */
    dx=lure_x-bigfx;dy=lure_y-bigfy;
    bigfx+=dx>0?1:(dx<0?-1:0);bigfy+=dy>0?1:(dy<0?-1:0);
    oval2(bigfx,bigfy,14,6,0,1);tri2(bigfx+14,bigfy,bigfx+22,bigfy-6,bigfx+22,bigfy+6,0,1);obj();
    if(dx>-4 && dx<4 && dy>-4 && dy<4 && (int)su>hooked_until) {
        catches++;hooked_until=(int)su+DS(30);bigfx=POND_X+NX*CELL-10;bigfy=POND_Y+20;
        sfx_start(SFX_SPLASH,E);fl_swirl((lure_x-POND_X)/CELL+1,(lure_y-POND_Y)/CELL+1,300);
    }
    if((int)su<hooked_until){ctext(150,"FISH ON!",2,14,12,0,0);obj();}
    bobber(lure_x,lure_y);obj();
    part_c=7;
    fill(0,POND_Y+NY*CELL+4,SW,24,0);
    text(4,POND_Y+NY*CELL+6,"ARROWS MOVE  SPACE STIRS  H HEAT  C CALM",15);
    sprintf(s,"CAUGHT: %d   ESC: BACK",catches);text(4,POND_Y+NY*CELL+16,s,14);obj();
    (void)fish_su;
}
static void fish_key(int k)
{
    int cx=(lure_x-POND_X)/CELL+1,cy=(lure_y-POND_Y)/CELL+1;
    switch(k) {
    case 0x4800:if(lure_y>POND_Y+4)lure_y-=4;fl_push(cx,cy,0,-120);break;
    case 0x5000:if(lure_y<POND_Y+NY*CELL-4)lure_y+=4;fl_push(cx,cy,0,120);break;
    case 0x4b00:if(lure_x>POND_X+4)lure_x-=4;fl_push(cx,cy,-120,0);break;
    case 0x4d00:if(lure_x<POND_X+NX*CELL-4)lure_x+=4;fl_push(cx,cy,120,0);break;
    case ' ':fl_swirl(cx,cy,400);fl_ink(cx,cy,900);sfx_start(SFX_PLOP,E);break;
    case 'h':case 'H':fl_inlet_u=fl_inlet_u>300?150:700;fl_inlet_d=fl_inlet_u>300?DMAX:1800;
        sfx_start(fl_inlet_u>300?SFX_SURGE:SFX_BLIP,E);break;
    case 'c':case 'C':pond_prime();fl_repaint=1;break;
    }
}

/* ============================================================= dispatch */
static void scene_backdrop(int b)
{
    begin_backdrop();
    {unsigned k;for(k=0;k<4;k++)_fmemset(bg+k*8192,0,8000);}
    switch(b) {
    case BG_TITLE:bg_title();break;
    case BG_CLUB:bg_club();break;
    case BG_CLOCK:bg_clock();break;
    case BG_STREET:bg_street();break;
    case BG_SIDE:bg_side();break;
    case BG_TOP:bg_top();break;
    case BG_LABS:bg_labs();break;
    case BG_CREDITS:bg_credits();break;
    default:bg_end();break;
    }
}
static void scene_enter(int n)
{
    hat_x=-1;pal_flash=0;phones_c=13;tc_ok=0;su_prev=0;
    if(n==SC_POND || n==SC_FISH || (n==SC_SURGE && !fl_primed)){pond_prime();}
    if(n==SC_FISH){fl_inlet_u=150;fl_inlet_d=1800;catches=0;hooked_until=0;}
    if(n==SC_POND){fl_inlet_u=150;fl_inlet_d=1800;}
}
/* After the backdrop: rasterise the scene's big title while it is dark. */
static void scene_prepare(int n)
{
    if(n==SC_RAVE)tc_prepare(16,"LETS GO!",3,14,12,0);
    else if(n==SC_FIGHT)tc_prepare(16,"FISH ON!",3,14,12,0);
    else if(n==SC_CATCH)tc_prepare(14,"PERSONAL BEST!",2,14,6,0);
}
static int fluid_scene(int n){return n==SC_POND || n==SC_SURGE || n==SC_FISH;}
static void scene_draw(int n)
{
    switch(n) {
    case SC_TITLE:fg_title();break;
    case SC_RAVE:fg_rave();break;
    case SC_LAST:fg_last();break;
    case SC_CLOCK:fg_clock();break;
    case SC_WALK:fg_walk();break;
    case SC_SPOT:fg_spot();break;
    case SC_CAST:fg_cast();break;
    case SC_POND:fg_pond();break;
    case SC_FIGHT:fg_fight();break;
    case SC_CATCH:fg_catch();break;
    case SC_ALARM:fg_alarm();break;
    case SC_SURGE:fg_surge();break;
    case SC_CALM:fg_calm();break;
    case SC_CREDITS:fg_credits();break;
    case SC_END:fg_end();break;
    default:fg_fish();break;
    }
    su_prev=su;
}
