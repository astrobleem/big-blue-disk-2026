#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define far
static unsigned char far *frame, far *previous;
static unsigned char far *video=(unsigned char far *)0xb8000000UL;
static int phase, current_q;
#include "FONT.H"
static int mn(int a,int b){return a<b?a:b;}
static void pixel(int x,int y,int c){
    unsigned n; unsigned char b;
    if(x<0||x>=320||y<0||y>=200)return;
    n=(unsigned)y*160+(unsigned)x/2;b=frame[n];
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

void reference(int n,int q,unsigned char *out){static unsigned char store[32000];frame=store;memset(frame,0,32000);current_q=q;scene(n,q);memcpy(out,frame,32000);}
