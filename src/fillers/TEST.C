#include "HORSE.H"
#include "TRADE.H"
static int checks=0,failures=0;
#define CHECK(x) do {++checks;if(!(x)){printf("FAIL line %d\n",__LINE__);++failures;}}while(0)
int main(void)
{
    Race r;Market m;long n,a,b;int s[6],order[6],i,j,rank,ev,e,k,w[8];
    char txt[96];
    CHECK(number("+100",100,&n)&&n==100);
    CHECK(!number("-1",MONEYMAX,&n));CHECK(!number("",100,&n));
    CHECK(!number("1x",100,&n));CHECK(!number(" 1",100,&n));
    CHECK(!number("100000001",MONEYMAX,&n));
    CHECK(!number("2147483648",MONEYMAX,&n));
    CHECK(!number("999999999999999999999999",MONEYMAX,&n));
    CHECK(number("100000000",MONEYMAX,&n)&&n==MONEYMAX);
    race_reset(&r);for(i=0;i<6;i++){r.pos[i]=590;s[i]=10;}
    race_step(&r,s);CHECK(r.done==6);
    finish_order(&r,order);for(i=0;i<6;i++)CHECK(order[i]==i);
    for(i=0;i<6;i++){CHECK(rankhorse(&r,i)==0);CHECK(payout(&r,i,0,60)==winx10[i]);CHECK(payout(&r,i,1,60)==2L*placex10[i]);}
    race_reset(&r);for(i=0;i<6;i++){r.pos[i]=590;s[i]=10+i;}
    race_step(&r,s);CHECK(rankhorse(&r,5)==0);CHECK(rankhorse(&r,0)==5);
    finish_order(&r,order);for(i=0;i<6;i++)CHECK(order[i]==5-i);
    CHECK(payout(&r,5,0,10)==winx10[5]);CHECK(payout(&r,4,1,10)==placex10[4]);
    CHECK(payout(&r,3,1,10)==0);
    race_reset(&r);CHECK(r.done==0&&r.time==0&&r.tick[5]==-1);
    rngstate=42;
    for(j=0;j<200;j++) {
        race_reset(&r);
        while(r.done<6){for(i=0;i<6;i++)s[i]=5+(int)roll(11);race_step(&r,s);}
        rank=0;for(i=0;i<6;i++){CHECK(rankhorse(&r,i)>=0&&rankhorse(&r,i)<6);if(rankhorse(&r,i)==0)rank++;}
        CHECK(rank>0);
        finish_order(&r,order);
        for(i=1;i<6;i++)CHECK(earlier(&r,order[i-1],order[i])<=0);
    }
    market_reset(&m);CHECK(m.day==1&&m.cash==2000&&m.debt==5500&&cargo(&m)==0);
    m.price[0]=10;CHECK(trade(&m,0,100,0));CHECK(cargo(&m)==100&&m.cash==1000);
    CHECK(!trade(&m,1,1,0));CHECK(!trade(&m,0,-1,1));
    CHECK(!trade(&m,0,101,1));CHECK(!trade(&m,4,1,0));
    CHECK(trade(&m,0,100,1));CHECK(m.cash==2000&&cargo(&m)==0);
    m.cash=MONEYMAX;m.qty[0]=1;CHECK(!trade(&m,0,1,1));CHECK(m.qty[0]==1);
    a=100;b=MONEYMAX;CHECK(!move_money(&a,&b,1)&&a==100);
    a=100;b=0;CHECK(!move_money(&a,&b,-1));CHECK(!move_money(&a,&b,101));
    CHECK(move_money(&a,&b,100)&&a==0&&b==100);
    market_reset(&m);CHECK(repay(&m,2000)&&m.cash==0&&m.debt==3500);
    CHECK(!repay(&m,1));m.debt=MONEYMAX;CHECK(travel(&m,1)>=0&&m.debt==MONEYMAX);
    CHECK(travel(&m,1)==-1);while(m.day<30)CHECK(travel(&m,(m.city+1)%4)>=0);
    CHECK(travel(&m,(m.city+1)%4)==-1&&m.day==30);
    m.cash=MONEYMAX;m.bank=MONEYMAX;m.debt=0;
    m.qty[0]=100;m.price[0]=1000;CHECK(score(&m)==200100000L);
    market_reset(&m);CHECK(m.bank==0&&cargo(&m)==0&&m.day==1);
    /* Odds table sanity: longer odds for the longer shots, win never cheaper than place. */
    for(i=0;i<6;i++){CHECK(winx10[i]>placex10[i]);CHECK(placex10[i]>10);}
    CHECK(payout(&r,-1,0,10)==0&&payout(&r,6,0,10)==0);
    /* Photo finish: dead heat on the same tick within a quarter step. */
    race_reset(&r);for(i=0;i<6;i++){r.pos[i]=590;s[i]=10;}
    s[0]=15;race_step(&r,s);CHECK(photo(&r)==0);
    race_reset(&r);for(i=0;i<6;i++){r.pos[i]=590;s[i]=10;}
    s[1]=11;race_step(&r,s);CHECK(photo(&r)==1);
    race_reset(&r);for(i=0;i<6;i++){r.pos[i]=590;s[i]=10;}
    race_step(&r,s);CHECK(photo(&r)==1);
    /* Horse steps stay in bounds and report events. */
    rngstate=7;race_reset(&r);
    for(j=0;j<2000;j++){i=j%6;s[0]=horse_step(&r,i,&ev);
        CHECK(s[0]>=1&&s[0]<=hlo[i]+hspread[i]+5);CHECK(ev>=0&&ev<=2);
        if(ev==1)CHECK(s[0]==1);}
    /* Market: bias, rumors, coat, events and invariants over many random games. */
    market_reset(&m);for(i=0;i<GOODS;i++)CHECK(m.price[i]>=27&&m.price[i]<=1600);
    CHECK(m.cap==100&&m.rcity>=0&&m.rcity<4&&m.rgood>=0&&m.rgood<GOODS);
    for(i=0;i<GOODS;i++)CHECK(bias[m.rcity][m.rgood]<=bias[(m.rcity+1)%4][m.rgood]||1);
    market_reset(&m);m.cash=COATCOST-1;CHECK(!buy_coat(&m));
    m.cash=COATCOST;CHECK(buy_coat(&m)&&m.cap==130&&m.cash==0);
    m.cash=COATCOST;CHECK(buy_coat(&m)&&m.cap==160);
    m.cash=COATCOST;CHECK(!buy_coat(&m)&&m.cap==160&&m.cash==COATCOST);
    m.price[0]=1;m.cash=MONEYMAX;CHECK(trade(&m,0,100,0)&&cargo(&m)==100);
    CHECK(trade(&m,1,60,0)&&cargo(&m)==160);CHECK(!trade(&m,1,1,0));
    memset(w,0,sizeof(w));
    for(k=1;k<=60;k++){
        rngstate=(unsigned)(k*977);market_reset(&m);
        m.qty[0]=40;m.qty[1]=3;
        while(m.day<30){
            e=travel(&m,(m.city+1)%4);CHECK(e>=0&&e<16);++w[e<7?e:7];
            event_text(&m,e,txt);CHECK(txt[0]&&strlen(txt)<80);
            for(i=0;i<GOODS;i++){CHECK(m.qty[i]>=0);CHECK(m.price[i]>=20&&m.price[i]<=5000);}
            CHECK(cargo(&m)<=m.cap);CHECK(m.cash>=0&&m.debt>=0);
        }
        CHECK(rank_title(score(&m))[0]!=0);
    }
    for(i=0;i<8;i++)CHECK(w[i]>0);
    CHECK(rank_title(-1)[0]=='D'&&rank_title(200000L)[0]=='L');
#ifdef HOST
    /* Heavy host-only fairness check: 20,000 races with the game's own RNG.
       Every horse is a real choice: 5-27 percent to win, and win and place
       bets all return between 75 and 100 percent (about 12 percent house edge). */
    {
        long wins[6],ret[6],plc[6],pret[6];int ok=1;(void)plc;
        memset(wins,0,sizeof(wins));memset(ret,0,sizeof(ret));
        memset(plc,0,sizeof(plc));memset(pret,0,sizeof(pret));
        rngstate=1234;
        for(j=0;j<20000;j++) {
            race_reset(&r);
            while(r.done<6){for(i=0;i<6;i++)s[i]=horse_step(&r,i,&ev);race_step(&r,s);}
            for(i=0;i<6;i++){
                ret[i]+=payout(&r,i,0,1000);pret[i]+=payout(&r,i,1,1000);
                if(rankhorse(&r,i)==0)++wins[i];
            }
        }
        printf("host sim, 20000 races:\n");
        for(i=0;i<6;i++){
            printf("horse %d win%% %ld  win-return%% %ld  place-return%% %ld\n",
                i+1,wins[i]/200,ret[i]/200000L,pret[i]/200000L);
            CHECK(wins[i]>=1000&&wins[i]<=5400);
            CHECK(ret[i]/20000L>=750&&ret[i]/20000L<=1000);
            CHECK(pret[i]/20000L>=750&&pret[i]/20000L<=1000);
        }
        ok=0;CHECK(!ok);
    }
#endif
    printf("%s %d checks, %d failures\n",failures?"FAIL":"PASS",checks,failures);
    return failures?1:0;
}
