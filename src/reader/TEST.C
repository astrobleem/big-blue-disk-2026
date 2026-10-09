#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CORE.H"
static int checks;
#define C(x) do{checks++;if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static const char *valid="ALFREDO|SHOW|Alfredo|ALFREDO\\ALFREDO.EXE|\nKROZ|GAME|Kroz|KROZ\\ROOM.EXE|\nHORSES|GAME|Horse racing|FILLERS\\HORSE.EXE|\nDRUGWARS|GAME|Drug Wars|FILLERS\\DRUGWAR.EXE|\nCAMERA|ARTICLE|Camera obscura|ARTICLES\\CAMERA.TXT|\nTANDSND|ARTICLE|Tandy sound|ARTICLES\\TANDSND.TXT|\n";
static int parse(const char *s){FILE *f=tmpfile();Item a[ITEMS];int n;C(f!=0);fputs(s,f);rewind(f);n=manifest(f,a);fclose(f);return n;}
int main(void)
{
    FILE *f;Pager p;char lines[ROWS][COLS+1],buf[2048];long next;int i,n;
    C(safe_path("ARTICLES\\CAMERA.TXT"));C(safe_path("KROZ\\ROOM.EXE"));
    C(!safe_path("..\\ROOM.EXE"));C(!safe_path("C:\\ROOM.EXE"));
    C(!safe_path("\\ROOM.EXE"));C(!safe_path("ROOM.EXE\\"));
    C(!safe_path("CON.TXT"));C(!safe_path("DIR\\NUL"));
    C(!safe_path("COM1.EXE"));C(!safe_path("LPT9.TXT"));
    C(!safe_path("TOOLONGNM.EXE"));C(!safe_path("DIR/ROOM.EXE"));
    C(!safe_path("ROOM.EXE&"));C(!safe_path("A..B"));
    C(parse(valid));strcpy(buf,valid);strcat(buf,"EXTRA|GAME|Extra|X.EXE|\n");C(!parse(buf));
    strcpy(buf,valid);buf[0]='X';C(!parse(buf));
    strcpy(buf,valid);strcat(buf,"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");C(!parse(buf));
    C(!parse("ALFREDO|SHOW|x|A.EXE|argument\n"));
    f=tmpfile();C(f!=0);for(i=0;i<41;i++)fprintf(f,"Line %d\r\n",i);
    pager_init(&p,f);C(pager_load(&p,lines)==20);
    C(!strcmp(lines[0],"Line 0"));C(!strcmp(lines[19],"Line 19"));
    C(pager_action(&p,2)==1);C(pager_load(&p,lines)==20);
    C(!strcmp(lines[0],"Line 20"));C(pager_action(&p,2)==2);
    C(pager_load(&p,lines)==1);C(!strcmp(lines[0],"Line 40"));
    C(pager_action(&p,2)==2);C(pager_action(&p,1)==1);
    C(pager_action(&p,3)==0);C(pager_action(&p,0)==-1);C(p.current==0);
    fclose(f);f=tmpfile();C(f!=0);
    for(i=0;i<76;i++)fputc('x',f);fputs("\r\n\r\n",f);fputc(219,f);fputs("\n",f);
    C(read_page(f,0,lines,&next)==3);C(strlen(lines[0])==76);
    C(!lines[1][0]);C((unsigned char)lines[2][0]==219);C(next==-1);
    fclose(f);f=tmpfile();C(f!=0);for(i=0;i<200;i++)fputc('z',f);
    C(read_page(f,0,lines,&next)==3);C(strlen(lines[2])==48);fclose(f);
    f=tmpfile();C(f!=0);pager_init(&p,f);C(pager_load(&p,lines)==0);fclose(f);
    f=tmpfile();C(f!=0);for(i=0;i<ROWS*(PAGES+1);i++)fputs("x\n",f);
    pager_init(&p,f);n=0;do{C(pager_load(&p,lines)==20);n++;if(p.current==PAGES-1)break;pager_action(&p,2);}while(1);
    C(n==PAGES&&p.count==PAGES&&p.more==2);fclose(f);
    printf("PASS %d checks\n",checks);return 0;
}

