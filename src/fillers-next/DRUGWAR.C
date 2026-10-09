#include "TRADE.H"
#include <time.h>
int main(int argc,char **argv)
{
    Market m;char cmd[64],txt[96];long g,n;int i,e,rc;
    rngstate=(unsigned)time(NULL);market_reset(&m);
    if(argc>1&&!strcmp(argv[1],"/demo")) {
        rngstate=123;market_reset(&m);
        while(m.day<30){trade(&m,0,1,0);travel(&m,(m.city+1)%4);trade(&m,0,1,1);}
        printf("DEMO day=%d cargo=%d cash=%ld bank=%ld debt=%ld score=%ld\n",
            m.day,cargo(&m),m.cash,m.bank,m.debt,score(&m));return 0;
    }
    for(;;) {
        clear();puts("DRUG WARS: BLUEPORT - original fictional trading game");
        puts("Imaginary contraband, prices and events. No real transactions.");
        printf("Day %d/30  %s  Coat %d/%d\n",m.day,cities[m.city],cargo(&m),m.cap);
        printf("Cash $%ld  Bank $%ld  Debt $%ld (5%% per travel day)\n",
            m.cash,m.bank,m.debt);
        for(i=0;i<4;i++)printf("%d %-14s price $%4d %-5s owned %d\n",
            i+1,goods[i],m.price[i],m.price[i]<250?"LOW":m.price[i]>750?"HIGH":"",
            m.qty[i]);
        printf("Rumor: %s runs cheapest in %s.\n",goods[m.rgood],cities[m.rcity]);
        puts("B buy  S sell  T travel  D deposit  W withdraw  P repay");
        printf("C bigger coat (+30 room, $%ld)\n",COATCOST);
        puts("F final score  R restart  Q quit. Numbers use Enter.");
        if(m.day==30)puts("Last market: sell or manage money, then F to finish.");
        printf("Action: ");if(!fgets(cmd,sizeof(cmd),stdin))break;
        if(cmd[0]=='q'||cmd[0]=='Q')break;
        if(cmd[0]=='r'||cmd[0]=='R'){market_reset(&m);continue;}
        if(cmd[0]=='f'||cmd[0]=='F') {
            printf("Final score $%ld = cash + bank + market cargo - debt.\n",score(&m));
            printf("Rank: %s\n",rank_title(score(&m)));
            puts("Press Enter to start a fresh 30-day game.");
            if(!fgets(cmd,sizeof(cmd),stdin))break;
            market_reset(&m);continue;
        }
        e=0;
        if(cmd[0]=='b'||cmd[0]=='B'||cmd[0]=='s'||cmd[0]=='S') {
            rc=ask("Commodity 1-4",4,&g);if(rc<0)break;if(!rc||!g)continue;
            rc=ask("Units",100,&n);if(rc<0)break;if(!rc||!n)continue;
            e=trade(&m,(int)g-1,n,cmd[0]=='s'||cmd[0]=='S');
        } else if(cmd[0]=='t'||cmd[0]=='T') {
            for(i=0;i<4;i++)printf("%d %s\n",i+1,cities[i]);
            rc=ask("Destination 1-4",4,&g);if(rc<0)break;if(!rc||!g)continue;
            e=travel(&m,(int)g-1);
            if(e>=0) {
                event_text(&m,e,txt);puts(txt);e=1;
            } else e=0;
        } else if(cmd[0]=='c'||cmd[0]=='C') {
            e=buy_coat(&m);
        } else {
            rc=ask("Amount",MONEYMAX,&n);if(rc<0)break;if(!rc||!n)continue;
            switch(cmd[0]) {
            case 'd':case 'D':e=move_money(&m.cash,&m.bank,n);break;
            case 'w':case 'W':e=move_money(&m.bank,&m.cash,n);break;
            case 'p':case 'P':e=repay(&m,n);break;
            }
        }
        puts(e?"Done.":"Cannot do that: check cash, stock, space or destination.");
        puts("Press Enter.");if(!fgets(cmd,sizeof(cmd),stdin))break;
    }
    return 0;
}
