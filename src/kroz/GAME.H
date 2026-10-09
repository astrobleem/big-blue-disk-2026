#ifndef GAME_H
#define GAME_H
#define GW 30
#define GH 16
#define GN 4
#define ROOMS 3
#define FIRE_LIFE 6
enum { FLOOR, WALL, RUBBLE, FOREST, WATER, DOOR, GEM, KEY,
       WHIP, STAIRS, PORTAL_A, PORTAL_B };
typedef struct {
    unsigned char tile[GH][GW],barrel[GH][GW],enemy[GH][GW];
    unsigned char flash[GH][GW],fire[GH][GW],fire_budget[GH][GW];
    int room,finished,px,py,dx,dy,tick,cooldown,whip_cooldown,whip_anim;
    int gems,keys,whips,start_gems,start_keys,start_whips;
    int cleared,monsters,blasts,collected,treasures,portal_latch,message,retries;
    struct { int active,x,y,dx,dy,travel,fuse; } g[GN];
} Game;
typedef struct { int dx,dy,weapon,quit,retry,skip; } Input;
int inside(int x,int y);
void reset(Game *s);
void reset_room(Game *s,int room);
void retry_room(Game *s);
void next_room(Game *s);
const char *room_name(int room);
const char *room_tip(int room);
int move_player(Game *s,int dx,int dy);
int throw_grenade(Game *s);
int use_whip(Game *s);
void apply_input(Game *s,int dx,int dy,int weapon);
void input_reset(Input *in);
void input_key(Input *in,int ch,int scan);
int apply_controls(Game *s,const Input *in);
void step(Game *s);
void detonate(Game *s,int x,int y);
void update_fire(Game *s);
#endif
