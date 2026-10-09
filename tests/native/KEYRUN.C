/* Owned guest fixture: seed one BIOS key, spawn safe candidate, verify return. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <dos.h>
#include <conio.h>
static unsigned short image[2000];
int main(int argc,char **argv) {
 union REGS r;unsigned beforeMode,beforePage,beforePos,beforeShape,before61,after61,flags,i;int rc,text=1;
 unsigned short far *head=(unsigned short far *)0x0040001aUL;
 unsigned short far *tail=(unsigned short far *)0x0040001cUL;
 unsigned short far *buf;unsigned next,key;
 void (_interrupt _far *v8)(void);
 void (_interrupt _far *v23)(void);
 if(argc!=4)return 2;
 r.h.ah=15;int86(0x10,&r,&r);beforeMode=r.h.al;beforePage=r.h.bh;
 r.h.ah=3;r.h.bh=(unsigned char)beforePage;int86(0x10,&r,&r);beforePos=r.x.dx;beforeShape=r.x.cx;
 _fmemcpy(image,(void far *)(0xb8000000UL+beforePage*4096u),sizeof image);
 before61=(unsigned)inp(0x61)&0xcfu;v8=_dos_getvect(8);v23=_dos_getvect(0x23);
 key=argv[3][0]=='c'?3:27;
 _asm pushf
 _asm pop flags
 _asm cli
 next=*tail+2;if(next>=0x3e)next=0x1e;
 if(next==*head) {
 _asm push flags
 _asm popf
 return 3;
 }
 buf=(unsigned short far *)(0x00400000UL+*tail);*buf=(unsigned short)(key|(1u<<8));*tail=(unsigned short)next;
 _asm push flags
 _asm popf
 if(argv[2][0]=='-')rc=spawnl(P_WAIT,argv[1],argv[1],NULL);
 else rc=spawnl(P_WAIT,argv[1],argv[1],argv[2],NULL);
 after61=(unsigned)inp(0x61)&0xcfu;
 for(i=0;i<2000;i++)if(((unsigned short far *)(0xb8000000UL+beforePage*4096u))[i]!=image[i])text=0;
 r.h.ah=15;int86(0x10,&r,&r);i=(unsigned)((unsigned)r.h.al==beforeMode&&(unsigned)r.h.bh==beforePage);
 r.h.ah=3;r.h.bh=(unsigned char)beforePage;int86(0x10,&r,&r);
 printf("KEYRETURN rc=%d text=%d modepage=%u cursor=%d shape=%d sound61=%d timerhook=%d breakhook=%d\n",rc,text,i,r.x.dx==beforePos,r.x.cx==beforeShape,before61==after61,_dos_getvect(8)==v8,_dos_getvect(0x23)==v23);
 return !(rc==0&&text&&i&&r.x.dx==beforePos&&r.x.cx==beforeShape&&before61==after61&&_dos_getvect(8)==v8&&_dos_getvect(0x23)==v23);
}
