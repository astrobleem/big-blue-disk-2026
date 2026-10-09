#include <dos.h>
#include <conio.h>
#include <stdio.h>
#include <string.h>
#include "GAME.H"
static Game game;
static unsigned short old[2000];
static unsigned short frame[2000];
static void text(int x,int y,const char *s,int color)
{
    while(*s&&x<80) {frame[y*80+x]=(unsigned short)
        ((color<<8)|(unsigned char)*s++);x++;}
}
static void pair(int x,int y,char a,char b,int color)
{
    frame[y*80+x]=(unsigned short)((color<<8)|(unsigned char)a);
    frame[y*80+x+1]=(unsigned short)((color<<8)|(unsigned char)b);
}
static void render(void)
{
    int x,y,i,color,t;char a,b,msg[80];
    unsigned short far *v=(unsigned short far *)0xb8000000L;
    for(i=0;i<2000;i++)frame[i]=0x0720;
    text(2,0,"KROZ: GRENADES / ORIGINAL THREE-LEVEL SLICE",15);
    text(2,1,"Explore. Gems protect you. W whips nearby; SPACE throws a grenade.",7);
    for(y=0;y<GH;y++)for(x=0;x<GW;x++) {
        a=b=' ';color=7;t=game.tile[y][x];
        if(t==WALL){a=b=(char)219;color=9;}
        else if(t==RUBBLE){a=b=':';color=6;}
        else if(t==FOREST){a=b='T';color=2;}
        else if(t==WATER){a=b='~';color=9;}
        else if(t==DOOR){a=b='D';color=11;}
        else if(t==GEM){a=b='+';color=14;}
        else if(t==KEY){a='k';b=' ';color=14;}
        else if(t==WHIP){a='w';b=' ';color=15;}
        else if(t==STAIRS){a=b='>';color=10;}
        else if(t==PORTAL_A||t==PORTAL_B){a=b='O';color=13;}
        if(game.barrel[y][x]){a='[';b=']';color=14;}
        if(game.enemy[y][x]){a='o';b='o';color=12;}
        if(game.fire[y][x]){a='^';b='^';color=12;}
        if(game.flash[y][x]){a=b='*';color=14;}
        if(game.whip_anim&&x>=game.px-1&&x<=game.px+1&&
            y>=game.py-1&&y<=game.py+1&&game.flash[y][x]){a='~';b='~';color=15;}
        pair(x*2+2,y+3,a,b,color);
    }
    for(i=0;i<GN;i++)if(game.g[i].active) {
        a=(char)('0'+(game.g[i].fuse+3)/4);
        pair(game.g[i].x*2+2,game.g[i].y+3,'!',a,15);
    }
    a=(char)(game.dx>0?'>':game.dx<0?'<':game.dy>0?'v':'^');
    pair(game.px*2+2,game.py+3,'@',a,11);
    text(64,4,"DUNGEON TOOLS",15);
    text(64,6,"+ Gems protect",14);
    text(64,7,"k Key / D door",11);
    text(64,8,"w Whip pickup",15);
    text(64,9,"T Forest : Rock",2);
    text(64,10,"~ River O Portal",13);
    text(64,11,"^ Fire [] Barrel",12);
    text(64,12,"Move: arrows",7);
    text(64,13,"or numpad",7);
    text(64,14,"SPACE: grenade",15);
    text(64,15,"W: nearby whip",15);
    text(64,16,"R: free retry",7);
    text(64,17,"N: skip level",10);
    text(64,18,"ESC: return",7);
    text(2,20,room_name(game.room),10);
    sprintf(msg,"Gems %d  Keys %d  Whips %d  Grenades unlimited",game.gems,game.keys,game.whips);
    text(2,21,msg,14);
    text(2,22,game.message==1?"Monster contact costs gems. With no gems, you get a free retry.":
        game.message==2?"No whips left. Find w pickups or use your unlimited grenades.":
        game.message==3?"This door needs a key. Grenades and whips cannot break it.":
        game.message==4?"Key collected. Step onto D to unlock it.":
        game.message==5?"Gem pickup: five protection gems.":
        game.message==6?"Whip pickup: five nearby sweeps.":
        game.message==7?"Free checkpoint retry. Supplies and dungeon layout restored.":
        game.message==8?"Paired portal. Step off before using this pad again.":
        game.message==9?"Burning forest blocks this tile briefly. W clears it nearby.":
        game.message==10?"The river blocks travel. Look for a bridge or another route.":room_tip(game.room),7);
    if(game.finished)text(2,22,"DESCENT COMPLETE! R explores again. ESC returns to the magazine.",15);
    sprintf(msg,"Monsters removed %d/%d   Chain bursts %d   No grenade self-damage",game.cleared,game.monsters,game.blasts);
    text(2,23,msg,7);
    for(i=0;i<2000;i++)if(old[i]!=frame[i]){v[i]=frame[i];old[i]=frame[i];}
}
static unsigned long ticks(void)
{
    union REGS r;r.h.ah=0;int86(0x1a,&r,&r);
    return ((unsigned long)r.x.cx<<16)|r.x.dx;
}
int main(int argc,char **argv)
{
    union REGS r;unsigned long last,now,started;int ch,scan;Input in;
    int running=1,round=0,auto_mode,fast,i,old_mode;FILE *f;
    auto_mode=argc>1&&(!strcmp(argv[1],"/demo")||!strcmp(argv[1],"/qa"));
    fast=argc>1&&!strcmp(argv[1],"/qa");
    r.h.ah=15;int86(0x10,&r,&r);old_mode=r.h.al;
    r.x.ax=3;int86(0x10,&r,&r);r.h.ah=1;r.x.cx=0x2000;int86(0x10,&r,&r);
    for(i=0;i<2000;i++)old[i]=0xffff;
    reset(&game);render();last=started=ticks();
    while(running) {
        now=ticks();if(!fast&&now>=last&&now-last<2)continue;
        last=now;input_reset(&in);
        /* At most one move and weapon action per bounded input batch. W is never up. */
        for(i=0;i<8&&kbhit();i++) {
            ch=getch();scan=0;if(ch==0||ch==224){scan=getch();ch=0;}
            input_key(&in,ch,scan);
        }
        if(in.quit)break;
        if(auto_mode) {
            if(round==0)in.weapon=1;
            if(round==5)in.weapon=2;
            if(round==18)next_room(&game);
            if(round==36){next_room(&game);in.weapon=1;}
            if(round==40)detonate(&game,17,6);
            if(round==54)next_room(&game);
        }
        running=apply_controls(&game,&in);step(&game);render();round++;
        if(auto_mode&&(round==1||round==6||round==13||round==19||round==41||round==55)) {
            const char *name=round==1?"START.BIN":round==6?"WHIP.BIN":round==13?"CHAIN.BIN":
                round==19?"RIVER.BIN":round==41?"FIRE.BIN":"FINISH.BIN";
            f=fopen(name,"wb");if(f){fwrite(frame,sizeof(frame),1,f);fclose(f);}
        }
        if(auto_mode&&round==60)running=0;
    }
    r.x.ax=old_mode;int86(0x10,&r,&r);
    if(auto_mode){f=fopen("DEMO.LOG","w");if(f){
        r.h.ah=15;int86(0x10,&r,&r);
        fprintf(f,"ticks=%lu rounds=%d level=%d finished=%d state=%u mode=%d restored=%d\n",
            ticks()-started,round,game.room+1,game.finished,(unsigned)sizeof(Game),old_mode,r.h.al);fclose(f);}}
    return 0;
}
