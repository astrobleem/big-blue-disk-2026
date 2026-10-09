/* Original teaching demos for "Your Tandy Has Another Screen".
 * MSC6 /G0 /AS; plain DOS on a Tandy 1000 only. No resident code, no timer
 * or IRQ changes. Hooks INT 23h while running and restores it on exit.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <conio.h>
#include "DOSGUARD.H"
#include "DEMOCORE.H"

#define LAST_DEMO 3
#define TICKLO (*(volatile unsigned far *)0x0040006cUL)
#define VRAM ((unsigned char far *)0xb8000000UL)

static volatile int stopped, nextDemo, toggleKey, splitKey;
static int hooked, errorCode, modeChanged;
static unsigned char origMode;
static void (_interrupt _far *oldBreak)(void);
static unsigned frames, paletteLoads, retraceTimeouts;

static void _interrupt _far stopBreak(void) { stopped=1; }

static unsigned char getMode(void)
{
    union REGS r;
    r.h.ah=0x0f;int86(0x10,&r,&r);
    return (unsigned char)(r.h.al&0x7f);
}
static void setMode(unsigned char m)
{
    union REGS r;
    r.h.ah=0;r.h.al=m;int86(0x10,&r,&r);
}
/* BIOS text works in the Tandy graphics modes; colour is a palette index. */
static void printAt(unsigned row,unsigned col,const char *s,unsigned color)
{
    union REGS r;
    for(;*s;s++,col++) {
        r.h.ah=2;r.h.bh=0;r.h.dh=(unsigned char)row;r.h.dl=(unsigned char)col;
        int86(0x10,&r,&r);
        r.h.ah=9;r.h.al=(unsigned char)*s;r.h.bh=0;r.h.bl=(unsigned char)color;r.x.cx=1;
        int86(0x10,&r,&r);
    }
}
static void cleanup(void)
{
    unsigned i;
    if(modeChanged) {
        /* Palette registers are write-only. Load the identity palette, then
           let the BIOS mode set reinitialise them for the caller. */
        _disable();
        for(i=0;i<16;i++){outp(0x3da,0x10+i);outp(0x3de,i);}
        outp(0x3da,0x01);
        _enable();
        setMode(origMode);
        modeChanged=0;
    }
    if(hooked) { _dos_setvect(0x23,oldBreak); hooked=0; }
}
/* Wait until (status & mask)==want. Returns 1 on timeout (~4 BIOS ticks). */
static int waitStatus(unsigned char mask,unsigned char want)
{
    unsigned start=TICKLO,n=0;
    for(;;) {
        if(((unsigned char)inp(0x3da)&mask)==want)return 0;
        if(!(++n&255u)&&(unsigned)(TICKLO-start)>4u)return 1;
    }
}
/* Leaves the CPU at the start of a vertical retrace (status bit 3 set). */
static int syncRetrace(void)
{
    if(waitStatus(8,0)||waitStatus(8,8)){ retraceTimeouts++;return 1; }
    return 0;
}
/* Loading a palette register blanks video while the array address is 10h-1Fh,
   so keep it short, with interrupts off, and finish with address < 10h. */
static void loadPalette(unsigned first,unsigned count,const unsigned char *v)
{
    unsigned i;
    _disable();
    for(i=0;i<count;i++){outp(0x3da,0x10+first+i);outp(0x3de,v[i]&15);}
    outp(0x3da,0x01);
    _enable();
    paletteLoads++;
}
static void pollKeys(void)
{
    int key;
    /* One key per frame keeps the loop responsive and scriptable. */
    if(kbhit()) {
        key=getch();
        if(key==0){
            key=getch();
            if(key==72)splitKey=-1;
            else if(key==80)splitKey=1;
        }
        else if(key==27||key==3)stopped=1;
        else if(key==' ')nextDemo=1;
        else if(key=='t'||key=='T')toggleKey=1;
    }
}
static int enterGraphics(unsigned char m)
{
    setMode(m);modeChanged=1;
    if(getMode()!=m){ errorCode=5;return 0; }
    return 1;
}
static void drawScene(void)
{
    unsigned char row[ROW_BYTES];
    unsigned y;
    for(y=0;y<SCR_H;y++){
        sceneRow(y,row);
        _fmemcpy(VRAM+rowOffset(y),row,ROW_BYTES);
    }
}
static void staticPalette(void)
{
    unsigned char p[16];
    unsigned i;
    p[0]=0;
    for(i=0;i<WATER_N;i++)p[WATER_BASE+i]=rampColor(waterRamp,WATER_N,i,0);
    for(i=0;i<FLAME_N;i++)p[FLAME_BASE+i]=rampColor(flameRamp,FLAME_N,i,0);
    p[TEXT_IDX]=15;
    loadPalette(0,16,p);
}
/* Demo 1: pixels are drawn once. Motion is only palette rotation. */
static void demoCycle(void)
{
    unsigned char water[WATER_N],flame[FLAME_N];
    unsigned i,phase=0,tick=0;
    if(!enterGraphics(9))return;
    syncRetrace();staticPalette();
    drawScene();
    printAt(0,1,"1/3 PALETTE CYCLING: DRAWN ONCE",TEXT_IDX);
    printAt(24,1,"SPACE next demo   ESC exit",TEXT_IDX);
    while(!stopped&&!nextDemo&&!errorCode) {
        if(syncRetrace()){ errorCode=6;break; }
        if(!(tick++&1u)) {
            phase++;
            /* Water flows one way; the flame ramp runs the other way. */
            for(i=0;i<WATER_N;i++)
                water[i]=rampColor(waterRamp,WATER_N,i,phase);
            for(i=0;i<FLAME_N;i++)
                flame[i]=rampColor(flameRamp,FLAME_N,i,FLAME_N-(phase%FLAME_N));
            loadPalette(WATER_BASE,WATER_N,water);
            loadPalette(FLAME_BASE,FLAME_N,flame);
        }
        frames++;
        pollKeys();
    }
}
/* ---- Demo 2: tearing versus a flipped hidden page --------------------- */
/* PCjr/Tandy BIOS page services, INT 10h AH=05h: AL=80h reads, AL=83h sets
   BH=CRT page, BL=CPU page. Not described in the 1000 EX manual excerpts used
   for this piece; behaviour here was observed in emulation only. */
