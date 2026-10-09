#include <dos.h>
#include <conio.h>
#include <direct.h>
#include <process.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "CORE.H"
static Item items[ITEMS];
static Pager pager;
static char lines[ROWS][COLS+1];
static char root[128],caller[128];
static unsigned short saved_mode,saved_cursor;
static unsigned short far *video=(unsigned short far *)0xb8000000L;
static void put(int x,int y,const char *s,int attr)
{
    while(*s&&x<80)video[y*80+x++]=(unsigned short)
        ((attr<<8)|(unsigned char)*s++);
}
static void mode3(void)
{
    union REGS r;r.x.ax=3;int86(0x10,&r,&r);
    r.h.ah=1;r.x.cx=0x2000;int86(0x10,&r,&r);
}
static void clear(void)
{
    int i;for(i=0;i<2000;i++)video[i]=0x0720;
}
static int setdir(const char *path)
{
    unsigned drives;
    if(path[0]&&path[1]==':')_dos_setdrive(
        (unsigned)(toupper(path[0])-'A'+1),&drives);
    return chdir(path)==0;
}
/* DOS3+ executable pathname in the PSP environment trailer. */
static int executable_root(void)
{
    union REGS r;unsigned short far *psp;unsigned seg;
    char far *env;int i,j;char exe[128],*slash;
    r.h.ah=0x30;int86(0x21,&r,&r);if(r.h.al<3)return 0;
    r.h.ah=0x62;int86(0x21,&r,&r);
    psp=(unsigned short far *)((unsigned long)r.x.bx<<16);
    seg=psp[0x16];if(!seg)return 0;
    env=(char far *)((unsigned long)seg<<16);
    for(i=0;i<32760;i++)if(!env[i]&&!env[i+1])break;
    if(i==32760||!(unsigned char)env[i+2])return 0;
    i+=4;
    for(j=0;j<127&&env[i+j];j++)exe[j]=env[i+j];
    if(j==127||j==0)return 0;exe[j]=0;
    if(!_fullpath(root,exe,sizeof(root)))return 0;
    slash=strrchr(root,'\\');if(!slash)return 0;
    if(slash==root+2)slash[1]=0;else *slash=0;
    return 1;
}
static int absolute(const char *rel,char path[128])
{
    unsigned n=(unsigned)strlen(root);
    if(!safe_path(rel)||n+strlen(rel)+2>=128)return 0;
    strcpy(path,root);if(n&&path[n-1]!='\\')strcat(path,"\\");
    strcat(path,rel);return 1;
}
static void message(const char *s)
{
    clear();put(2,3,s,15);put(2,5,"Press any key to return to the six-item menu.",7);
    getch();
}
static int launch_arg(const char *rel,const char *arg)
{
    char path[128],dir[128],*slash;int result;
    FILE *f;
    if(!absolute(rel,path))return -2;
    strcpy(dir,path);slash=strrchr(dir,'\\');if(!slash)return -2;
    if(slash==dir+2)slash[1]=0;else *slash=0;
    if(!setdir(dir)){setdir(root);return -4;}
    f=fopen(path,"rb");
    if(!f){setdir(root);mode3();return -3;}
    fclose(f);
    if(arg)result=spawnl(P_WAIT,path,path,arg,(char *)0);
    else result=spawnl(P_WAIT,path,path,(char *)0);
    setdir(root);mode3();return result;
}
static int launch(const char *rel){return launch_arg(rel,(const char *)0);}
static int diagram(int automated)
{
    char path[128];unsigned char buffer[512];FILE *f;
    unsigned n,i,at=0;union REGS r;int ok=1;
    if(!absolute("ARTICLES\\CAMERA.CGA",path))return 0;
    f=fopen(path,"rb");if(!f)return 0;
    fseek(f,0,SEEK_END);if(ftell(f)!=16384L){fclose(f);return 0;}
    rewind(f);r.x.ax=6;int86(0x10,&r,&r);
    while((n=(unsigned)fread(buffer,1,512,f))!=0) {
        for(i=0;i<n;i++)((unsigned char far *)video)[at++]=buffer[i];
    }
    if(ferror(f)||at!=16384U)ok=0;fclose(f);
    r.h.ah=15;int86(0x10,&r,&r);if(r.h.al!=6)ok=0;
    if(automated) {
        if(absolute("DIAG.BIN",path)) {
            f=fopen(path,"wb");if(!f)ok=0;
            else {for(at=0;at<16384U;at+=512) {
                for(i=0;i<512;i++)buffer[i]=((unsigned char far *)video)[at+i];
                if(fwrite(buffer,1,512,f)!=512)ok=0;
            }fclose(f);}
        }
    } else getch();
    mode3();return ok;
}
static void menu(int selected)
{
    int i,x,y,attr;char number[4];
    for(i=0;i<2000;i++)video[i]=0x1720;
    put(2,1,"BIG BLUE DISK 2026 / WORKING ISSUE",0x1f);
    put(2,3,"Six selected features",0x1e);
    for(x=2;x<78;x++)video[4*80+x]=video[20*80+x]=0x17cd;
    for(y=5;y<20;y++)video[y*80+1]=video[y*80+78]=0x17ba;
    video[4*80+1]=0x17c9;video[4*80+78]=0x17bb;
    video[20*80+1]=0x17c8;video[20*80+78]=0x17bc;
    for(i=0;i<ITEMS;i++) {
        attr=i==selected?0x71:0x1f;
        if(i==selected)for(x=2;x<78;x++)video[(6+i*2)*80+x]=0x7120;
        sprintf(number,"%d.",i+1);put(4,6+i*2,number,attr);
        put(7,6+i*2,items[i].label,attr);
        put(55,6+i*2,items[i].kind,i==selected?0x71:0x1b);
        if(i==selected)put(2,6+i*2,">",0x71);
    }
    put(2,22,"Arrows or 1-6 select. ENTER opens. ESC exits.",0x1e);
    put(2,24,"Prototype assembly: missing items are reported without leaving the menu.",0x1b);
}
static int menu_colors_ok(int selected)
{
    int i,x,y,want;
    menu(selected);
    for(i=0;i<2000;i++) {
        x=i%80;y=i/80;
        want=(y==6+selected*2&&x>=2&&x<78)?7:1;
        if((int)((video[i]>>12)&7)!=want)return 0;
    }
    return 1;
}
static int key(void)
{
    int k=getch();if(k==0||k==224)return 256+getch();return k;
}
static void article(const Item *item)
{
    FILE *f;char path[128],status[78];int k,i,rows,oldpage;
    if(!absolute(item->path,path)){message("Invalid article path.");return;}
    f=fopen(path,"rb");if(!f){message("This item is not installed yet.");return;}
    pager_init(&pager,f);
    while(1) {
        rows=pager_load(&pager,lines);
        if(rows<0){fclose(f);message("The article could not be read.");return;}
        clear();put(2,0,item->label,15);
        if(!rows)put(2,3,"This article is empty.",7);
        for(i=0;i<rows;i++)put(2,3+i,lines[i],7);
        sprintf(status,"Page %d | PgUp/PgDn or SPACE | Home/End | ESC returns",
            pager.current+1);put(2,24,status,11);
        if(!strcmp(item->id,"CAMERA"))put(2,23,"D shows the camera diagram; any key returns to this page.",14);
        if(pager.more==2)put(2,23,"Reader limit: 256 pages. Remaining text is not displayed.",14);
        k=key();if(k==27)break;
        if(!strcmp(item->id,"CAMERA")&&(k=='d'||k=='D')) {
            if(!diagram(0))message("The camera diagram is not installed or could not be read.");
        }
        if(k==256+73)pager_action(&pager,1);
        if(k==256+81||k==' '||k==13)pager_action(&pager,2);
        if(k==256+71)pager_action(&pager,3);
        if(k==256+79) {
            do {oldpage=pager.current;pager_action(&pager,2);
                if(pager.current==oldpage)break;
                if(pager_load(&pager,lines)<0)break;
            }while(1);
        }
    }
    fclose(f);
}
static void snapshot(const char *path)
{
    FILE *f;int i;unsigned short cell;
    f=fopen(path,"wb");if(!f)return;
    for(i=0;i<2000;i++){cell=video[i];fwrite(&cell,2,1,f);}fclose(f);
}
static int qa(void)
{
    FILE *f;union REGS r;char cwd[128];int result,i,failed;
    menu(0);snapshot("MENU.BIN");
    f=fopen("ARTICLES\\CAMERA.TXT","rb");if(!f)return 1;
    pager_init(&pager,f);if(pager_load(&pager,lines)<=0)return 2;
    clear();put(2,0,"TEMPORARY CAMERA PAGING TEST - NO CAMERA CONTENT",14);
    {int i;for(i=0;i<ROWS;i++)put(2,3+i,lines[i],7);}
    snapshot("ARTICLE.BIN");fclose(f);
    for(i=0;i<ITEMS;i++)if(!menu_colors_ok(i))return 6;
    for(i=0;i<3;i++) {
        result=launch("CHILD\\PROBE.EXE");
        getcwd(cwd,sizeof(cwd));r.h.ah=15;int86(0x10,&r,&r);
        if(result!=7||strcmp(cwd,root)||r.h.al!=3)return 4;
    }
    failed=launch("CHILD\\MISSING.EXE");
    getcwd(cwd,sizeof(cwd));r.h.ah=15;int86(0x10,&r,&r);
    f=fopen("RETURN.LOG","w");if(!f)return 3;
    fprintf(f,"repeated=3 child=%d missing=%d cwd=%s root=%s mode=%d\n",
        result,failed,cwd,root,r.h.al);
    fclose(f);menu(0);snapshot("RETURN.BIN");
    if(result!=7||failed!=-3||strcmp(cwd,root)||r.h.al!=3)return 4;
    return 0;
}
static int suite(int articles_only)
{
    FILE *f,*log;char path[128],cwd[128];int i,result,failed=0,pages;
    union REGS r;
    static const char *args[4]={"/QA","/qa","/demo","/demo"};
    log=fopen("INTEG.LOG","w");if(!log)return 1;
    for(i=0;i<ITEMS;i++) {
        result=menu_colors_ok(i);
        fprintf(log,"BLUE_MENU selection=%d attributes=%s\n",i+1,result?"PASS":"FAIL");
        if(!result)failed=1;
    }
    for(i=articles_only?4:0;i<4;i++) {
        result=launch_arg(items[i].path,args[i]);
        getcwd(cwd,sizeof(cwd));r.h.ah=15;int86(0x10,&r,&r);
        fprintf(log,"%s exit=%d cwd=%s root=%s mode=%d\n",
            items[i].id,result,cwd,root,r.h.al);fflush(log);
        if(result||strcmp(cwd,root)||r.h.al!=3)failed=1;
    }
    for(i=4;i<6;i++) {
        if(!absolute(items[i].path,path)){failed=1;continue;}
        f=fopen(path,"rb");if(!f){failed=1;continue;}
        pager_init(&pager,f);pages=0;
        do {
            if(pager_load(&pager,lines)<=0){failed=1;break;}
            pages++;
            if(pager.current==pager.count-1)break;
            pager_action(&pager,2);
        }while(pages<PAGES);
        fprintf(log,"%s pages=%d limit=%d cancel=%d\n",items[i].id,
            pages,pager.more,pager_action(&pager,0));
        fclose(f);if(pager.more==2)failed=1;
    }
    result=diagram(1);fprintf(log,"CAMERA diagram=%d mode=6 readback=DIAG.BIN\n",result);
    if(!result)failed=1;
    menu(0);snapshot("MENU.BIN");
    fprintf(log,"RESULT %s\n",failed?"FAIL":"PASS");fclose(log);return failed;
}
int main(int argc,char **argv)
{
    FILE *f;union REGS r;int selected=0,k,result=0,done=0;
    char exitpath[128],cwd[128];
    if(!getcwd(caller,sizeof(caller))||!executable_root()) {
        puts("Reader needs DOS3+ and a readable executable path.");return 1;
    }
    r.h.ah=15;int86(0x10,&r,&r);saved_mode=r.h.al;
    r.h.ah=3;r.h.bh=0;int86(0x10,&r,&r);saved_cursor=r.x.cx;
    if(!setdir(root)){puts("Cannot open the issue folder.");return 1;}
    f=fopen("ISSUE.DAT","rb");
    if(!f||!manifest(f,items)){if(f)fclose(f);setdir(caller);
        puts("ISSUE.DAT is invalid or missing. Exactly six approved items required.");return 1;}
    fclose(f);mode3();
    if(argc>1&&!strcmp(argv[1],"/qa"))result=qa();
    else if(argc>1&&!strcmp(argv[1],"/suite"))result=suite(0);
    else if(argc>1&&!strcmp(argv[1],"/readqa"))result=suite(1);
    else while(!done) {
        menu(selected);k=key();
        if(k==27){done=1;continue;}
        if(k==256+72&&selected>0)selected--;
        else if(k==256+80&&selected<ITEMS-1)selected++;
        else if(k>='1'&&k<='6')selected=k-'1';
        else if(k==13) {
            if(!strcmp(items[selected].kind,"ARTICLE"))article(&items[selected]);
            else {
                result=launch(items[selected].path);
                if(result==-3)message("This item is not installed yet.");
                else if(result<0)message("The program could not be launched.");
                else if(result>0)message("The program returned a nonzero status.");
                result=0;
            }
        }
    }
    setdir(caller);
    if(argc>1&&(!strcmp(argv[1],"/qa")||!strcmp(argv[1],"/suite")||!strcmp(argv[1],"/readqa"))) {
        getcwd(cwd,sizeof(cwd));
        if(absolute("EXIT.LOG",exitpath)) {
            f=fopen(exitpath,"w");
            if(f){fprintf(f,"caller=%s restored=%s\n",caller,cwd);fclose(f);}
        }
        if(strcmp(cwd,caller))result=5;
    }
    r.x.ax=saved_mode;int86(0x10,&r,&r);
    r.h.ah=1;r.x.cx=saved_cursor;int86(0x10,&r,&r);
    return result;
}

