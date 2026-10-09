#include "HORSE.H"
#include <time.h>
static char *names[6]={"Blue Comet","Tin Biscuit","Velvet Bolt",
    "Copper Kite","Pocket Storm","Night Lantern"};
static char *notes[6]={"Favorite. Steady, rarely trips.",
    "Fast starter. Does she fade?","Smooth runner, stretch kick.",
    "Wild streak. Lunges late.","Longshot. Kicks hard.",
    "Wild card. Trips a lot."};
static char msg[64]="And they're off!";
static void bet_summary(int h,int place,long stake)
{
    int x=place?placex10[h]:winx10[h];
    printf("Bet: #%d %s | %s pays %d.%dx | Stake $%ld (fictional)\n",
        h+1,names[h],place?"PLACE":"WIN",x/10,x%10,stake);
}
static void results(Race *r,int h,int place,long stake,long gross,long cash)
{
    int i,j,tied,order[6];
    clear();
    puts("BIG BLUE DERBY - RESULTS");
    if(photo(r))puts("*** PHOTO FINISH - decided by a nose! ***");
    bet_summary(h,place,stake);
    if(gross>stake)printf("WINNER! Gross return $%ld (profit $%ld). Bankroll $%ld.\n",
        gross,gross-stake,cash);
    else if(gross)printf("Gross return $%ld (includes stake). Bankroll $%ld.\n",gross,cash);
    else printf("No return. Bankroll $%ld.\n",cash);
    puts("FINISH ORDER (equal places are exact ties)");
    finish_order(r,order);
    for(i=0;i<6;i++) {
        tied=0;
        for(j=0;j<6;j++)
            if(j!=order[i]&&!earlier(r,j,order[i]))tied=1;
        printf("Place %d %-5s Horse #%d  %s\n",rankhorse(r,order[i])+1,
            tied?"(tie)":"",order[i]+1,names[order[i]]);
    }
    puts("Enter: next race | R then Enter: restart | Q then Enter: quit");
}
static void frame(Race *r,int h,int place,long stake)
{
    int i,j,p;
#ifndef HOST
    union REGS v;v.h.ah=2;v.h.bh=0;v.h.dh=0;v.h.dl=0;int86(0x10,&v,&v);
#endif
    puts("BIG BLUE DERBY - original fictional-money racing");
    bet_summary(h,place,stake);
    puts("                                                   FINISH");
    for(i=0;i<6;i++) {
        p=r->pos[i]/12;if(p>50)p=50;
        printf("%d ",i+1);
        for(j=0;j<51;j++)putchar(j==p?'@':'.');
        printf("| %-13s\n  ",names[i]);
        for(j=0;j<p;j++)putchar(' ');
        printf("%s",r->time%2?"/=/>":"\\=\\>");
        for(j=p+4;j<57;j++)putchar(' ');
        putchar('\n');
    }
    printf("CALL: %-64s\n",msg);
    puts("ESC finishes animation quickly; all horses still finish.");
}
static int leader(Race *r)
{
    int i,b=0;
    for(i=1;i<HORSES;i++)if(r->pos[i]>r->pos[b])b=i;
    return b;
}
static void runrace(Race *r,int animate,int h,int place,long stake)
{
    int i,steps[6],ev,fast=!animate,lead=-1,l,stretch=0;
    race_reset(r);if(animate){clear();strcpy(msg,"And they're off!");}
    while(r->done<6) {
        for(i=0;i<6;i++) {
            steps[i]=horse_step(r,i,&ev);
            if(ev==1)sprintf(msg,"%s stumbles!",names[i]);
            else if(ev==2)sprintf(msg,"%s kicks for home!",names[i]);
        }
        race_step(r,steps);
        l=leader(r);
        if(r->done==1&&!stretch){stretch=1;
            sprintf(msg,"%s hits the wire!",names[l]);}
        else if(l!=lead){
            if(lead>=0&&r->pos[l]>r->pos[lead]&&r->time>4&&r->done==0)
                sprintf(msg,"%s takes the lead!",names[l]);
            lead=l;
        }
        if(!fast) {
            frame(r,h,place,stake);
#ifndef HOST
            {clock_t start=clock();
             while(clock()-start<CLOCKS_PER_SEC/10) {
                 if(kbhit()&&getch()==27){fast=1;break;}
             }}
#endif
        }
    }
}
int main(int argc,char **argv)
{
    Race r;long cash=1000,h,kind,stake,gross;int i,rc,x;
    if(argc>1&&!strcmp(argv[1],"/demo")) {
        rngstate=123;runrace(&r,0,0,0,0);
        for(i=0;i<6;i++)printf("Horse %d rank %d win=%ld place=%ld\n",
            i+1,rankhorse(&r,i)+1,payout(&r,i,0,10),payout(&r,i,1,10));
        printf("photo=%d\n",photo(&r));
        return 0;
    }
    rngstate=(unsigned)time(NULL);
    for(;;) {
        clear();printf("BIG BLUE DERBY       Fictional bankroll: $%ld\n",cash);
        puts("Real odds now: favorites pay less, longshots pay more.");
        puts("Place pays first or second. Gross return includes stake.");
        puts("Exact ties split affected winning slots; fractions round down.");
        puts("#  Horse          WIN    PLACE  Form");
        for(i=0;i<6;i++) {
            x=winx10[i];
            printf("%d  %-13s %2d.%dx  %2d.%dx  %s\n",i+1,names[i],x/10,x%10,
                placex10[i]/10,placex10[i]%10,notes[i]);
        }
        puts("0 at horse selection exits. Bankroll limit $100,000,000.");
        if(!cash){puts("Bankroll empty. Starting a fresh $1000 session.");cash=1000;}
        rc=ask("Horse 1-6",6,&h);if(rc<0|| (rc==1&&!h))break;if(!rc)continue;
        rc=ask("1 Win / 2 Place",2,&kind);if(rc<0)break;if(!rc||!kind)continue;
        rc=ask("Stake (maximum $10000)",cash<10000?cash:10000,&stake);
        if(rc<0)break;
        if(!rc||!stake)continue;
        if(cash>MONEYMAX-stake*9){puts("Bankroll limit reached.");break;}
        cash-=stake;runrace(&r,1,(int)h-1,kind==2,stake);
        gross=payout(&r,(int)h-1,kind==2,stake);
        cash+=gross;results(&r,(int)h-1,kind==2,stake,gross,cash);
        {char line[64];if(!fgets(line,sizeof(line),stdin))break;
         if(line[0]=='q'||line[0]=='Q')break;
         if(line[0]=='r'||line[0]=='R')cash=1000;}
    }
    return 0;
}
