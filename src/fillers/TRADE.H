#ifndef TRADE_H
#define TRADE_H
#include "COMMON.H"
#define GOODS 4
typedef struct {long cash,bank,debt;int day,city,qty[GOODS],price[GOODS];} Market;
static void prices(Market *m)
{int i;for(i=0;i<GOODS;i++)m->price[i]=50+(int)roll(951);}
static void market_reset(Market *m)
{memset(m,0,sizeof(*m));m->cash=2000;m->debt=5500;m->day=1;prices(m);}
static int cargo(Market *m)
{int i,n=0;for(i=0;i<GOODS;i++)n+=m->qty[i];return n;}
static int trade(Market *m,int g,long n,int sell)
{
    long cost;
    if(g<0||g>=GOODS||n<=0||n>100||m->day>30)return 0;
    cost=n*m->price[g];
    if(sell) {
        if(n>m->qty[g]||m->cash>MONEYMAX-cost)return 0;
        m->qty[g]-=(int)n;m->cash+=cost;
    } else {
        if(n>100-cargo(m)||cost>m->cash)return 0;
        m->qty[g]+=(int)n;m->cash-=cost;
    }
    return 1;
}
static int move_money(long *from,long *to,long n)
{
    if(n<=0||n>*from||n>MONEYMAX-*to)return 0;
    *from-=n;*to+=n;return 1;
}
static int repay(Market *m,long n)
{
    if(n<=0||n>m->cash||n>m->debt)return 0;
    m->cash-=n;m->debt-=n;return 1;
}
static int travel(Market *m,int city)
{
    int event;long interest;
    if(city<0||city>=4||city==m->city||m->day>=30)return -1;
    m->city=city;++m->day;
    interest=m->debt/20;
    if(interest>MONEYMAX-m->debt)m->debt=MONEYMAX;
    else m->debt+=interest;
    event=(int)roll(8);
    if(event==0)m->cash-=m->cash/10;
    if(event==1 && m->cash<=MONEYMAX-150)m->cash+=150;
    prices(m);return event;
}
static long score(Market *m)
{
    long n=m->cash+m->bank-m->debt;int i;
    for(i=0;i<GOODS;i++)n+=(long)m->qty[i]*m->price[i];
    return n;
}
#endif
