#include <string.h>
#include <ctype.h>
#include "CORE.H"
static int token(const char *s,int n)
{
    int i,len=(int)strlen(s);
    if(!len||len>n)return 0;
    for(i=0;i<len;i++)if(!(s[i]>='A'&&s[i]<='Z')&&
        !(s[i]>='0'&&s[i]<='9')&&s[i]!='_'&&s[i]!='-')return 0;
    return 1;
}
static int component(char *s)
{
    char *dot=strchr(s,'.');int len;
    if(dot) {
        *dot=0;
        if(!token(dot+1,3))return 0;
    }
    if(!token(s,8))return 0;
    len=(int)strlen(s);
    if(!strcmp(s,"CON")||!strcmp(s,"PRN")||!strcmp(s,"AUX")||
       !strcmp(s,"NUL")||(len==4&&(!strncmp(s,"COM",3)||
       !strncmp(s,"LPT",3))&&s[3]>='1'&&s[3]<='9'))return 0;
    return 1;
}
int safe_path(const char *s)
{
    char b[64],*p,*slash;int len=(int)strlen(s);
    if(!len||len>63)return 0;
    strcpy(b,s);p=b;
    while(1) {
        slash=strchr(p,'\\');
        if(slash)*slash=0;
        if(!component(p))return 0;
        if(!slash)break;
        p=slash+1;
    }
    return 1;
}
int manifest(FILE *f,Item out[ITEMS])
{
    char b[192],*part[5],*p;int n=0,i,j,c;
    static const char *ids[ITEMS]={"ALFREDO","KROZ","HORSES","DRUGWARS","CAMERA","TANDSND"};
    static const char *kinds[ITEMS]={"SHOW","GAME","GAME","GAME","ARTICLE","ARTICLE"};
    while(fgets(b,sizeof(b),f)) {
        if(!strchr(b,'\n')&&!feof(f))return 0;
        b[strcspn(b,"\r\n")]=0;
        if(!b[0]||b[0]==';')continue;
        if(n>=ITEMS)return 0;
        p=b;part[0]=p;
        for(i=1;i<5;i++) {
            p=strchr(p,'|');if(!p)return 0;*p++=0;part[i]=p;
        }
        if(strchr(part[4],'|')||part[4][0])return 0;
        if(strcmp(part[0],ids[n])||strcmp(part[1],kinds[n]))return 0;
        if(!part[2][0]||strlen(part[2])>40||!safe_path(part[3]))return 0;
        for(j=0;part[2][j];j++){c=(unsigned char)part[2][j];if(c<32||c==127)return 0;}
        if(strcmp(part[3]+strlen(part[3])-((strlen(part[3])>=4)?4:0),
           n<4?".EXE":".TXT"))return 0;
        strcpy(out[n].id,part[0]);strcpy(out[n].kind,part[1]);
        strcpy(out[n].label,part[2]);strcpy(out[n].path,part[3]);n++;
    }
    return n==ITEMS&&!ferror(f);
}
/* Stream fixed-width pages; preserve CP437 bytes, CRLF and blank lines. */
int read_page(FILE *f,long start,char lines[ROWS][COLS+1],long *next)
{
    int row,col,c,peek,any=0;
    memset(lines,0,ROWS*(COLS+1));*next=-1;
    if(fseek(f,start,SEEK_SET))return -1;
    for(row=0;row<ROWS;row++) {
        col=0;c=fgetc(f);if(c==EOF)break;any=1;
        while(c!=EOF&&c!='\r'&&c!='\n') {
            lines[row][col++]=(char)(c=='\t'?' ':c);
            if(col==COLS)break;
            c=fgetc(f);
        }
        if(col==COLS) {
            peek=fgetc(f);
            if(peek=='\r'){c=peek;}
            else if(peek=='\n'){c=peek;}
            else {if(peek!=EOF)ungetc(peek,f);c=EOF;}
        }
        if(c=='\r'){peek=fgetc(f);if(peek!='\n'&&peek!=EOF)ungetc(peek,f);}
        if(ferror(f))return -1;
    }
    peek=fgetc(f);
    if(peek!=EOF){ungetc(peek,f);*next=ftell(f);}
    return any?row:0;
}
void pager_init(Pager *p,FILE *f)
{
    memset(p,0,sizeof(*p));p->f=f;p->count=1;p->more=1;
}
int pager_load(Pager *p,char lines[ROWS][COLS+1])
{
    long next;int rows=read_page(p->f,p->offsets[p->current],lines,&next);
    if(rows<0)return -1;
    if(p->current==p->count-1) {
        if(next>=0&&p->count<PAGES) {
            p->offsets[p->count++]=next;p->more=1;
        } else p->more=(next>=0?2:0);
    }
    return rows;
}
/* 0 cancel, 1 previous, 2 next, 3 home. End is a UI-assisted scan. */
int pager_action(Pager *p,int key)
{
    if(key==0)return -1;
    if(key==1&&p->current>0)p->current--;
    if(key==2&&p->current<p->count-1)p->current++;
    if(key==3)p->current=0;
    return p->current;
}

