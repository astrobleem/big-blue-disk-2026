#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "GAME.H"
static unsigned long checks=0;
static Game s,a,b;
#define C(v) do{checks++;if(!(v)){printf("FAIL %d: %s\n",__LINE__,#v);exit(1);}}while(0)
static void empty(Game *g)
{
    int x,y;memset(g,0,sizeof(*g));g->px=3;g->py=8;g->dx=1;
    g->gems=g->start_gems=40;g->whips=g->start_whips=20;
    for(y=0;y<GH;y++)for(x=0;x<GW;x++)
        if(x==0||y==0||x==GW-1||y==GH-1)g->tile[y][x]=WALL;
}
static int count(unsigned char cells[GH][GW])
{
    int x,y,n=0;for(y=0;y<GH;y++)for(x=0;x<GW;x++)if(cells[y][x])n++;return n;
}
/* Walk actual core movement along a BFS path, ignoring optional portal pads. */
static int walk_to(Game *g,int target)
{
    int q[GW*GH],prev[GW*GH],head=0,tail=0,i,n,x,y,nx,ny,goal=-1,at,path[GW*GH],len=0,t;
    static const int dx[4]={1,-1,0,0},dy[4]={0,0,1,-1};
    for(i=0;i<GW*GH;i++)prev[i]=-2;
    at=g->py*GW+g->px;prev[at]=-1;q[tail++]=at;
    while(head<tail&&goal<0) {
        n=q[head++];x=n%GW;y=n/GW;
        for(i=0;i<4;i++) {
            nx=x+dx[i];ny=y+dy[i];if(!inside(nx,ny))continue;at=ny*GW+nx;
            t=g->tile[ny][nx];
            if(prev[at]!=-2||t==WALL||t==WATER||t==FOREST||t==RUBBLE||
               (t==DOOR&&!g->keys)||t==PORTAL_A||t==PORTAL_B||g->fire[ny][nx]||g->barrel[ny][nx])continue;
            prev[at]=n;q[tail++]=at;if(t==target){goal=at;break;}
        }
    }
    if(goal<0)return 0;
    for(at=goal;prev[at]!=-1;at=prev[at])path[len++]=at;
    while(len){at=path[--len];C(move_player(g,at%GW-g->px,at/GW-g->py));}
    return 1;
}
int main(void)
{
    int x,y,i,r,accepted,oldg,oldw,oldk;Input in;
    reset(&s);C(s.room==0&&s.gems==40&&s.whips==20&&s.keys==0&&s.monsters==8);
    C(!move_player(&s,2,0));C(!move_player(&s,0,0));
    empty(&s);s.px=1;s.py=1;C(!move_player(&s,-1,0));C(!move_player(&s,0,-1));
    C(move_player(&s,1,1));C(s.px==2&&s.py==2);
    s.tile[2][3]=WATER;C(!move_player(&s,1,1));
    empty(&s);s.tile[8][4]=KEY;s.tile[8][5]=DOOR;
    C(move_player(&s,1,0));C(s.keys==1&&s.tile[8][4]==FLOOR);
    C(move_player(&s,1,0));C(!s.keys&&s.tile[8][5]==FLOOR);
    s.tile[8][6]=DOOR;C(!move_player(&s,1,0));C(!s.keys&&s.px==5);
    empty(&s);s.tile[8][4]=GEM;s.tile[8][5]=WHIP;
    C(move_player(&s,1,0));C(s.gems==45&&s.collected==1);
    C(move_player(&s,1,0));C(s.whips==25);
    s.tile[8][6]=GEM;s.gems=998;C(move_player(&s,1,0));C(s.gems==999);
    empty(&s);s.enemy[8][4]=1;C(move_player(&s,1,0));C(s.gems==38&&!s.enemy[8][4]&&s.cleared==1);
    s.gems=1;s.enemy[8][5]=1;C(move_player(&s,1,0));C(s.gems==0);
    s.enemy[8][6]=1;C(!move_player(&s,1,0));C(s.gems==40&&s.px==3&&s.retries==1);
    empty(&s);s.enemy[8][4]=1;s.tick=7;step(&s);C(s.gems==38&&!s.enemy[8][4]&&s.px==3);
    empty(&s);s.gems=0;s.enemy[8][4]=1;s.tick=7;step(&s);C(s.gems==40&&s.retries==1);
    /* Range, corner, protected pickup, charges and repeated input. */
    empty(&s);s.px=1;s.py=1;s.enemy[1][2]=s.enemy[2][1]=s.enemy[2][2]=s.enemy[1][3]=1;
    s.tile[2][1]=GEM;s.barrel[2][2]=1;
    C(use_whip(&s));C(s.cleared==3&&s.enemy[1][3]&&s.whips==19);
    C(s.tile[2][1]==GEM&&s.barrel[2][2]&&!s.blasts&&s.whip_anim==3);
    for(i=0;i<1000;i++)C(!use_whip(&s));
    for(i=0;i<6;i++)step(&s);C(!s.whip_cooldown&&!s.whip_anim);
    s.whips=0;C(!use_whip(&s));C(!s.whips);
    empty(&s);s.tile[8][2]=FOREST;s.fire[8][2]=6;s.fire_budget[8][2]=2;
    s.tile[8][4]=RUBBLE;s.tile[7][3]=WALL;s.enemy[7][4]=1;
    s.tile[9][3]=WATER;s.tile[9][4]=DOOR;
    C(use_whip(&s));C(s.tile[8][2]==FLOOR&&s.tile[8][4]==FLOOR&&!s.fire[8][2]);
    C(s.tile[7][3]==WALL&&s.tile[9][3]==WATER&&s.tile[9][4]==DOOR&&s.enemy[7][4]);
    empty(&s);accepted=0;
    for(i=0;i<1000;i++){oldw=s.whips;apply_input(&s,0,0,2);if(s.whips<oldw)accepted++;step(&s);C(s.whips>=0);}
    C(accepted==20&&s.whips==0);
    empty(&s);oldw=s.whips;apply_input(&s,0,0,2);C(s.whips==oldw-1&&s.py==8);
    empty(&s);input_reset(&in);
    for(i=0;i<1000;i++){input_key(&in,'W',0);input_key(&in,' ',0);}
    C(in.weapon==2&&!in.dx&&!in.dy);C(apply_controls(&s,&in));C(s.whips==19&&s.py==8&&!s.g[0].active);
    input_reset(&in);for(i=0;i<1000;i++)input_key(&in,'N',0);
    C(apply_controls(&s,&in));C(s.room==1);input_key(&in,27,0);a=s;
    C(!apply_controls(&s,&in));C(!memcmp(&s,&a,sizeof(s)));
    empty(&s);input_reset(&in);input_key(&in,'9',0);C(apply_controls(&s,&in));C(s.px==4&&s.py==7);
    input_reset(&in);input_key(&in,0,79);C(apply_controls(&s,&in));C(s.px==3&&s.py==8);
    /* Infinite grenades; bounds/fuse, protected tiles, shielding and chains. */
    empty(&s);C(throw_grenade(&s));for(i=0;i<100;i++)C(!throw_grenade(&s));
    for(i=0;i<4;i++)step(&s);C(s.g[0].x==7&&s.g[0].travel==0);
    for(i=0;i<8;i++)step(&s);C(!s.g[0].active&&s.px==3&&s.gems==40);
    empty(&s);s.px=1;s.py=1;s.dx=-1;s.dy=-1;C(throw_grenade(&s));step(&s);C(s.g[0].x==1&&s.g[0].y==1);
    empty(&s);s.dx=1;s.dy=1;s.tile[8][4]=DOOR;C(throw_grenade(&s));step(&s);C(s.g[0].x==3&&s.g[0].y==8);
    empty(&s);s.tile[8][4]=DOOR;s.tile[8][5]=GEM;s.enemy[8][5]=1;
    detonate(&s,3,8);C(s.tile[8][4]==DOOR&&s.tile[8][5]==GEM&&s.enemy[8][5]);
    empty(&s);s.tile[7][3]=GEM;s.tile[8][4]=KEY;s.tile[9][3]=WHIP;s.tile[8][2]=STAIRS;
    s.tile[7][4]=PORTAL_A;s.tile[9][2]=PORTAL_B;oldg=s.gems;oldw=s.whips;oldk=s.keys;
    detonate(&s,3,8);C(s.tile[7][3]==GEM&&s.tile[8][4]==KEY&&s.tile[9][3]==WHIP&&s.tile[8][2]==STAIRS);
    C(s.tile[7][4]==PORTAL_A&&s.tile[9][2]==PORTAL_B&&s.gems==oldg&&s.whips==oldw&&s.keys==oldk);
    memset(&s,0,sizeof(s));for(y=0;y<GH;y++)for(x=0;x<GW;x++)s.barrel[y][x]=1;
    detonate(&s,15,8);C(s.blasts==GW*GH&&count(s.barrel)==0);
    /* Paired arrival latch: no timed bounce, safe destination and re-entry. */
    empty(&s);s.tile[8][4]=PORTAL_A;s.tile[10][20]=PORTAL_B;
    C(move_player(&s,1,0));C(s.px==20&&s.py==10&&s.portal_latch);
    for(i=0;i<20;i++)step(&s);C(s.px==20&&s.py==10);
    C(move_player(&s,-1,0));C(!s.portal_latch);C(move_player(&s,1,0));C(s.px==4&&s.py==8);
    empty(&s);s.tile[8][4]=PORTAL_A;s.tile[10][20]=PORTAL_B;s.enemy[10][20]=1;
    C(move_player(&s,1,0));C(s.px==4&&s.py==8);
    /* Fire is bounded, stops at water/loot, and has scan-order-independent births. */
    empty(&s);s.tile[8][4]=FOREST;s.tile[8][5]=WATER;s.tile[7][4]=GEM;
    detonate(&s,4,8);C(s.fire[8][4]&&!s.fire[8][5]&&s.tile[7][4]==GEM);
    C(!move_player(&s,1,0));C(s.gems==40);C(use_whip(&s));C(!s.fire[8][4]&&s.tile[8][4]==FLOOR);
    empty(&s);for(y=1;y<GH-1;y++)for(x=1;x<GW-1;x++)s.tile[y][x]=FOREST;
    detonate(&s,15,8);for(i=0;i<100;i++)update_fire(&s);C(!count(s.fire));C(s.tile[1][1]==FOREST);
    empty(&a);empty(&b);
    for(y=3;y<13;y++)for(x=5;x<25;x++)a.tile[y][x]=b.tile[y][x]=FOREST;
    a.fire[7][11]=3;a.fire_budget[7][11]=2;a.fire[7][13]=3;a.fire_budget[7][13]=1;
    b.fire[GH-1-7][GW-1-11]=3;b.fire_budget[GH-1-7][GW-1-11]=2;
    b.fire[GH-1-7][GW-1-13]=3;b.fire_budget[GH-1-7][GW-1-13]=1;
    for(i=0;i<25;i++) {
        update_fire(&a);update_fire(&b);
        for(y=0;y<GH;y++)for(x=0;x<GW;x++) {
            C(a.fire[y][x]==b.fire[GH-1-y][GW-1-x]);
            C(a.fire_budget[y][x]==b.fire_budget[GH-1-y][GW-1-x]);
            C(a.tile[y][x]==b.tile[GH-1-y][GW-1-x]);
        }
    }
    /* Authored maps: coherent tiles, required key reachable, exits walkable. */
    reset(&s);
    for(r=0;r<ROOMS;r++) {
        C(s.room==r&&s.gems>=40&&s.whips>=20);
        C(s.monsters==count(s.enemy)&&s.monsters>=(r==0?8:10));
        for(y=0;y<GH;y++)for(x=0;x<GW;x++) {
            if(x==0||y==0||x==GW-1||y==GH-1)C(s.tile[y][x]==WALL);
            if(s.enemy[y][x]||s.barrel[y][x])C(s.tile[y][x]==FLOOR);
            if(s.tile[y][x]==PORTAL_A||s.tile[y][x]==PORTAL_B)C(!s.enemy[y][x]&&!s.barrel[y][x]);
        }
        C(walk_to(&s,KEY));C(s.keys>0);C(walk_to(&s,STAIRS));
    }
    C(s.finished);a=s;step(&s);C(!memcmp(&s,&a,sizeof(s)));
    C(!use_whip(&s)&&!throw_grenade(&s)&&!move_player(&s,1,0));
    reset(&s);s.gems=1;s.whips=0;s.keys=9;throw_grenade(&s);s.fire[2][2]=4;
    retry_room(&s);C(s.gems==40&&s.whips==20&&!s.keys&&!s.g[0].active&&!count(s.fire));
    for(r=0;r<ROOMS;r++){reset_room(&s,r);next_room(&s);C(r==2?s.finished:s.room==r+1);}
    reset_room(&s,-1);C(s.room==0);reset_room(&s,99);C(s.room==0);
    /* Repeated actual inputs may never escape the grid or wrap resources. */
    for(r=0;r<ROOMS;r++) {
        reset_room(&s,r);
        for(i=0;i<3000;i++) {
            apply_input(&s,(i%3)-1,((i/3)%3)-1,(i%2)+1);step(&s);
            C(inside(s.px,s.py));C(s.gems>=0&&s.keys>=0&&s.whips>=0);
            C(s.cleared+count(s.enemy)==s.monsters);
        }
    }
    C(sizeof(Game)<8192);
    printf("PASS %lu checks; state %u bytes; 3 key-to-stairs paths; bounded fire\n",checks,(unsigned)sizeof(Game));
    return 0;
}
