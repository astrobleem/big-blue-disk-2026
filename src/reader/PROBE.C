#include <dos.h>
#include <direct.h>
int main(void){union REGS r;chdir("\\");r.x.ax=7;int86(0x10,&r,&r);return 7;}
