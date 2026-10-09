#ifndef TRADE_H
#define TRADE_H
#include "COMMON.H"
#define GOODS 4
#define CAPSTART 100
#define CAPMAX 160
#define COATCOST 1500L
typedef struct {
    long cash,bank,debt;
    int day,city,qty[GOODS],price[GOODS];
    int cap,evgood,evqty,rgood,rcity;
} Market;
static char *goods[GOODS]={"Moonleaf","Fog dust","Dream drops","Star resin"};
static char *cities[4]={"Blueport","Tin Junction","Lantern Bay","Copper Hill"};
/* Local price multiplier in percent: where each good runs cheap or dear. */
static const int bias[4][GOODS]={
    { 55,100,140,100},{100,150, 70,105},{130, 80,100,160},{100,100,120, 55}};
static void prices(Market *m)
{
    int i;
    for(i=0;i<GOODS;i++)
        m->price[i]=(int)((long)(50+(int)roll(951))*bias[m->city][i]/100L);
}
static void rumor(Market *m)
{
    int c,best=0;
    m->rgood=(int)roll(GOODS);
    for(c=1;c<4;c++)if(bias[c][m->rgood]<bias[best][m->rgood])best=c;
    m->rcity=best;
}
static void market_reset(Market *m)
{memset(m,0,sizeof(*m));m->cash=2000;m->debt=5500;m->day=1;
 m->cap=CAPSTART;prices(m);rumor(m);}
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
        if(n>m->cap-cargo(m)||cost>m->cash)return 0;
        m->qty[g]+=(int)n;m->cash-=cost;
    }
    return 1;
}
/* A bigger coat: more room to carry. */
static int buy_coat(Market *m)
{
    if(m->day>30||m->cap+30>CAPMAX||m->cash<COATCOST)return 0;
    m->cash-=COATCOST;m->cap+=30;return 1;
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
/* Returns event 0-15, or -1 if travel is not allowed.
   0 lost purse, 1 festival, 2 customs, 3 Mr. Tin leans, 4 shortage (x3),
   5 glut (/4), 6 lucky find, 7-15 quiet road. */
static int travel(Market *m,int city)
{
    int event,g,i,take;long interest,extra;
    if(city<0||city>=4||city==m->city||m->day>=30)return -1;
    m->city=city;++m->day;
    interest=m->debt/20;
    if(interest>MONEYMAX-m->debt)m->debt=MONEYMAX;
    else m->debt+=interest;
    event=(int)roll(16);
    m->evgood=(int)roll(GOODS);m->evqty=0;
    prices(m);
    switch(event) {
    case 0:m->cash-=m->cash/10;break;
    case 1:if(m->cash<=MONEYMAX-400)m->cash+=400;break;
    case 2:
        g=0;for(i=1;i<GOODS;i++)if(m->qty[i]>m->qty[g])g=i;
        take=m->qty[g]<25?m->qty[g]:25;
        m->qty[g]-=take;m->evgood=g;m->evqty=take;break;
    case 3:
        extra=m->debt/10;
        if(extra>MONEYMAX-m->debt)extra=MONEYMAX-m->debt;
        m->debt+=extra;m->evqty=(int)(extra>30000L?30000L:extra);break;
    case 4:m->price[m->evgood]*=3;break;
    case 5:m->price[m->evgood]/=4;if(m->price[m->evgood]<20)m->price[m->evgood]=20;break;
    case 6:
        take=m->cap-cargo(m);if(take>10)take=10;
        m->qty[m->evgood]+=take;m->evqty=take;break;
    }
    rumor(m);
    return event;
}
static void event_text(Market *m,int e,char *buf)
{
    switch(e) {
    case 0:strcpy(buf,"A lost purse cost 10% of cash.");break;
    case 1:strcpy(buf,"Festival raffle! You win $400.");break;
    case 2:
        if(m->evqty)sprintf(buf,"Customs sniffs your coat: %d %s confiscated!",
            m->evqty,goods[m->evgood]);
        else strcpy(buf,"Customs pats your empty coat and waves you on.");
        break;
    case 3:sprintf(buf,"Mr. Tin's collectors lean on you: debt +$%d.",m->evqty);break;
    case 4:sprintf(buf,"Dock strike! %s is scarce: price x3 today.",goods[m->evgood]);break;
    case 5:sprintf(buf,"Cargo ship wreck! %s floods the market: price /4.",goods[m->evgood]);break;
    case 6:
        if(m->evqty)sprintf(buf,"Lucky find: %d %s under a loose plank.",m->evqty,goods[m->evgood]);
        else strcpy(buf,"You find a loose plank, but your coat is full.");
        break;
    default:strcpy(buf,"A quiet journey.");
    }
}
static long score(Market *m)
{
    long n=m->cash+m->bank-m->debt;int i;
    for(i=0;i<GOODS;i++)n+=(long)m->qty[i]*m->price[i];
    return n;
}
static const char *rank_title(long s)
{
    return s<0?"Drowned in Mr. Tin's ledger":s<5000?"Dockside nobody":
        s<20000?"Corner hustler":s<60000?"Harbor operator":
        s<150000L?"Blueport boss":"Legend of the Big Blue Disk";
}
#endif
