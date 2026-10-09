#ifndef HORSE_H
#define HORSE_H
#include "COMMON.H"
#define HORSES 6
#define FINISH 600
typedef struct {
    int pos[HORSES], tick[HORSES], num[HORSES], den[HORSES];
    int time, done;
} Race;
/* Per-horse form: step = lo + roll(spread); 1/stum chance of a stumble tick;
   1/kick chance per tick of a stretch kick (+6) once past 400.  Tuned by
   40,000-race host simulation: win chances ~22/18/19/19/14/9 percent under the game's own RNG.
   Gross multipliers are tenths (29 = 2.9x stake) with about a 12% house edge. */
static const int hlo[HORSES]   ={9, 8, 7, 6, 5, 4};
static const int hspread[HORSES]={3, 5, 7, 9,11,13};
static const int hkick[HORSES] ={8,10,12,16,20,40};
static const int hstum[HORSES] ={45,50,45,40,40,16};
static const int winx10[HORSES]  ={39,50,47,47,65,94};
static const int placex10[HORSES]={19,23,22,26,34,57};
static void race_reset(Race *r)
{ int i; memset(r,0,sizeof(*r));for(i=0;i<HORSES;i++)r->tick[i]=-1; }
static void race_step(Race *r, int *steps)
{
    int i, old;
    for(i=0;i<HORSES;i++) if(r->tick[i]<0) {
        old=r->pos[i];r->pos[i]+=steps[i];
        if(r->pos[i]>=FINISH) {
            r->tick[i]=r->time;r->num[i]=FINISH-old;
            r->den[i]=steps[i];++r->done;
        }
    }
    ++r->time;
}
/* Fractional crossing times, not array iteration, decide the order. */
static int earlier(Race *r,int a,int b)
{
    long x,y;
    if(r->tick[a]!=r->tick[b])return r->tick[a]<r->tick[b]?-1:1;
    x=(long)r->num[a]*r->den[b];y=(long)r->num[b]*r->den[a];
    return x<y?-1:x>y?1:0;
}
static int rankhorse(Race *r,int h)
{ int i,n=0;for(i=0;i<HORSES;i++)if(earlier(r,i,h)<0)++n;return n; }
static void finish_order(Race *r,int *order)
{
    int i,j,h;
    for(i=0;i<HORSES;i++)order[i]=i;
    for(i=1;i<HORSES;i++) {
        h=order[i];j=i;
        while(j>0 && earlier(r,h,order[j-1])<0) {
            order[j]=order[j-1];--j;
        }
        order[j]=h;
    }
}
static long payout(Race *r,int h,int place,long stake)
{
    int i,ties=0,rank=rankhorse(r,h),slots;
    long x10;
    if(h<0||h>=HORSES||rank>=(place?2:1))return 0;
    for(i=0;i<HORSES;i++)if(!earlier(r,i,h))++ties;
    slots=(place?2:1)-rank;if(slots>ties)slots=ties;
    x10=place?placex10[h]:winx10[h];
    return stake*x10*slots/(10L*ties);
}
/* Step for one horse this tick; *ev gets 0 none, 1 stumble, 2 kick. */
static int horse_step(Race *r,int i,int *ev)
{
    int s=hlo[i]+(int)roll(hspread[i]);
    *ev=0;
    if(roll(hstum[i])==0){*ev=1;return 1;}
    if(r->pos[i]>400&&roll(hkick[i])==0){*ev=2;s+=6;}
    return s;
}
/* A photo finish: first two share a tick and cross within 1/4 step. */
static int photo(Race *r)
{
    int o[HORSES];long x,y;
    finish_order(r,o);
    if(r->tick[o[0]]!=r->tick[o[1]])return 0;
    x=(long)r->num[o[0]]*r->den[o[1]];y=(long)r->num[o[1]]*r->den[o[0]];
    return (y-x)*4L<=(long)r->den[o[0]]*r->den[o[1]];
}
#endif
