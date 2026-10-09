/* Flow-88: original design/draft Gemini; MSC6 port Claude; safety corrections
 * Codex, edited/submitted by Chad. GPLv3. Separate experimental contribution.
 * No audio port writes: PIT/PSG state cannot be completely read/restored.
 * Plain DOS, Tandy, current 80x25 mode 3 only. No timer or IRQ changes.
 */
#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <conio.h>
#include <time.h>
#include <string.h>
#include "DOSGUARD.H"
#include "FLOWCORE.H"
static volatile int stopped;
static int hooked,saved;
static unsigned pageOffset,oldCursor,oldShape;
static unsigned char oldPage;
static unsigned short oldScreen[2000];
static void (_interrupt _far *oldBreak)(void);
static void _interrupt _far stopBreak(void) { stopped=1; }
static void cursor(unsigned shape) { union REGS r;r.h.ah=1;r.x.cx=shape;int86(0x10,&r,&r); }
static void cleanup(void) {
    union REGS r;
    if(saved) {
        _fmemcpy((void far *)(0xb8000000UL+pageOffset),oldScreen,sizeof oldScreen);
        cursor(oldShape);r.h.ah=2;r.h.bh=oldPage;r.x.dx=oldCursor;int86(0x10,&r,&r);
        saved=0;
    }
    if(hooked) {_dos_setvect(0x23,oldBreak);hooked=0;}
}
static int begin(void) {
    union REGS r;
    if(!DosSessionPermit())return 0;
    r.h.ah=15;int86(0x10,&r,&r);
    if((r.h.al&127)!=3||r.h.ah!=80||r.h.bh>3)return 0;
    oldPage=r.h.bh;pageOffset=(unsigned)oldPage*4096u;
    r.h.ah=3;r.h.bh=oldPage;int86(0x10,&r,&r);oldCursor=r.x.dx;oldShape=r.x.cx;
    _fmemcpy(oldScreen,(void far *)(0xb8000000UL+pageOffset),sizeof oldScreen);saved=1;
    if(atexit(cleanup)) {cleanup();return 0;}
    oldBreak=_dos_getvect(0x23);_dos_setvect(0x23,stopBreak);hooked=1;
    cursor(0x2000);return 1;
}
/* ----------------------------------------------------------------------------
 * DIRECT VIDEO RAM (0xB800) DRAWING ROUTINES
 * ---------------------------------------------------------------------------- */

/* Fast cell write directly to Text VRAM using inline assembly */
void vram_put_char(int x, int y, char ch, unsigned char attr) {
    unsigned int far *cell = (unsigned int far *)(((unsigned long)VRAM_SEGMENT << 16) + pageOffset + (unsigned)((y * 80 + x) * 2));
    *cell = ((unsigned)attr << 8) | (unsigned char)ch;
}

/* Draw a screen-aligned string into VRAM */
void vram_print_str(int x, int y, const char *str, unsigned char attr) {
    int i = 0;
    while (str[i] != '\0') {
        vram_put_char(x + i, y, str[i], attr);
        i++;
    }
}

/* Draw the viewport border and interactive status headers */
void draw_ui_frame(void) {
    int x, y;

    /* Top Status Bar Background */
    for (x = 0; x < 80; x++) {
        vram_put_char(x, 0, ' ', 0x70);
        vram_put_char(x, 24, ' ', 0x70);
    }

    /* Single-line border around $78 \times 23$ fluid viewport */
    vram_put_char(VIEW_X - 1, VIEW_Y - 1, (char)0xDA, 0x07); /* Top-left corner */
    vram_put_char(VIEW_X + GRID_W, VIEW_Y - 1, (char)0xBF, 0x07); /* Top-right corner */
    vram_put_char(VIEW_X - 1, VIEW_Y + GRID_H, (char)0xC0, 0x07); /* Bottom-left corner */
    vram_put_char(VIEW_X + GRID_W, VIEW_Y + GRID_H, (char)0xD9, 0x07); /* Bottom-right corner */

    for (x = VIEW_X; x < VIEW_X + GRID_W; x++) {
        vram_put_char(x, VIEW_Y - 1, (char)0xC4, 0x07);
        vram_put_char(x, VIEW_Y + GRID_H, (char)0xC4, 0x07);
    }
    for (y = VIEW_Y; y < VIEW_Y + GRID_H; y++) {
        vram_put_char(VIEW_X - 1, y, (char)0xB3, 0x07);
        vram_put_char(VIEW_X + GRID_W, y, (char)0xB3, 0x07);
    }

    /* Bottom Hotkey Instructions */
    vram_print_str(1, 24, "[ARROWS] Move [SPACE] Inject [B] Obstacle [V] Vectors [C] Clear [Q] Quit", 0x70);
}

/* ----------------------------------------------------------------------------
 * RENDERING & DISPLAY SOLVER
 * ---------------------------------------------------------------------------- */

