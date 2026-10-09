#include <string.h>
#include "GAME.H"
int inside(int x,int y){return x>=0&&y>=0&&x<GW&&y<GH;}
static int hard(int t){return t==WALL||t==WATER||t==DOOR;}
static int blocked(Game *s,int x,int y)
{
    int t;if(!inside(x,y))return 1;t=s->tile[y][x];
    return hard(t)||t==RUBBLE||t==FOREST||s->fire[y][x];
}
static int critical(int t)
{
    return t==GEM||t==KEY||t==WHIP||t==STAIRS||t==PORTAL_A||t==PORTAL_B;
}
static int add(int n,int amount,int cap){return n>cap-amount?cap:n+amount;}
const char *room_name(int room)
{
    static const char *names[ROOMS]={"1 / ENTRY DUNGEON","2 / RIVER PASSAGE","3 / EMBER CAVERNS"};
    return names[room>=0&&room<ROOMS?room:0];
}
const char *room_tip(int room)
{
    return room==0?"Find the key. Whip rubble; grenade the barrel cluster. > descends.":
        room==1?"The river has a bridge. Keys open D; paired O portals link areas.":
        "Water stops fire. W clears burning forest. The last > ends this descent.";
}
static void load(Game *s,int room,int gems,int keys,int whips)
{
    int x,y,i;
    static const unsigned char e0[8][2]={{9,8},{10,6},{11,10},{18,5},{19,10},{22,6},{24,11},{26,4}};
    static const unsigned char e1[10][2]={{8,5},{8,10},{10,4},{10,12},{18,5},{21,7},{23,10},{26,5},{26,13},{20,12}};
    static const unsigned char e2[12][2]={{8,5},{8,11},{11,6},{11,10},{14,4},{14,12},{18,5},{18,11},{21,6},{21,10},{23,4},{23,13}};
    memset(s,0,sizeof(*s));s->room=room;s->px=3;s->py=8;s->dx=1;
    s->gems=s->start_gems=gems;s->keys=s->start_keys=keys;s->whips=s->start_whips=whips;
    for(y=0;y<GH;y++)for(x=0;x<GW;x++)
        s->tile[y][x]=(unsigned char)(x==0||y==0||x==GW-1||y==GH-1?WALL:FLOOR);
    s->tile[8][GW-2]=STAIRS;
    if(room==0) {
        for(y=1;y<GH-1;y++)s->tile[y][17]=WALL;
        s->tile[8][17]=DOOR;s->tile[5][6]=KEY;
        for(y=4;y<=6;y++)s->tile[y][8]=RUBBLE;
        s->tile[10][10]=FOREST;s->tile[11][10]=FOREST;
        s->barrel[8][8]=s->barrel[8][10]=s->barrel[7][10]=s->barrel[10][10]=1;
        s->tile[10][10]=FLOOR;
        s->tile[11][6]=s->tile[4][12]=s->tile[5][20]=s->tile[13][25]=GEM;
        s->tile[4][5]=s->tile[12][20]=WHIP;
        for(i=0;i<8;i++)s->enemy[e0[i][1]][e0[i][0]]=1;
    } else if(room==1) {
        for(y=1;y<GH-1;y++)s->tile[y][13]=s->tile[y][14]=WATER;
        s->tile[8][13]=s->tile[8][14]=FLOOR;s->tile[8][12]=DOOR;
        s->tile[4][5]=KEY;s->tile[4][9]=FOREST;
        s->tile[6][20]=s->tile[7][20]=RUBBLE;
        s->barrel[8][8]=s->barrel[8][10]=s->barrel[6][9]=1;
        s->barrel[10][19]=s->barrel[10][21]=s->barrel[12][21]=s->barrel[12][23]=1;
        s->tile[3][19]=PORTAL_A;s->tile[12][25]=PORTAL_B;
        s->tile[12][5]=s->tile[5][10]=s->tile[12][17]=s->tile[4][22]=s->tile[10][27]=GEM;
        s->tile[5][4]=s->tile[5][17]=WHIP;
        for(i=0;i<10;i++)s->enemy[e1[i][1]][e1[i][0]]=1;
    } else {
        for(y=3;y<=12;y++)for(x=10;x<=22;x++)
            if(y!=8&&(x+y)%3!=0)s->tile[y][x]=FOREST;
        for(y=1;y<GH-1;y++) {
            s->tile[y][24]=s->tile[y][25]=WATER;s->tile[y][26]=WALL;
        }
        s->tile[8][24]=s->tile[8][25]=s->tile[12][24]=s->tile[12][25]=FLOOR;
        s->tile[8][26]=DOOR;s->tile[12][6]=KEY;
        s->tile[4][6]=PORTAL_A;s->tile[11][20]=PORTAL_B;
        s->tile[10][20]=s->tile[12][20]=s->tile[11][19]=s->tile[11][21]=FLOOR;
        for(x=8;x<=22;x+=2)s->barrel[8][x]=1;
        s->tile[4][5]=s->tile[12][5]=s->tile[2][18]=s->tile[13][22]=GEM;
        s->tile[5][5]=s->tile[12][18]=WHIP;
        for(i=0;i<12;i++) {
            x=e2[i][0];y=e2[i][1];s->tile[y][x]=FLOOR;s->enemy[y][x]=1;
        }
    }
    for(y=0;y<GH;y++)for(x=0;x<GW;x++) {
        if(s->enemy[y][x])s->monsters++;
        if(s->tile[y][x]==GEM)s->treasures++;
    }
}
void reset_room(Game *s,int room){load(s,room>=0&&room<ROOMS?room:0,40,0,20);}
void reset(Game *s){reset_room(s,0);}
void retry_room(Game *s)
{
    int room=s->room,g=s->start_gems,k=s->start_keys,w=s->start_whips,r=s->retries;
    load(s,room,g,k,w);s->retries=add(r,1,30000);s->message=7;
}
void next_room(Game *s)
{
    int g=s->gems,k=s->keys,w=s->whips,r=s->retries;
    if(s->room<ROOMS-1) {
        load(s,s->room+1,g<40?40:g,k,w<20?20:w);s->retries=r;
    } else {
        s->finished=1;memset(s->g,0,sizeof(s->g));
        memset(s->flash,0,sizeof(s->flash));memset(s->fire,0,sizeof(s->fire));
        memset(s->fire_budget,0,sizeof(s->fire_budget));s->whip_anim=0;
    }
}
/* New gentle balance: contact costs two gems; no gems means a free retry. */
static int contact(Game *s,int x,int y)
{
    if(!s->enemy[y][x])return 0;
    if(!s->gems){retry_room(s);return 1;}
    s->enemy[y][x]=0;s->cleared++;s->gems=s->gems>2?s->gems-2:0;s->message=1;
    return 0;
}
int move_player(Game *s,int dx,int dy)
{
    int x,y,bx,by,t,nx,ny,other;
    if(s->finished||(!dx&&!dy)||dx < -1||dx > 1||dy < -1||dy > 1)return 0;
    s->dx=dx;s->dy=dy;x=s->px+dx;y=s->py+dy;
    if(!inside(x,y))return 0;
    if(dx&&dy&&(blocked(s,s->px+dx,s->py)||blocked(s,s->px,s->py+dy)))return 0;
    t=s->tile[y][x];
    if(t==DOOR) {
        if(!s->keys){s->message=3;return 0;}
    } else if(blocked(s,x,y)) {s->message=t==WATER?10:s->fire[y][x]?9:0;return 0;}
    if(s->barrel[y][x]) {
        if(dx&&dy)return 0;bx=x+dx;by=y+dy;
        if(blocked(s,bx,by)||s->barrel[by][bx]||s->enemy[by][bx]||critical(s->tile[by][bx]))return 0;
        s->barrel[y][x]=0;s->barrel[by][bx]=1;
    }
    if(contact(s,x,y))return 0;
    if(t==DOOR){s->keys--;s->tile[y][x]=FLOOR;}
    s->px=x;s->py=y;
    if(t==GEM){s->gems=add(s->gems,5,999);s->collected++;s->tile[y][x]=FLOOR;s->message=5;}
    if(t==KEY){s->keys=add(s->keys,1,99);s->tile[y][x]=FLOOR;s->message=4;}
    if(t==WHIP){s->whips=add(s->whips,5,999);s->tile[y][x]=FLOOR;s->message=6;}
    if(t==PORTAL_A||t==PORTAL_B) {
        if(!s->portal_latch) {
            other=t==PORTAL_A?PORTAL_B:PORTAL_A;
            for(ny=1;ny<GH-1;ny++)for(nx=1;nx<GW-1;nx++)
                if((int)s->tile[ny][nx]==other&&!s->enemy[ny][nx]&&!s->barrel[ny][nx]&&!s->fire[ny][nx]) {
                    s->px=nx;s->py=ny;s->portal_latch=1;s->message=8;return 1;
                }
        }
        s->portal_latch=1;
    } else s->portal_latch=0;
    if(t==STAIRS)next_room(s);
    return 1;
}
int throw_grenade(Game *s)
{
    int i;if(s->finished||s->cooldown)return 0;
    for(i=0;i<GN;i++)if(!s->g[i].active) {
        s->g[i].active=1;s->g[i].x=s->px;s->g[i].y=s->py;
        s->g[i].dx=s->dx;s->g[i].dy=s->dy;s->g[i].travel=4;
        s->g[i].fuse=12;s->cooldown=4;return 1;
    }
    return 0;
}
/* Documented fallback: one charge, eight neighbors, no random break chance. */
int use_whip(Game *s)
{
    int dx,dy,x,y,t;
    if(s->finished||s->whip_cooldown)return 0;
    if(!s->whips){s->message=2;return 0;}
    s->whips--;s->whip_cooldown=6;s->whip_anim=3;
    for(dy=-1;dy<=1;dy++)for(dx=-1;dx<=1;dx++)if(dx||dy) {
        x=s->px+dx;y=s->py+dy;if(!inside(x,y))continue;
        t=s->tile[y][x];if(hard(t))continue;
        if(dx&&dy&&(hard(s->tile[s->py][x])||hard(s->tile[y][s->px])))continue;
        if(s->enemy[y][x]){s->enemy[y][x]=0;s->cleared++;}
        if(t==RUBBLE||t==FOREST)s->tile[y][x]=FLOOR;
        s->fire[y][x]=s->fire_budget[y][x]=0;s->flash[y][x]=3;
    }
    return 1;
}
void apply_input(Game *s,int dx,int dy,int weapon)
{
    if(dx||dy)move_player(s,dx,dy);
    if(weapon==2)use_whip(s);else if(weapon==1)throw_grenade(s);
}
void input_reset(Input *in){memset(in,0,sizeof(*in));}
void input_key(Input *in,int ch,int scan)
{
    if(ch==27)in->quit=1;
    if(ch=='r'||ch=='R')in->retry=1;
    if(ch=='n'||ch=='N')in->skip=1;
    if(scan==75||ch=='4'||ch=='j'||ch=='J'){in->dx=-1;in->dy=0;}
    if(scan==77||ch=='6'||ch=='k'||ch=='K'){in->dx=1;in->dy=0;}
    if(scan==72||ch=='8'||ch=='i'||ch=='I'){in->dx=0;in->dy=-1;}
    if(scan==80||ch=='2'||ch=='m'||ch=='M'){in->dx=0;in->dy=1;}
    if(scan==71||ch=='7'||ch=='u'||ch=='U'){in->dx=-1;in->dy=-1;}
    if(scan==73||ch=='9'||ch=='o'||ch=='O'){in->dx=1;in->dy=-1;}
    if(scan==79||ch=='1'){in->dx=-1;in->dy=1;}
    if(scan==81||ch=='3'||ch==','){in->dx=1;in->dy=1;}
    if(ch==' '&&in->weapon!=2)in->weapon=1;
    if(ch=='w'||ch=='W')in->weapon=2;
}
int apply_controls(Game *s,const Input *in)
{
    if(in->quit)return 0;
    if(in->retry){if(s->finished)reset(s);else retry_room(s);}
    else if(in->skip)next_room(s);
    else apply_input(s,in->dx,in->dy,in->weapon);
    return 1;
}
static void ignite(Game *s,int x,int y,int budget)
{
    if(!inside(x,y)||s->tile[y][x]!=FOREST)return;
    if(!s->fire[y][x]){s->fire[y][x]=FIRE_LIFE;s->fire_budget[y][x]=(unsigned char)budget;}
    else if(budget>(int)s->fire_budget[y][x])s->fire_budget[y][x]=(unsigned char)budget;
}
void update_fire(Game *s)
{
    unsigned char before[GH][GW];int x,y,b;
    memcpy(before,s->fire,sizeof(before));
    for(y=0;y<GH;y++)for(x=0;x<GW;x++)if(before[y][x]) {
        if(--s->fire[y][x]==0){s->tile[y][x]=FLOOR;s->fire_budget[y][x]=0;}
    }
    /* Only the snapshot spreads; newborn cells cannot propagate this step. */
    for(y=0;y<GH;y++)for(x=0;x<GW;x++)if(before[y][x]==3&&s->fire_budget[y][x]) {
        b=s->fire_budget[y][x]-1;
        if(x>0&&!before[y][x-1])ignite(s,x-1,y,b);
        if(x+1<GW&&!before[y][x+1])ignite(s,x+1,y,b);
        if(y>0&&!before[y-1][x])ignite(s,x,y-1,b);
        if(y+1<GH&&!before[y+1][x])ignite(s,x,y+1,b);
    }
}
void detonate(Game *s,int x,int y)
{
    int qx[GW*GH+1],qy[GW*GH+1],head=0,tail=1;
    int cx,cy,dx,dy,nx,ny,dist,k,tx,ty,shield,t;
    if(!inside(x,y))return;s->barrel[y][x]=0;qx[0]=x;qy[0]=y;
    while(head<tail) {
        cx=qx[head];cy=qy[head++];s->blasts=add(s->blasts,1,30000);
        for(dy=-2;dy<=2;dy++)for(dx=-2;dx<=2;dx++) {
            nx=cx+dx;ny=cy+dy;dist=(dx<0?-dx:dx)+(dy<0?-dy:dy);
            if(dist>2||!inside(nx,ny)||hard(s->tile[ny][nx]))continue;
            shield=0;
            if(dx==0||dy==0)for(k=1;k<dist;k++) {
                tx=cx+(dx==0?0:(dx>0?k:-k));ty=cy+(dy==0?0:(dy>0?k:-k));
                if(hard(s->tile[ty][tx]))shield=1;
            }
            else if(hard(s->tile[cy][nx])||hard(s->tile[ny][cx]))shield=1;
            if(shield)continue;s->flash[ny][nx]=4;
            if(s->enemy[ny][nx]){s->enemy[ny][nx]=0;s->cleared++;}
            t=s->tile[ny][nx];if(t==RUBBLE)s->tile[ny][nx]=FLOOR;
            if(t==FOREST)ignite(s,nx,ny,2);
            if(s->barrel[ny][nx]) {
                s->barrel[ny][nx]=0;
                if(tail<GW*GH+1){qx[tail]=nx;qy[tail++]=ny;}
            }
        }
    }
}
void step(Game *s)
{
    unsigned char done[GH][GW];int x,y,i,nx,ny,dx,dy;
    if(s->finished)return;
    s->tick=(s->tick+1)%8;if(s->cooldown)s->cooldown--;
    if(s->whip_cooldown)s->whip_cooldown--;if(s->whip_anim)s->whip_anim--;
    for(y=0;y<GH;y++)for(x=0;x<GW;x++)if(s->flash[y][x])s->flash[y][x]--;
    update_fire(s);
    for(i=0;i<GN;i++)if(s->g[i].active) {
        if(s->g[i].travel) {
            nx=s->g[i].x+s->g[i].dx;ny=s->g[i].y+s->g[i].dy;
            if(blocked(s,nx,ny)||s->barrel[ny][nx]||
                (s->g[i].dx&&s->g[i].dy&&
                (blocked(s,s->g[i].x+s->g[i].dx,s->g[i].y)||
                 blocked(s,s->g[i].x,s->g[i].y+s->g[i].dy))))s->g[i].travel=0;
            else {s->g[i].x=nx;s->g[i].y=ny;s->g[i].travel--;}
        }
        if(--s->g[i].fuse==0){s->g[i].active=0;detonate(s,s->g[i].x,s->g[i].y);}
    }
    if(s->tick==0) {
        memset(done,0,sizeof(done));
        for(y=1;y<GH-1;y++)for(x=1;x<GW-1;x++)if(s->enemy[y][x]&&!done[y][x]) {
            dx=dy=0;if(s->px!=x)dx=s->px>x?1:-1;else if(s->py!=y)dy=s->py>y?1:-1;
            nx=x+dx;ny=y+dy;
            if(nx==s->px&&ny==s->py){if(contact(s,x,y))return;}
            else if(!blocked(s,nx,ny)&&!s->barrel[ny][nx]&&!s->enemy[ny][nx]&&
                !s->flash[ny][nx]&&s->tile[ny][nx]!=PORTAL_A&&s->tile[ny][nx]!=PORTAL_B) {
                s->enemy[y][x]=0;s->enemy[ny][nx]=1;done[ny][nx]=1;
            }
        }
    }
}
