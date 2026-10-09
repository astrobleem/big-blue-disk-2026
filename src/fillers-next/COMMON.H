#ifndef COMMON_H
#define COMMON_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MONEYMAX 100000000L
/* Strict unsigned decimal, optional plus, no minus or trailing junk. */
static int number(const char *s, long max, long *out)
{
    long n=0; int digits=0;
    if (*s=='+') ++s;
    while (*s>='0' && *s<='9') {
        int d=*s++-'0';
        if(n>(max-(long)d)/10L || (long)d>max) return 0;
        n=n*10L+(long)d; digits=1;
    }
    if(!digits || *s) return 0;
    *out=n; return 1;
}
static int ask(const char *label, long max, long *n)
{
    char s[64]; int c;
    printf("%s (0 cancels): ",label);
    if(!fgets(s,sizeof(s),stdin)) return -1;
    if(!strchr(s,'\n')) {
        while((c=getchar())!='\n' && c!=EOF) ;
        puts("Input too long."); return 0;
    }
    s[strcspn(s,"\r\n")]=0;
    if(!number(s,max,n)) {puts("Enter a whole positive number.");return 0;}
    return 1;
}
static unsigned rngstate=1;
static unsigned roll(unsigned n)
{
    rngstate=(unsigned)((rngstate*25173UL+13849UL)&65535UL);
    return (unsigned)((rngstate*(unsigned long)n)>>16);
}
#ifdef HOST
#define clear() ((void)0)
#else
#include <conio.h>
#include <dos.h>
static void clear(void) { union REGS r; r.h.ah=0; r.h.al=3; int86(0x10,&r,&r); }
#endif
#endif
