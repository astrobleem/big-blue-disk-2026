/* Failure injection tests shared guard code; no Windows session is launched. */
#include <stdio.h>
#include <dos.h>
static int enhanced,standard;
static int probe86(int n,union REGS *a,union REGS *b) { (void)n;if(a->x.ax==0x1600)b->h.al=(unsigned char)enhanced;else if(a->x.ax==0x4680)b->x.ax=standard?0:0x4680;return 0; }
#define int86 probe86
#include "DOSGUARD.H"
int main(void) {int ok=1;enhanced=3;standard=0;ok&=!DosSessionPermit();enhanced=0;standard=1;ok&=!DosSessionPermit();enhanced=128;standard=1;ok&=!DosSessionPermit();enhanced=0;standard=0;ok&=DosSessionPermit();printf("GUARDQA %s enhanced/standard/refusal/plain-marker\n",ok?"PASS":"FAIL");return !ok;}
