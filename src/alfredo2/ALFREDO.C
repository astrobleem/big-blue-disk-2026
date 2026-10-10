/* Alfredo: After Hours. Big Blue Disk 2026 cartoon, episode 2. GPLv3.
 * C89 for Microsoft C 6 /G0 /AS (small model, 8088); also OpenWatcom 2.
 *
 * Needs a Tandy 1000 (320x200x16 mode 9, SN76496 sound). No resident code
 * and no timer reprogramming: the fine clock only latches PIT 0. Ctrl-Break
 * is caught through INT 23h, which DOS restores from the PSP on exit. The
 * PSG is claimed through the shared DOSSND owner and silenced on exit; the
 * original video mode is restored.
 *
 * Keys: Space/P pause, Left/Right previous/next scene, R replay, M mute,
 * Esc exit. After the credits: F stays at the pond (a playable Flow-88).
 * Switches: /AUTO plays once and logs timing, /QA and /RENDER run on a
 * virtual clock (scene stills or every frame plus PSG/palette logs).
 */
#ifndef HOST
#include <dos.h>
#include <conio.h>
#include <malloc.h>
#include <bios.h>
#include "DOSGUARD.H"
#include "DOSSND.H"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CLOCK.H"
#include "ENGINE.H"
#include "SOUND.H"
#include "FLUID.H"
#include "CAST.H"
#include "SCENES.H"

static int mode_qa,mode_render,mode_auto,quit,only_scene=-1;
static int paused,fishing,ended;
static unsigned long last_E,frames,worst_real,total_fine;
static unsigned long scene_starts[SC_COUNT];
static unsigned long fl_total_steps;
#define ANIM_Q 384
#define STAGE_Q 128
static unsigned long anim_key=0xffffffffUL,stage_at;
static int was_paused=-1;
static unsigned long draws;
/* /AUTO: where the time goes (fine units): restore, pond, actors, commit, palette. */
static unsigned long prof[5];
#define PT(k) if(mode_auto){unsigned long t_=clk_real();prof[k]+=t_-pt0;pt0=t_;}

static FILE *pal_log;
static int scene_seen[SC_COUNT];
static unsigned long sc_frames[SC_COUNT],sc_worst[SC_COUNT],sc_late[SC_COUNT],late_all,change_worst;

