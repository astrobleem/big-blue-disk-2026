/* Host check for Alfredo: After Hours. Plays the whole episode on the
 * virtual clock against memory buffers and checks that the picture engine,
 * the pond solver and the soundtrack behave:
 *   - the screen always equals the composed frame (no stale actors)
 *   - nothing is written outside the visible part of each video bank
 *   - every scene is reached, every sound effect fires, music stays on time
 *   - fluid values stay inside their clamps; the pond really moves
 *   - two runs produce identical pictures and sound (deterministic)
 * With a directory argument it also writes the frames, PSG.LOG and PAL.LOG
 * for tools/alfredo2_video.py.
 */
#define HOST
#define far
#define interrupt
#define _fmemcpy memcpy
#define _fmemset memset
#include "ALFREDO.C"
static unsigned char hbg[32768],hfrm[32768],hvid[32768];
static unsigned long hash;
static void mix(const unsigned char *p,unsigned n){while(n--)hash=(hash^*p++)*16777619UL;}
static int fail(const char *m){printf("FAIL %s (frame %lu, scene %d)\n",m,frames,scene_no);return 1;}
static void reset_all(void)
{
    memset(hbg,0,sizeof hbg);memset(hfrm,0,sizeof hfrm);memset(hvid,0,sizeof hvid);
    bg=hbg;frm=hfrm;video=hvid;
    frames=0;E=0;last_E=0;scene_no=-1;scene_bg=-1;fishing=0;ended=0;fl_primed=0;
    music_stop();sfx_end();music_events=0;music_late=0;music_late_max=0;sfx_count=0;psg_bytes=0;
    rn[0]=rn[1]=0;rcur=0;fl_total_steps=0;act_conflicts=0;act_forget();
    memset(scene_seen,0,sizeof scene_seen);
    clk_virtual=1;clk_vtime=0;mode_render=1;snd_ok=1;psg_invalidate();
    {int k;for(k=0;k<16;k++)pal_hw[k]=(unsigned char)k;}
}
static int run(const char *dir,unsigned long *out_hash,unsigned long *nframes)
{
    char name[300];FILE *f;unsigned y,b,i;int moved=0,maxv=0,maxd=0;
    reset_all();hash=2166136261UL;
    if(dir){sprintf(name,"%s/PSG.LOG",dir);psg_log=fopen(name,"w");sprintf(name,"%s/PAL.LOG",dir);pal_log=fopen(name,"w");}
    while(run_frame()) {
        mix(hvid,32768U);
        if(memcmp(hvid,hfrm,32768U))return fail("screen differs from composed frame");
        for(b=0;b<4;b++)for(i=8000;i<8192;i++)
            if(hbg[b*8192+i]||hfrm[b*8192+i]||hvid[b*8192+i])return fail("write outside visible bank area");
        for(i=0;i<NVCELL;i++) {
            if(u[i]>VMAX||u[i]< -VMAX||v[i]>VMAX||v[i]< -VMAX)return fail("velocity clamp");
            if(fp[i]>PMAX||fp[i]< -PMAX)return fail("pressure clamp");
            if(abs(u[i])>maxv)maxv=abs(u[i]);
        }
        for(i=0;i<NCELL;i++) {
            if(den[i]<0||den[i]>DMAX)return fail("heat clamp");
            if(den[i]>maxd)maxd=den[i];
        }
        if(fluid_scene(scene_no) && fl_steps>4 && maxv>100)moved=1;
        if(dir) {
            sprintf(name,"%s/F%05lu.RAW",dir,frames-1);f=fopen(name,"wb");
            for(y=0;y<SH;y++)fwrite(hvid+row_ofs[y],1,160,f);
            fclose(f);
        }
        if(frames>20000)return fail("episode never ended");
    }
    if(psg_log){fclose(psg_log);psg_log=0;}
    if(pal_log){fclose(pal_log);pal_log=0;}
    for(i=0;i<SC_END;i++)if(!scene_seen[i]){printf("FAIL scene %u never shown\n",i);return 1;}
    if(sfx_count!=NSFXQ){printf("FAIL %lu of %u sound effects fired\n",sfx_count,(unsigned)NSFXQ);return 1;}
    if(music_late)return fail("music events late on the virtual clock");
    if(act_conflicts){printf("FAIL %lu keyed-actor erase conflicts\n",act_conflicts);return 1;}
    if(!moved)return fail("pond never moved");
    *out_hash=hash;*nframes=frames;
    printf("frames %lu, episode %.1f s, fluid steps %lu, music events %lu, psg bytes %lu, rect peak %u merges %u, vram bytes %lu, peak |u| %d, peak heat %d\n",
        frames,total_fine/256.0/18.2065,fl_total_steps,music_events,psg_bytes,rect_peak,rect_merges,vram_bytes,maxv,maxd);
    return 0;
}
static int fish_check(void)
{
    int k;
    reset_all();fishing=1;E=total_fine;ended=1;
    for(k=0;k<600;k++) {
        if(k%40==5)handle_key(0x4d00);
        if(k%40==25)handle_key(0x4800);
        if(k%90==50)handle_key(' ');
        if(k==300)handle_key('h');
        run_frame();
        if(memcmp(hvid,hfrm,32768U))return fail("fish mode: screen differs from frame");
        if(scene_no!=SC_FISH)return fail("fish mode left early");
    }
    handle_key(27);run_frame();
    if(scene_no!=SC_END)return fail("Esc from fish mode does not return to the end card");
    printf("fish mode: 600 frames, lure, stir, heat and return to end card\n");
    return 0;
}
static int seek_check(void)
{
    int k;
    reset_all();mode_render=0;mode_auto=0;
    /* Jumping forward and back must repaint cleanly and keep music in step. */
    for(k=0;k<SC_END;k++){handle_key(0x4d00);run_frame();run_frame();
        if(memcmp(hvid,hfrm,32768U))return fail("seek: stale picture");}
    for(k=0;k<6;k++){handle_key(0x4b00);run_frame();
        if(memcmp(hvid,hfrm,32768U))return fail("seek back: stale picture");}
    handle_key('r');run_frame();
    if(scene_no!=SC_TITLE)return fail("replay does not restart");
    printf("scene skip forward/back and replay: clean repaint\n");
    mode_render=1;
    return 0;
}
int main(int argc,char **argv)
{
    unsigned long h1,h2,n1,n2;
    gfx_init();timeline_init();
    if(run(argc>1?argv[1]:0,&h1,&n1))return 1;
    if(run(argc>2?argv[2]:0,&h2,&n2))return 1;
    if(h1!=h2||n1!=n2){printf("FAIL two runs differ\n");return 1;}
    /* Kept actors must look exactly like freshly drawn ones. */
    act_off=1;
    if(run(argc>3?argv[3]:0,&h2,&n2))return 1;
    act_off=0;
    if(h1!=h2){printf("FAIL keeping unchanged actors changes the picture\n");return 1;}
    if(fish_check()||seek_check())return 1;
    printf("PASS alfredo2: %lu frames three times (actors kept / kept / always redrawn), identical (hash %08lx); %d scenes; %u sound effects\n",
        n1,h1&0xffffffffUL,SC_END,(unsigned)NSFXQ);
    return 0;
}
