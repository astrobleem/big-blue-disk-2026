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
static unsigned qaMode, qaStop, qaFrames;
static unsigned char origPage;
static unsigned origCursor, origShape, origCrt, origCpu;
static unsigned short origText[8192];
static unsigned char qaPalette[16];

static void _interrupt _far stopBreak(void) { stopped=1; }

static unsigned char getMode(void)
{
    union REGS r;
    r.h.ah=0x0f;int86(0x10,&r,&r);
    return (unsigned char)(r.h.al&0x7f);
}
static void readRawPages(unsigned *crt,unsigned *cpu)
{
    union REGS r;r.x.ax=0x0580;r.x.bx=0;int86(0x10,&r,&r);
    *crt=r.h.bh;*cpu=r.h.bl;
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
        {
            union REGS r;r.x.ax=0x0583;r.h.bh=(unsigned char)origCrt;r.h.bl=(unsigned char)origCpu;int86(0x10,&r,&r);
            _fmemcpy(VRAM,origText,sizeof origText);
        }
        if(origMode<=3||origMode==7) {
            union REGS r;
            r.h.ah=5;r.h.al=origPage;int86(0x10,&r,&r);
            r.h.ah=1;r.x.cx=origShape;int86(0x10,&r,&r);
            r.h.ah=2;r.h.bh=origPage;r.x.dx=origCursor;int86(0x10,&r,&r);
        }
        modeChanged=0;
    }
    if(hooked) { _dos_setvect(0x23,oldBreak); hooked=0; }
}
/* Wait until (status & mask)==want. Returns 1 on timeout (~4 BIOS ticks). */
static int waitStatus(unsigned char mask,unsigned char want)
{
    unsigned start=TICKLO,n=0;
    for(;;) {
        if(stopped)return 1;
        if(qaMode==4)return 1;
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
    for(i=0;i<count;i++){outp(0x3da,0x10+first+i);outp(0x3de,v[i]&15);qaPalette[first+i]=(unsigned char)(v[i]&15);}
    outp(0x3da,0x01);
    _enable();
    paletteLoads++;
}
static void applyKey(int key)
{
    if(key==27||key==3)stopped=1;
    else if(key==' ')nextDemo=1;
    else if(key=='t'||key=='T')toggleKey=1;
}
static void pollKeys(void)
{
    int key;
    if(qaMode) {
        if(++qaFrames>=2) {
            union REGS r;
            if(qaStop==2)int86(0x23,&r,&r);
            else if(qaStop==1)applyKey(27);
            else nextDemo=1;
        }
        return;
    }
    /* One key per frame keeps the loop responsive and scriptable. */
    if(kbhit()) {
        key=getch();
        if(key==0){
            key=getch();
            if(key==72)splitKey=-1;
            else if(key==80)splitKey=1;
        }
        else applyKey(key);
    }
}
static int enterGraphics(unsigned char m)
{
    unsigned c,p;
    /* We use only mode 8's single 16K frame. Its raw CPU/CRT selector
       must be the caller's already-used odd (16K) text window. No new
       page is selected and no 32K access or extra buffer is permitted. */
    if(m!=8||origCrt!=origCpu||!(origCpu&1u)||origCpu>7u){errorCode=7;return 0;}
    setMode(m);modeChanged=1;
    readRawPages(&c,&p);
    if(c!=origCrt||p!=origCpu){errorCode=7;return 0;}
    if(getMode()!=m){ errorCode=5;return 0; }
    return 1;
}
static void drawScene(void)
{
    unsigned char row[ROW8_BYTES];
    unsigned y;
    for(y=0;y<SCR_H;y++){
        sceneRow8(y,row);
        _fmemcpy(VRAM+rowOffset8(y),row,ROW8_BYTES);
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
static void capture(unsigned demo)
{
    FILE *f;char name[13];unsigned i;
    if(!qaMode)return;
    sprintf(name,"SC%u.RAW",demo);
    f=fopen(name,"wb");if(!f){errorCode=9;return;}
    for(i=0;i<16384u;i++)if(fputc(VRAM[i],f)==EOF){errorCode=9;break;}
    if(fclose(f))errorCode=9;
    sprintf(name,"SC%u.PAL",demo);f=fopen(name,"wb");
    if(!f){errorCode=9;return;}
    if(fwrite(qaPalette,1,16,f)!=16)errorCode=9;
    if(fclose(f))errorCode=9;
}
static void demoCycle(void)
{
    unsigned char water[WATER_N],flame[FLAME_N];
    unsigned i,phase=0,tick=0;
    if(!enterGraphics(8))return;
    if(syncRetrace()){errorCode=6;return;}staticPalette();
    drawScene();
    printAt(0,1,"1/3 PALETTE CYCLE",TEXT_IDX);
    printAt(24,1,"SPC NEXT ESC EXIT",TEXT_IDX);
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
    capture(1);
}
/* Mode 8 needs two demonstrably reserved 16K buffers for page flipping.
   Matching BIOS selectors is not an ownership proof. Until an EX/DOS
   reservation contract is qualified, refuse this experiment before any
   mode, palette, selector or VRAM write. Original proposal is preserved
   in Claude's branch/commit; no unsafe opt-in build flag is offered. */
static void demoFlip(void)
{
    errorCode=7;
}
/* ---- Demo 3: change palette entries mid-frame -------------------------- */
/* Bounded scan-line counting is shared by the retained raster experiment. */
static int waitScanlines(unsigned lines)
{
    while(lines--)if(waitStatus(1,0)||waitStatus(1,1))return 1;
    return 0;
}
static void splitLabel(unsigned split)
{
    char buf[24];
    sprintf(buf,"3/3 SPLIT %3u",split);
    printAt(0,0,buf,TEXT_IDX);
}
static void demoSplit(void)
{
    unsigned char row[ROW8_BYTES],pal[16];
    unsigned y,i,split=100;
    if(!enterGraphics(8))return;
    for(i=0;i<16;i++)pal[i]=0;
    for(i=0;i<SPLIT_N;i++)pal[1+i]=splitTop[i];
    pal[TEXT_IDX]=15;
    if(syncRetrace()){errorCode=6;return;}loadPalette(0,16,pal);
    for(y=0;y<SCR_H;y++){
        splitRow8(y,row);
        _fmemcpy(VRAM+rowOffset8(y),row,ROW8_BYTES);
    }
    splitLabel(split);
    printAt(24,0,"UP/DN SPC NEXT ESC",TEXT_IDX);
    while(!stopped&&!nextDemo&&!errorCode) {
        if(splitKey){ split=moveSplit(split,splitKey);splitKey=0;splitLabel(split); }
        if(syncRetrace()){ errorCode=6;break; }
        loadPalette(1,SPLIT_N,splitTop);
        if(waitScanlines(split)){ errorCode=6;break; }
        loadPalette(1,SPLIT_N,splitBottom);
        frames++;
        pollKeys();
    }
    capture(3);
}
int main(int argc,char **argv)
{
    unsigned first=1,last=LAST_DEMO,d,flags,i,before61;int textOK=1,cursorOK=1,hookOK;
    if(argc==4&&!strcmp(argv[1],"/qa")&&argv[2][0]>='1'&&argv[2][0]<='4'&&!argv[2][1]&&argv[3][0]>='0'&&argv[3][0]<='2'&&!argv[3][1]) {
        qaMode=argv[2][0]-'0';qaStop=argv[3][0]-'0';
        first=last=qaMode==4?1:qaMode;
    }
    else if(argc==2&&argv[1][0]>='1' &&argv[1][0]<='0'+LAST_DEMO&&!argv[1][1])
        first=last=argv[1][0]-'0';
    else if(argc!=1) {
        puts("Usage: SCREEN [1|2|3]; default runs all. Space next, Escape exits.");
        return 2;
    }
    _asm pushf
    _asm pop flags
    if(!(flags&0x200)) {
        puts("REFUSED: BIOS tick interrupts must be enabled.");return 3;
    }
    if(!DosSessionPermit()) {
        puts("REFUSED: needs plain DOS on a Tandy 1000 (not Windows).");return 3;
    }
    origMode=getMode();
    if(origMode!=3) {puts("REFUSED: start in 80-column DOS text mode 3.");return 3;}
    {
        union REGS r;
        r.h.ah=0x0f;int86(0x10,&r,&r);origPage=r.h.bh;
        if(r.h.ah!=80) {puts("REFUSED: 80-column text required.");return 3;}
        r.h.ah=3;r.h.bh=origPage;int86(0x10,&r,&r);
        origCursor=r.x.dx;origShape=r.x.cx;
        if(origPage>3) {puts("REFUSED: unsupported text page.");return 3;}
        readRawPages(&origCrt,&origCpu);
        if(origCrt!=origCpu||!(origCpu&1u)||origCpu>7u) {puts("REFUSED: no known 16K caller video window.");return 3;}
        _fmemcpy(origText,VRAM,sizeof origText);
    }
    if(atexit(cleanup)) {puts("REFUSED: cannot register cleanup.");return 3;}
    before61=(unsigned)inp(0x61)&0xcfu;
    oldBreak=_dos_getvect(0x23);_dos_setvect(0x23,stopBreak);hooked=1;
    for(d=first;d<=last&&!stopped&&!errorCode;d++) {
        nextDemo=0;
        if(d==1)demoCycle();
        else if(d==2) {if(first==last)demoFlip();else continue;}
        else if(d==3)demoSplit();
    }
    cleanup();
    if(qaMode) {
        union REGS r;
        for(i=0;i<8192;i++)if(((unsigned short far *)0xb8000000UL)[i]!=origText[i])textOK=0;
        r.h.ah=3;r.h.bh=origPage;int86(0x10,&r,&r);
        cursorOK=r.x.cx==origShape&&r.x.dx==origCursor;
        hookOK=_dos_getvect(0x23)==oldBreak;
        printf("SCREENQA text=%d cursor=%d hook=%d sound61=%d\n",textOK,cursorOK,hookOK,((unsigned)inp(0x61)&0xcfu)==before61);
        if(!textOK||!cursorOK||!hookOK||((unsigned)inp(0x61)&0xcfu)!=before61)errorCode=10;
    }
    if(errorCode==7)puts("REFUSED: extra buffers or changed BIOS window are not qualified.");
    if(qaMode)printf("DONE stop=%d error=%d frames=%u palette_loads=%u "
        "retrace_timeouts=%u mode_restored=%u\n",
        stopped,errorCode,frames,paletteLoads,retraceTimeouts,
        getMode()==origMode);
    return errorCode?4:0;
}