static unsigned long sfx_fine[NSFXQ];
static void timeline_init(void)
{
    int k;unsigned long t=0;
    for(k=0;k<(int)NSFXQ;k++)sfx_fine[k]=(unsigned long)DS(sfxq[k].ds)<<4;
    for(k=0;k<SC_END;k++){scene_starts[k]=t;t+=(unsigned long)scenes[k].ticks<<8;}
    total_fine=t;scene_starts[SC_END]=t;
}
static int scene_at(unsigned long e)
{
    int k;
    for(k=0;k<SC_END;k++)if(e<scene_starts[k]+((unsigned long)scenes[k].ticks<<8))return k;
    return SC_END;
}
/* Which cue should be sounding now, and since when. */
static void music_sync(void)
{
    int k=scene_no,c=-1;unsigned long t0=0;
    if(scene_no==SC_FISH){c=CUE_POND;t0=sc_start;}
    else {
        for(;k>=0;k--)if(scenes[k].cue>=0){c=scenes[k].cue;t0=scene_starts[k];break;}
        if(scenes[scene_no].cue_end && su>=DS(scenes[scene_no].cue_end))c=-1;
    }
    if(c!=cue_no || (c>=0 && t0!=cue_start))music_seek(c,t0,E);
}
static void pal_flush(void)
{
    unsigned i,o;int dirty=0;
    for(i=0;i<16;i++)if(pal_out(i)!=pal_hw[i])dirty=1;
    if(!dirty)return;
    vsync_wait();
#ifndef HOST
    (void)inp(0x3da);
#endif
    for(i=0;i<16;i++) {
        o=pal_out(i);
        if(o==pal_hw[i])continue;
#ifndef HOST
        outp(0x3da,0x10+i);outp(0x3de,o);
#endif
        pal_hw[i]=(unsigned char)o;pal_writes++;
    }
#ifndef HOST
    (void)inp(0x3da);outp(0x3da,0);
#endif
    if(pal_log) {
        fprintf(pal_log,"%lu",frames);
        for(i=0;i<16;i++)fprintf(pal_log," %u",pal_hw[i]);
        fputc('\n',pal_log);
    }
}
/* Called from long loops: keeps the episode clock and the music moving. */
static unsigned long clk_prev;
static void audio_poll(void)
{
    unsigned long now;
    if(!clk_virtual) {
        now=clk_fine();
        if(!paused)E+=now-clk_prev;
        clk_prev=now;
    }
    if(paused)return;

    if(cue_no>=0)music_poll(E);
    music_render(E);sfx_render(E);
}
static int fade_level(void)
{
    int fin=4,fout=0;unsigned step=DS(3)/4,len;
    if(scene_no>0 && scene_no<SC_END && scenes[scene_no-1].bgid==scenes[scene_no].bgid)fin=0;
    else fin=4-(int)(su/step);
    if(fin<0)fin=0;
    if(scene_no<SC_END-1 && scenes[scene_no+1].bgid!=scenes[scene_no].bgid) {
        len=(unsigned)(((unsigned long)scenes[scene_no].ticks<<8)>>4);
        if(su+step*4>len)fout=4-(int)((len-su)/step);
    }
    if(scene_no==SC_CREDITS)fout=0;
    return fin>fout?fin:fout;
}
static void go_scene(int n,unsigned long start)
{
    int b=scenes[n].bgid;
    scene_no=n;sc_start=start;su=0;
    scene_enter(n);scene_seen[n]=1;
    if(b!=scene_bg) {
        pal_fade=4;pal_flush();
        scene_backdrop(b);present_full();scene_bg=b;
    }
    scene_prepare(n);
}
static void jump_to(unsigned long e)
{
    E=e;last_E=e;scene_no=-1;scene_bg=-1;fishing=0;anim_key=0xffffffffUL;
    if(e<scene_starts[SC_SURGE] || e>=scene_starts[SC_SURGE+1])fl_primed=0;
    music_stop();sfx_end();snd_silence();
}
static void sfx_due(void)
{
    unsigned k;unsigned long t;
    for(k=0;k<NSFXQ;k++) {
        if(sfxq[k].sc!=scene_no)continue;
        t=sc_start+sfx_fine[k];
        if(t>last_E && t<=E)sfx_start(sfxq[k].id,t);
        if(t==0 && E==0 && last_E==0)sfx_start(sfxq[k].id,t);
    }
    last_E=E;
}
/* Animation is drawn "on threes": a new picture every 384 fine units
   (about 12 a second), the classic cartoon rate. Between pictures the
   machine only keeps the music going and works on the pond. */
/* One pass of the main loop. Returns 0 when a capture run is finished. */
static int run_frame(void)
{
    int n,draw;unsigned long d,key;
    if(fishing)n=SC_FISH;
    else n=scene_at(E);
    if(n!=scene_no){go_scene(n,n==SC_FISH?E:scene_starts[n]);anim_key=0xffffffffUL;stage_at=E;}
    d=E-sc_start;key=(unsigned)(d>>7)/3U;      /* d/ANIM_Q without a long divide */
    su=key<2730UL?(unsigned)key*(ANIM_Q/16):65535U;
    if(n==SC_END)ended=1;
    draw=key!=anim_key || paused!=was_paused || fl_repaint;
    /* Solver stages 0-2 touch no pixels; stage 3 draws, so it needs a picture. */
    if(fluid_scene(n) && !paused && (clk_virtual || E-stage_at>=STAGE_Q)) {
        if(fl_stage<FL_DRAW_STAGE){unsigned long pt0=mode_auto?clk_real():0;fl_stage_run();stage_at=E;PT(1)}
        else draw=1;
    }
    if(draw) {
        unsigned long pt0=mode_auto?clk_real():0;
        frame_begin();PT(0)
        if(fluid_scene(n) && !paused && fl_stage==FL_DRAW_STAGE && (clk_virtual || E-stage_at>=STAGE_Q))
            {fl_stage_run();fl_total_steps++;stage_at=E;}
        if(fl_repaint){fl_repaint=0;fl_draw(1);mark_bg(POND_X,POND_Y,POND_X+NX*CELL-1,POND_Y+NY*CELL-1);}
        PT(1)
        scene_draw(n);
        if(paused){caption(92,"PAUSED",15,9);obj();}
        PT(2)
        frame_commit();PT(3)
        anim_key=key;was_paused=paused;draws++;
    }
    if(!paused){sfx_due();music_sync();}
    audio_poll();
    pal_fade=fade_level();
    {unsigned long pt0=mode_auto?clk_real():0;pal_flush();PT(4)}
    frames++;
    if(clk_virtual) {
        E+=RENDER_STEP;clk_vtime=E;
        if((mode_render || mode_qa) && E>=total_fine+(4UL<<8)*18)return 0;
    }
    if(mode_auto && E>=total_fine+(2UL<<8)*18)return 0;
    return 1;
}
static void handle_key(int k)
{
    unsigned long t;int n;
    if(fishing) {
        if(k==27){fishing=0;jump_to(total_fine);return;}
        fish_key(k);return;
    }
    switch(k) {
    case 27:case 3:quit=1;break;        /* Esc, or Ctrl-C read from the BIOS */
    case ' ':case 'p':case 'P':
        paused=!paused;
        if(paused)snd_silence();
        break;
    case 'r':case 'R':jump_to(0);paused=0;break;
    case 'm':case 'M':snd_mute=!snd_mute;if(snd_mute){snd_mute=0;snd_silence();snd_mute=1;}else psg_invalidate();break;
    case 'f':case 'F':
        if(ended){jump_to(E);fishing=1;}
        break;
    case 0x4d00:
        n=scene_at(E);
        if(n<SC_END){jump_to(scene_starts[n+1]);paused=0;}
        break;
    case 0x4b00:
        n=scene_at(E);t=n<SC_END?scene_starts[n]:total_fine;
        if(E-t<(18UL<<8) && n>0)t=scene_starts[n-1];
        if(n==SC_END)t=scene_starts[SC_CREDITS];
        jump_to(t);paused=0;
        break;
    }
}

