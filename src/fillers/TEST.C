#include "HORSE.H"
#include "TRADE.H"
static int checks=0,failures=0;
#define CHECK(x) do {++checks;if(!(x)){printf("FAIL line %d\n",__LINE__);++failures;}}while(0)
int main(void)
{
    Race r;Market m;long n,a,b;int s[6],order[6],i,j,rank;
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
    for(i=0;i<6;i++){CHECK(rankhorse(&r,i)==0);CHECK(payout(&r,i,0,60)==60);CHECK(payout(&r,i,1,60)==40);}
    race_reset(&r);for(i=0;i<6;i++){r.pos[i]=590;s[i]=10+i;}
    race_step(&r,s);CHECK(rankhorse(&r,5)==0);CHECK(rankhorse(&r,0)==5);
    finish_order(&r,order);for(i=0;i<6;i++)CHECK(order[i]==5-i);
    CHECK(payout(&r,5,0,10)==60);CHECK(payout(&r,4,1,10)==20);
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
    printf("%s %d checks, %d failures\n",failures?"FAIL":"PASS",checks,failures);
    return failures?1:0;
}
