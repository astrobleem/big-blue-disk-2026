/* Original silent cartoon. MSC 6 C89, /G0 /AS. No interrupt hooks or sound. */
#ifndef HOST_TEST
#include <dos.h>
#include <conio.h>
#else
#define far
#endif
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#ifdef HOST_TEST
#define _fmemset memset
#endif
static unsigned char far *frame, far *previous;
static unsigned char far *video=(unsigned char far *)0xb8000000UL;
#define MAX_TOUCH 4096
static unsigned touch_a[MAX_TOUCH],touch_b[MAX_TOUCH];
static unsigned *last_touch=touch_a,*this_touch=touch_b;
static unsigned last_count,this_count,max_touch;
static unsigned char seen[4096];
static int overflow,last_full;
static unsigned fallback_frames;
static unsigned long clear_bytes,compare_bytes;
static int phase, current_q;
#include "FONT.H"
static int mn(int a,int b){return a<b?a:b;}
static void pixel(int x,int y,int c){
    unsigned n,bit; unsigned char b;
    if(x<0||x>=320||y<0||y>=200)return;
    n=((unsigned)(y&3)<<13)+(unsigned)(y>>2)*160+(unsigned)x/2;
    bit=1U<<(n&7);
    if(!(seen[n>>3]&bit)){
        seen[n>>3]|=(unsigned char)bit;
        if(this_count<MAX_TOUCH)this_touch[this_count++]=n;
        else overflow=1;
    }
    b=frame[n];
    frame[n]=(unsigned char)((x&1)?((b&240)|c):((b&15)|(c<<4)));
}
static void line(int x,int y,int u,int v,int c){
    int dx,dy,sx,sy,e,t;dx=abs(u-x);dy=-abs(v-y);
    sx=x<u?1:-1;sy=y<v?1:-1;e=dx+dy;
    for(;;){pixel(x,y,c);if(x==u&&y==v)break;t=e*2;
        if(t>=dy){e+=dy;x+=sx;}if(t<=dx){e+=dx;y+=sy;}}
}
static void box(int x,int y,int u,int v,int c){
    line(x,y,u,y,c);line(u,y,u,v,c);line(u,v,x,v,c);line(x,v,x,y,c);
}
static int cx[16]={1000,923,707,382,0,-382,-707,-923,-1000,-923,-707,-382,0,382,707,923};
static int cy[16]={0,382,707,923,1000,923,707,382,0,-382,-707,-923,-1000,-923,-707,-382};
static int scale(int r,int v){return (int)((long)r*v/1000L);}
static void circle(int x,int y,int r,int c){
    int k,n;for(k=0;k<16;k++){n=(k+1)&15;line(x+scale(r,cx[k]),y+scale(r,cy[k]),x+scale(r,cx[n]),y+scale(r,cy[n]),c);}
}
static void blades(int x,int y,int r){
    int k,n;for(k=0;k<4;k++){n=(k*4+phase)&15;line(x,y,x+scale(r,cx[n]),y+scale(r,cy[n]),15);}
}
static void hat(int x,int y){line(x-8,y,x+8,y,15);box(x-5,y-5,x+4,y,15);}
static void man(int x,int y,int p,int h){
    int stride=5,brush=0;
    if(p==0){stride=(current_q/40)%4;stride=stride==0?3:stride==2?7:5;}
    if(p==6){brush=(current_q/30)%8;brush=brush<4?brush:7-brush;brush-=1;}
    if(p==2){circle(x,y-5,3,15);line(x+3,y-5,x+15,y-5,15);line(x+9,y-5,x+12,y-12,15);line(x+15,y-5,x+21,y-10,15);line(x+15,y-5,x+21,y,15);}
    else if(p==3){circle(x-5,y-16,3,15);line(x-3,y-13,x+2,y-5,15);line(x+2,y-5,x+12,y-5,15);line(x+12,y-5,x+16,y+3,15);line(x,y-9,x+8,y-7,15);hat(x-5,y-20);}
    else {circle(x,y-19,3,15);line(x,y-16,x,y-8,15);line(x,y-8,x-stride,y,15);line(x,y-8,x+stride,y,15);
        line(x,y-13,x-6,(p==4||p==5)?y-24:y-9,15);
        line(x,y-13,x+8,p==5?y-24:p==6?y-17+brush:(p==1||p==4)?y-17:y-9,15);
        if(h)hat(x,y-23);}
}
static void fish(int x,int y){line(x-10,y,x,y-4,15);line(x,y-4,x+10,y,15);line(x+10,y,x,y+4,15);line(x,y+4,x-10,y,15);line(x+10,y,x+16,y-5,15);line(x+16,y-5,x+16,y+5,15);line(x+16,y+5,x+10,y,15);}
static void text(int x,int y,char *s,int c){
    int a,b;unsigned char row;
    while(*s){if(*s>='A'&&*s<='Z')for(a=0;a<7;a++){row=glyphs[*s-'A'][a];for(b=0;b<5;b++)if(row&(16>>b))pixel(x+b,y+a,c);}x+=6;s++;}
}
static void death(int x,int y){
    if(current_q<100){circle(206,149,36,15);line(180,144,235,154,15);line(180,154,235,144,15);}
    x=y; /* arguments reserved for authoring DSL */
}
#include "SCENES.H"
#ifndef HOST_TEST
static void mode(int m){union REGS r;r.x.ax=m;int86(0x10,&r,&r);}
static unsigned long now(void){
    unsigned long t;unsigned flags;
    _asm pushf
    _asm pop flags
    _asm cli
    t=*(unsigned long far *)0x0040006cUL;
    _asm push flags
    _asm popf
    return t;
}
static unsigned long age(unsigned long a,unsigned long b){return a>=b?a-b:a+0x1800b0UL-b;}
#endif
static unsigned changed,frames,worst,worst_ticks;
static void draw(int n,int q){
    unsigned i,k,count=0;unsigned *swap;
    /* Clear only bytes touched by the previous vectors. Frame buffers use
       native Tandy bank offsets, so committing a byte needs no row division. */
    if(last_full){_fmemset(frame,0,32768U);clear_bytes+=32768UL;}
    else{for(i=0;i<last_count;i++)frame[last_touch[i]]=0;clear_bytes+=last_count;}
    memset(seen,0,sizeof(seen));this_count=0;overflow=0;
    current_q=q;scene(n,q);
    if(this_count>max_touch)max_touch=this_count;
    if(overflow||last_full){
        fallback_frames++;compare_bytes+=32768UL;
        for(k=0;k<32768U;k++)if(frame[k]!=previous[k]){video[k]=frame[k];previous[k]=frame[k];count++;}
    }else{
        compare_bytes+=(unsigned long)last_count+this_count;
        for(i=0;i<last_count;i++){k=last_touch[i];if(frame[k]!=previous[k]){video[k]=frame[k];previous[k]=frame[k];count++;}}
        for(i=0;i<this_count;i++){k=this_touch[i];if(frame[k]!=previous[k]){video[k]=frame[k];previous[k]=frame[k];count++;}}
    }
    last_full=overflow;last_count=this_count;swap=last_touch;last_touch=this_touch;this_touch=swap;
    changed=count;if(count>worst)worst=count;frames++;
}
#ifndef HOST_TEST
int main(int argc,char **argv){
    union REGS r;int old,n=0,q=0,key,paused=0,qa=0,auto_mode=0;
    unsigned t=0,base=0,total=0,k,dumped=65535,cost;
    unsigned long origin,last,stamp,delta,drawstart;FILE *f;char filename[13];
    if(argc>1 && strcmp(argv[1],"/QA")==0)qa=1;
    if(argc>1 && strcmp(argv[1],"/AUTO")==0)auto_mode=1;
    frame=(unsigned char far *)_fmalloc(32768U);previous=(unsigned char far *)_fmalloc(32768U);
    if(!frame||!previous){puts("Not enough free DOS memory for the cartoon.");return 1;}
    _fmemset(frame,0,32768U);_fmemset(previous,0,32768U);r.h.ah=15;int86(0x10,&r,&r);old=r.h.al;mode(9);
    for(k=0;k<sizeof(durations)/sizeof(durations[0]);k++)total+=durations[k];
restart:
    t=base=n=0;paused=0;origin=last=now();draw(0,0);
    while(t<total){
        stamp=now();delta=age(stamp,last);last=stamp;
        if(kbhit()){
            key=getch();if(key==27)break;
            if(key=='p'||key=='P'||key==' ')paused=!paused;
            if(key=='r'||key=='R'){t=base=n=0;paused=0;origin=stamp;draw(0,0);}
        }
        if(paused)origin+=delta;
        t=qa?(unsigned)(t+3):(unsigned)age(stamp,origin);
        if(t>=total)break;
        while(n<15 && t>=base+durations[n]){base+=durations[n];n++;}
        q=(int)((unsigned long)(t-base)*1000UL/durations[n]);
        if(qa || (!paused && t%3==0 && age(stamp,origin)>0)){
            static unsigned drawn=65535;if(t!=drawn){
                drawstart=now();draw(n,q);drawn=t;cost=(unsigned)age(now(),drawstart);if(cost>worst_ticks)worst_ticks=cost;
                if(qa && q>=950 && dumped!=(unsigned)n){
                    sprintf(filename,"S%02d.RAW",n+1);f=fopen(filename,"wb");
                    if(f){for(k=0;k<32000;k++){unsigned y=k/160;unsigned offset=(y&3)*8192+(y>>2)*160+k%160;fputc(frame[offset],f);}fclose(f);}dumped=n;
                }
            }
        }
    }
    if(qa||auto_mode){f=fopen(qa?"QA.TXT":"PLAY.TXT","w");if(f){fprintf(f,"Original scene engine completed. Frames %u; peak changed bytes %u; timeline ticks %u; elapsed ticks %lu; worst render ticks %u; final scene %d.\n",frames,worst,total,age(now(),origin),worst_ticks,n+1);fprintf(f,"Cleared packed bytes %lu; compared packed bytes %lu; max touched bytes %u; fallback frames %u.\n",clear_bytes,compare_bytes,max_touch,fallback_frames);fclose(f);}}
    if(!qa && !auto_mode && t>=total){while(1){key=getch();if(key==27)break;if(key=='r'||key=='R')goto restart;}}
    mode(old);_ffree(frame);_ffree(previous);
    if(qa||auto_mode){r.h.ah=15;int86(0x10,&r,&r);f=fopen("CLEANUP.TXT","w");if(f){fprintf(f,"Previous mode %d; restored mode %d; far buffers freed; no hooks installed.\n",old,r.h.al);fclose(f);}}
    return 0;
}
#endif