#ifndef HOST
static int read_key(void)
{
    unsigned k;
    if(!_bios_keybrd(_KEYBRD_READY))return 0;
    k=_bios_keybrd(_KEYBRD_READ);
    if((k&255)==0 || (k&255)==0xe0)return (int)(k&0xff00);
    return (int)(k&255);
}
static void set_mode(int m){union REGS r;r.x.ax=m;int86(0x10,&r,&r);}
static int get_mode(void){union REGS r;r.h.ah=15;int86(0x10,&r,&r);return r.h.al;}
static void (interrupt far *old23)(void);
static void interrupt far on_break(void){quit=1;}
static unsigned char rowbuf[160];
static void dump_frame(const char *name)
{
    FILE *f=fopen(name,"wb");unsigned y;
    if(!f)return;
    for(y=0;y<SH;y++){_fmemcpy((void far *)rowbuf,video+row_ofs[y],160);fwrite(rowbuf,1,160,f);}
    fclose(f);
}
int main(int argc,char **argv)
{
    int old,k,owned=0,mid;char name[16];FILE *f;
    unsigned long t0,t1,d0,qa_done[3];
    for(k=1;k<argc;k++) {
        if(!strcmp(argv[k],"/QA"))mode_qa=1;
        else if(!strcmp(argv[k],"/RENDER"))mode_render=1;
        else if(!strcmp(argv[k],"/AUTO"))mode_auto=1;
        else if(!strcmp(argv[k],"/FISH"))fishing=1;
        else if(!strcmp(argv[k],"/QUIET"))snd_mute=1;
        else if(argv[k][0]=='/' && argv[k][1]=='S' && argv[k][2]>='0' && argv[k][2]<='9')only_scene=atoi(argv[k]+2);
    }
    qa_done[0]=qa_done[1]=qa_done[2]=0;
    if(DosWindows()){puts("Please run Alfredo from plain DOS, not Windows.");return 1;}
    if(!DosSessionPermit()) {
        puts("Alfredo: After Hours needs a Tandy 1000 (320x200 16-color mode).");
        return 1;
    }
    bg=(unsigned char far *)_fmalloc(BUFSZ);frm=(unsigned char far *)_fmalloc(BUFSZ);
    if(!bg || !frm){puts("Not enough free memory for Alfredo (needs about 100K).");return 1;}
    video=(unsigned char far *)0xb8000000UL;
    _fmemset(bg,0,BUFSZ);_fmemset(frm,0,BUFSZ);
    if(DosSoundAcquire(DS_PSG)==0){owned=1;snd_ok=1;}
    old=get_mode();set_mode(9);
    gfx_init();timeline_init();psg_invalidate();
    for(k=0;k<16;k++)pal_hw[k]=(unsigned char)k;
    old23=_dos_getvect(0x23);_dos_setvect(0x23,on_break);
    if(mode_qa || mode_render){clk_virtual=1;clk_vtime=0;}
    if(mode_render){psg_log=fopen("PSG.LOG","w");pal_log=fopen("PAL.LOG","w");}
    if(fishing){fishing=0;E=total_fine;ended=1;fishing=1;}
    clk_prev=clk_fine();
    snd_silence();
    if(only_scene>=0 && only_scene<SC_END){E=scene_starts[only_scene];last_E=E;}
    for(;;) {
        if(!mode_qa && !mode_render) {
            k=read_key();
            if(k)handle_key(k);
            if(quit)break;
        } else if(read_key()==27)break;
        if(quit)break;
        t0=clk_real();k=scene_no;music_late_max=0;d0=draws;
        if(!run_frame())break;
        if(only_scene>=0 && scene_no!=only_scene)break;
        t1=clk_real();
        if(music_late_max>late_all)late_all=music_late_max;
        if(k!=scene_no){if(frames>1 && t1-t0>change_worst)change_worst=t1-t0;}
        else if(draws==d0)continue;   /* nothing drawn: not a picture for the statistics */
        else if(scene_no>=0) {
            sc_frames[scene_no]++;
            if(t1-t0>sc_worst[scene_no])sc_worst[scene_no]=t1-t0;
            if(t1-t0>worst_real)worst_real=t1-t0;
            if(music_late_max>sc_late[scene_no])sc_late[scene_no]=music_late_max;
        }
        if(mode_render){sprintf(name,"F%05lu.RAW",frames-1);dump_frame(name);}
        if(mode_qa && scene_no<SC_END) {
            /* Stills at a quarter, half and three quarters of each scene. */
            for(mid=1;mid<=3;mid++)
                if(su>=(unsigned)(((unsigned long)scenes[scene_no].ticks<<4)*mid/4) &&
                   !(qa_done[mid-1]&(1UL<<scene_no))) {
                    qa_done[mid-1]|=1UL<<scene_no;
                    sprintf(name,"S%02d%c.RAW",scene_no,'A'+mid-1);dump_frame(name);
                }
        }

        if(mode_auto && read_key()==27)break;
    }
    snd_silence();
    if(owned)DosSoundRelease();
    set_mode(old);
    _dos_setvect(0x23,old23);
    _ffree(bg);_ffree(frm);
    if(psg_log)fclose(psg_log);
    if(pal_log)fclose(pal_log);
    if(mode_qa || mode_auto || mode_render) {
        f=fopen(mode_qa?"QA.TXT":(mode_auto?"AUTO.TXT":"RENDER.TXT"),"w");
        if(f) {
            fprintf(f,"frames=%lu pictures=%lu episode_ticks=%lu worst_frame_fine=%lu fluid_steps=%lu\n",
                frames,draws,total_fine>>8,worst_real,fl_total_steps);
            fprintf(f,"music_events=%lu music_late=%lu music_late_max_fine=%lu sfx=%lu psg_bytes=%lu\n",
                music_events,music_late,late_all,sfx_count,psg_bytes);
            fprintf(f,"scene_change_worst_fine=%lu\n",change_worst);
            fprintf(f,"time_restore=%lu time_pond=%lu time_actors=%lu time_commit=%lu time_palette=%lu\n",
                prof[0],prof[1],prof[2],prof[3],prof[4]);
            for(k=0;k<SC_COUNT;k++)if(sc_frames[k])
                fprintf(f,"scene%02d frames=%lu worst_fine=%lu late_fine=%lu\n",k,sc_frames[k],sc_worst[k],sc_late[k]);
            fprintf(f,"rect_peak=%u rect_merges=%u vram_bytes=%lu pal_writes=%lu sound=%d\n",
                rect_peak,rect_merges,vram_bytes,pal_writes,snd_ok);
            fprintf(f,"scenes=");
            for(k=0;k<SC_COUNT;k++)fputc(scene_seen[k]?'1':'0',f);
            fprintf(f,"\nprevious_mode=%d restored_mode=%d\n",old,get_mode());
            fclose(f);
        }
    }
    return 0;
}
#endif