static void readPages(unsigned *crt,unsigned *cpu)
{
    union REGS r;
    r.x.ax=0x0580;r.x.bx=0;int86(0x10,&r,&r);
    *crt=r.h.bh;*cpu=r.h.bl;
}
static int setPages(unsigned crt,unsigned cpu)
{
    union REGS r;
    unsigned c,p;
    r.x.ax=0x0583;r.h.bh=(unsigned char)crt;r.h.bl=(unsigned char)cpu;
    int86(0x10,&r,&r);
    readPages(&c,&p);
    return c==crt&&p==cpu;
}
/* Count scan-line blanking pulses (status bit 0) after the retrace ends. */
static int waitScanlines(unsigned lines)
{
    while(lines--) {
        if(waitStatus(1,0)||waitStatus(1,1))return 1;
    }
    return 0;
}
static unsigned char gridRow[ROW8_BYTES];
static void drawBar(unsigned oldx,unsigned newx)
{
    unsigned lo,hi,y,bx,a,b;
    unsigned char far *row;
    a=oldx==NO_BAR?newx:oldx;b=newx;
    lo=(a<b?a:b)>>1;
    hi=((a>b?a:b)+BAR_W)>>1;
    if(hi>=ROW8_BYTES)hi=ROW8_BYTES-1;
    for(y=FLIP_TOP;y<FLIP_BOTTOM;y++) {
        row=VRAM+rowOffset8(y);
        for(bx=lo;bx<=hi;bx++)row[bx]=flipByte(bx,newx,gridRow[bx]);
    }
}
static void drawBackground(unsigned page)
{
    unsigned y,bx;
    unsigned char far *row;
    _fmemset(VRAM,0,16384u);
    for(y=FLIP_TOP;y<FLIP_BOTTOM;y++) {
        row=VRAM+rowOffset8(y);
        for(bx=0;bx<ROW8_BYTES;bx++)row[bx]=gridRow[bx];
    }
    (void)page;
}
static void marker(int clean)
{
    unsigned y,bx;
    unsigned char far *row;
    unsigned char v=(unsigned char)(((clean?MARK_CLEAN:MARK_TEAR)<<4)|(clean?MARK_CLEAN:MARK_TEAR));
    for(y=8;y<14;y++) {
        row=VRAM+rowOffset8(y);
        for(bx=66;bx<78;bx++)row[bx]=v;
    }
}
/* Label and marker go on both pages, once per mode change. */
static int showMode(int clean,unsigned shown,unsigned hidden)
{
    unsigned i,pg;
    for(i=0;i<2;i++) {
        pg=i?hidden:shown;
        if(!setPages(shown,pg))return 0;
        marker(clean);
        printAt(0,0,clean?"CLEAN: HIDDEN PAGE":"TEAR: BEAM RACE    ",TEXT_IDX);
    }
    return setPages(shown,clean?hidden:shown);
}
static void demoFlip(void)
{
    unsigned p9,p8,dummy,shown,hidden,frame=0,i,newx,lastx[2],cleanMode=0,hold=0,labelled=2;
    unsigned char pal[16];
    if(!enterGraphics(9))return;
    readPages(&p9,&dummy);
    if(!enterGraphics(8))return;
    readPages(&p8,&dummy);
    /* Only flip inside the 32K pair the BIOS itself uses for 320x200x16. */
    if((p8>>1)!=(p9>>1)||(p9&1u)){ errorCode=7;return; }
    shown=p8;hidden=p8^1u;
    for(i=0;i<ROW8_BYTES;i++)gridRow[i]=gridByte(i);
    for(i=0;i<16;i++)pal[i]=0;
    pal[BAR_COLOR]=15;pal[GRID_COLOR]=8;pal[MARK_TEAR]=4;pal[MARK_CLEAN]=10;pal[15]=15;
    syncRetrace();loadPalette(0,16,pal);
    for(i=0;i<2;i++) {
        unsigned pg=i?hidden:shown;
        if(!setPages(shown,pg)){ errorCode=8;return; }
        drawBackground(pg);
        printAt(24,0,"SPC NEXT T MODE ESC",TEXT_IDX);
        lastx[pg&1u]=NO_BAR;
    }
    while(!stopped&&!nextDemo&&!errorCode) {
        if(toggleKey){ toggleKey=0;cleanMode=!cleanMode;hold=1;frame=0; }
        else if(!hold&&frame>=240u){ cleanMode=!cleanMode;frame=0; }
        if(labelled!=cleanMode) {
            if(!showMode(cleanMode,shown,hidden)){ errorCode=8;break; }
            labelled=cleanMode;
        }
        newx=barPos(frame);
        if(cleanMode) {
            if(!setPages(shown,hidden)){ errorCode=8;break; }
            drawBar(lastx[hidden&1u],newx);lastx[hidden&1u]=newx;
            if(syncRetrace()){ errorCode=6;break; }
            if(!setPages(hidden,hidden)){ errorCode=8;break; }
            i=shown;shown=hidden;hidden=i;
        } else {
            if(!setPages(shown,shown)){ errorCode=8;break; }
            if(syncRetrace()||waitScanlines(90)){ errorCode=6;break; }
            drawBar(lastx[shown&1u],newx);lastx[shown&1u]=newx;
        }
        frame++;frames++;
        pollKeys();
    }
}
/* ---- Demo 3: change palette entries mid-frame -------------------------- */
static void splitLabel(unsigned split)
{
    char buf[24];
    sprintf(buf,"3/3 SPLIT LINE %3u  ",split);
    printAt(0,0,buf,TEXT_IDX);
}
static void demoSplit(void)
{
    unsigned char row[ROW_BYTES],pal[16];
    unsigned y,i,split=100;
    if(!enterGraphics(9))return;
    for(i=0;i<16;i++)pal[i]=0;
    for(i=0;i<SPLIT_N;i++)pal[1+i]=splitTop[i];
    pal[TEXT_IDX]=15;
    syncRetrace();loadPalette(0,16,pal);
    for(y=0;y<SCR_H;y++){
        splitRow(y,row);
        _fmemcpy(VRAM+rowOffset(y),row,ROW_BYTES);
    }
    splitLabel(split);
    printAt(24,0,"UP/DN MOVE SPC NEXT ESC",TEXT_IDX);
    while(!stopped&&!nextDemo&&!errorCode) {
        if(splitKey){ split=moveSplit(split,splitKey);splitKey=0;splitLabel(split); }
        if(syncRetrace()){ errorCode=6;break; }
        loadPalette(1,SPLIT_N,splitTop);
        if(waitScanlines(split)){ errorCode=6;break; }
        loadPalette(1,SPLIT_N,splitBottom);
        frames++;
        pollKeys();
    }
}
int main(int argc,char **argv)
{
    unsigned first=1,last=LAST_DEMO,d,flags;
    if(argc==2&&argv[1][0]>='1'&&argv[1][0]<='0'+LAST_DEMO&&!argv[1][1])
        first=last=argv[1][0]-'0';
    else if(argc!=1) {
        puts("Usage: SCREEN [1|2|3]; default runs all. Space next, Escape exits.");
        return 2;
    }
    puts("Tandy 1000 graphics demos. Plain DOS only. Space next, Esc exits.");
    _asm pushf
    _asm pop flags
    if(!(flags&0x200)) {
        puts("REFUSED: BIOS tick interrupts must be enabled.");return 3;
    }
    if(!DosSessionPermit()) {
        puts("REFUSED: needs plain DOS on a Tandy 1000 (not Windows).");return 3;
    }
    origMode=getMode();
    atexit(cleanup);
    oldBreak=_dos_getvect(0x23);_dos_setvect(0x23,stopBreak);hooked=1;
    for(d=first;d<=last&&!stopped&&!errorCode;d++) {
        nextDemo=0;
        if(d==1)demoCycle();
        else if(d==2)demoFlip();
        else if(d==3)demoSplit();
    }
    cleanup();
    if(errorCode==7)puts("REFUSED demo 2: BIOS video pages are not the expected 32K pair.");
    printf("DONE stop=%d error=%d frames=%u palette_loads=%u "
        "retrace_timeouts=%u mode_restored=%u\n",
        stopped,errorCode,frames,paletteLoads,retraceTimeouts,
        getMode()==origMode);
    return errorCode?4:0;
}