void render_viewport(void) {
    int x, y;
    char draw_ch;
    unsigned char draw_attr;
    int ramp_idx;
    int vel_u, vel_v;

    for (x = 0; x < GRID_W; x++) {
        for (y = 0; y < GRID_H; y++) {
            if (obstacle[x][y]) {
                draw_ch = 0xDB; /* Solid ASCII block for airfoils/barriers */
                draw_attr = 0x08; /* Dark Gray */
            } else if (show_velocity) {
                vel_u = u[x][y];
                vel_v = v[x][y];
                
                if (fix_abs((Fix)vel_u) > fix_abs((Fix)vel_v)) {
                    draw_ch = (char)((vel_u > 0) ? '>' : '<');
                } else if (fix_abs((Fix)vel_v) > 0) {
                    draw_ch = (char)((vel_v > 0) ? 'v' : '^');
                } else {
                    draw_ch = ' ';
                }
                draw_attr = 0x0A; /* Light Green vectors */
            } else {
                /* Render fluid density levels */
                ramp_idx = density_slot(dens[x][y]);
                if (ramp_idx < 0) ramp_idx = 0;
                if (ramp_idx > 6) ramp_idx = 6;

                draw_ch = density_chars[ramp_idx];
                draw_attr = density_attrs[ramp_idx];
            }

            /* Draw cursor overlay */
            if (x == cursor_x && y == cursor_y) {
                draw_attr = 0xCF; /* Blinking Bright White on Red */
            }

            vram_put_char(VIEW_X + x, VIEW_Y + y, draw_ch, draw_attr);
        }
    }
}


static void applyKey(int ch)
{
    if(ch==27||ch==3||ch=='q'||ch=='Q')stopped=1;
    else if(ch==' '||ch==13)inject_fluid=!inject_fluid;
    else if(ch=='b'||ch=='B')obstacle[cursor_x][cursor_y]=(unsigned char)!obstacle[cursor_x][cursor_y];
    else if(ch=='v'||ch=='V')show_velocity=!show_velocity;
    else if(ch=='o'||ch=='O')load_preset_obstacle((++current_mode)%3);
    else if(ch=='c'||ch=='C')clear_simulation();
}
int main(int argc,char **argv) {
    int ch,done=0,qa=-1,i,x,y;unsigned frames=0,before61,after61;char status[80];
    unsigned checkCursor,checkShape;int screenOK,hookOK;union REGS r;
    if(argc==3&&!strcmp(argv[1],"/qa")&&argv[2][0]>='0'&&argv[2][0]<='2'&&!argv[2][1])qa=argv[2][0]-'0';
    else if(argc!=1) {puts("Usage: FLOW88 [/qa 0|1|2]");return 2;}
    if(!begin()) {puts("REFUSED: plain DOS, Tandy, 80x25 text mode 3 required.");return 3;}
    before61=(unsigned)inp(0x61)&0xcfu;draw_ui_frame();
    while(!done&&!stopped) {
        if(qa>=0) {
            if(frames==2) {
                if(qa==2)int86(0x23,&r,&r);else if(qa==1)applyKey(27);else done=1;
            }
            else {dens_prev[cursor_x][cursor_y]=INT_TO_FIX(8);u_prev[cursor_x][cursor_y]=INT_TO_FIX(2);}
        } else if(kbhit()) {
            ch=getch();
            if(ch==0||ch==0xe0) {
                ch=getch();
                if(ch==KEY_UP&&cursor_y>0)cursor_y--;
                if(ch==KEY_DOWN&&cursor_y<GRID_H-1)cursor_y++;
                if(ch==KEY_LEFT&&cursor_x>0)cursor_x--;
                if(ch==KEY_RIGHT&&cursor_x<GRID_W-1)cursor_x++;
            } else {
                applyKey(ch);
            }
        }
        if(done||stopped)break;
        if(inject_fluid) {dens_prev[cursor_x][cursor_y]=INT_TO_FIX(8);u_prev[cursor_x][cursor_y]=INT_TO_FIX(2);v_prev[cursor_x][cursor_y]=INT_TO_FIX(1);}
        fluid_step(FIX_HALF);render_viewport();frames++;
        sprintf(status,"FLOW-88 | frame %u | %s | audio unchanged",frames,show_velocity?"Velocity":"Density");
        vram_print_str(1,0,status,0x70);
    }
    cleanup();after61=(unsigned)inp(0x61)&0xcfu;
    if(qa>=0) {
        screenOK=1;
        for(i=0;i<2000;i++)if(((unsigned short far *)(0xb8000000UL+pageOffset))[i]!=oldScreen[i])screenOK=0;
        r.h.ah=3;r.h.bh=oldPage;int86(0x10,&r,&r);checkCursor=r.x.dx;checkShape=r.x.cx;
        x=checkCursor==oldCursor;y=checkShape==oldShape;hookOK=_dos_getvect(0x23)==oldBreak;
        printf("FLOWQA frames=%u stop=%d screen=%d cursor=%d shape=%d hook=%d sound61=%d\n",frames,stopped,screenOK,x,y,hookOK,before61==after61);
        return !(screenOK&&x&&y&&hookOK&&before61==after61&&frames==2);
    }
    return 0;
}
