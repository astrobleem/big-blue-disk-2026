#ifndef HORSE_H
#define HORSE_H
#include "COMMON.H"
#define HORSES 6
#define FINISH 600
typedef struct {
    int pos[HORSES], tick[HORSES], num[HORSES], den[HORSES];
    int time, done;
} Race;
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
    if(rank>=(place?2:1))return 0;
    for(i=0;i<HORSES;i++)if(!earlier(r,i,h))++ties;
    slots=(place?2:1)-rank;if(slots>ties)slots=ties;
    return stake*(place?2L:6L)*slots/ties;
}
#endif
