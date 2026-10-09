/* Original teaching demo. MSC6 /G0 /AS; plain DOS, cooperative adapter. */
#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <conio.h>
#include "DOSSND.H"
#include "DEMOCORE.H"
static volatile int stopped;
static int owned, hooked, errorCode;
static void (_interrupt _far *oldBreak)(void);
static unsigned writes, passes;
static void _interrupt _far stopBreak(void) { stopped=1; }
static unsigned long ticks(void)
{
    unsigned long value;
    unsigned flags;
    _asm pushf
    _asm pop flags
    _asm cli
    value=*(unsigned long _far *)0x0040006cUL;
    _asm push flags
    _asm popf
    return value;
}
static void cleanup(void)
{
    if(owned) { DosSoundRelease(); owned=0; }
    if(hooked) { _dos_setvect(0x23,oldBreak); hooked=0; }
}
static void byteOut(unsigned b)
{
    int r;
    if(errorCode)return;
    r=DosSoundPsgByte(b);
    if(r)errorCode=r;else writes++;
}
static void tone(unsigned ch,unsigned divider,unsigned attenuation)
{
    byteOut(0x80|(ch<<5)|(divider&15));
    byteOut(divider>>4);
    byteOut(0x90|(ch<<5)|attenuation);
}
static void silence(void)
{
    unsigned i;
    for(i=0;i<4;i++)byteOut(0x9f|(i<<5));
    if(!errorCode)errorCode=DosSoundPitNote(0,0);
}
static void step(unsigned demo,unsigned age)
{
    unsigned a,period;
    int r;
    if(demo==1) {
        if(!age) {
            tone(0,427,11);tone(1,339,11);tone(2,285,11);
        }
        if(!(age&7)) {
            r=DosSoundPitNote(melody[(age>>3)&3],1);
            if(r)errorCode=r;
        }
    } else if(demo==2) {
        a=percussion(age&7);
        if(!(age&7))byteOut((age&8)?0xe4:0xe2);
        byteOut(0xf0|a);
    } else {
        a=articulation(age);period=leadPeriod(age);
        if(!age)tone(0,period,a);
        else {
            /* Pitch changes only after vibrato starts; volume never attacks. */
            if(age>=8&&a<15) {
                byteOut(0x80|(period&15));byteOut(period>>4);
            }
            byteOut(0x90|a);
        }
    }
    passes++;
}
int main(int argc,char **argv)
{
    unsigned first=1,last=3,d,age,before,after,key;
    unsigned long start,now;
    int r;
    if(argc==2&&argv[1][0]>='1'&&argv[1][0]<='3'&&!argv[1][1])
        first=last=argv[1][0]-'0';
    else if(argc!=1) {
        puts("Usage: VOICE [1|2|3]; default runs all. Escape stops.");
        return 2;
    }
    puts("1 Extra PIT melody  2 Noise percussion  3 Envelope/vibrato");
    puts("Plain DOS, known-compatible Tandy only. Escape/Ctrl-C stops.");
    _asm pushf
    _asm pop before
    if(!(before&0x200)) {
        puts("REFUSED: BIOS tick interrupts must be enabled.");return 3;
    }
    atexit(cleanup);
    r=DosSoundAcquire(DS_PSG|DS_PIT);
    if(r) { printf("REFUSED acquire=%d; no demo writes\n",r);return 3; }
    owned=1;before=inp(0x61);
    oldBreak=_dos_getvect(0x23);_dos_setvect(0x23,stopBreak);hooked=1;
    silence();
    for(d=first;d<=last&&!stopped&&!errorCode;d++) {
        start=ticks();age=0;
        while(age<32&&!stopped&&!errorCode) {
            step(d,age);
            do {
                if(kbhit()) {
                    key=getch();if(key==27||key==3)stopped=1;
                }
                now=ticks();
                if(now==start+age&&!stopped) {
                    /* Ordinary BIOS interrupts wake an otherwise idle CPU. */
                    _asm hlt
                }
            } while(now==start+age&&!stopped);
            /* BIOS midnight wrap ends safely; no IRQ or timer0 changes. */
            if(now<start) { stopped=1;break; }
            age=(unsigned)(now-start);
        }
        silence();
    }
    cleanup();after=inp(0x61);
    printf("DONE stop=%d error=%d passes=%u psg_writes=%u\n",
        stopped,errorCode,passes,writes);
    printf("speaker_off=%u control61_preserved=%u\n",
        !(after&3),!((before^after)&0xcc));
    return errorCode?4:0;
}
