/* The pond: Flow-88's stable-fluids solver, cut down to run under a cartoon.
 *
 * Same method as src/flow88/FLOWCORE.H (Gemini design, Claude port, Codex
 * safety corrections): add forces, semi-Lagrangian advection with bilinear
 * sampling, Gauss-Seidel pressure projection, 8.8 fixed point. Changes for
 * an 8088 that also has to draw and play music:
 *   - the current is solved on a coarse 16x8 grid; the warm water it
 *     carries is advected on a fine 32x16 grid, so the plume keeps a sharp
 *     edge while the expensive pressure solve stays small
 *   - every value is an int and every product stays inside 16 bits
 *     (4-bit interpolation weights, clamped velocity, heat and pressure)
 *   - one solver step is spread over five stages; only the last draws
 *   - cell types instead of a boundary pass: bank/boulder, the data
 *     center's warm OUTFLOW pipe (fixed inflow) and its cooling-water
 *     INTAKE (open: pressure zero, water leaves)
 *   - cheap vorticity confinement so the wake behind the boulder curls
 *   - heat is drawn as 2x2 interpolated sub-blocks per fine cell, a
 *     nine-step dithered palette ramp, writing only blocks that changed
 */
/* Fine (heat, drawing) grid. */
#define NX 32
#define NY 16
#define SX (NX+2)
#define NCELL (SX*(NY+2))
#define POND_X 32
#define POND_Y 36
#define CELL 8
/* Coarse (current) grid: one coarse cell is 2x2 fine cells. */
#define VX 16
#define VY 8
#define SVX (VX+2)
#define NVCELL (SVX*(VY+2))
#define VMAX 900
#define DMAX 2047
#define PMAX 12000
#define T_FLUID 0
#define T_WALL 1
#define T_INLET 2
#define T_OPEN 3
/* Pipe rows, intake rows and the boulder, in coarse cells... */
#define INLET_C0 3
#define INLET_C1 4
#define INTAKE_C0 3
#define INTAKE_C1 5
#define ROCK_C0 6
#define ROCK_R0 4
/* ...and in fine cells / pixels for the scenery. */
#define INLET_Y0 (INLET_C0*2-1)
#define INLET_Y1 (INLET_C1*2)
#define INTAKE_Y0 (INTAKE_C0*2-1)
#define INTAKE_Y1 (INTAKE_C1*2)
#define ROCK_PX (POND_X+(ROCK_C0-1)*2*CELL)
#define ROCK_PY (POND_Y+(ROCK_R0-1)*2*CELL)
#define ROCK_CX (ROCK_PX+2*CELL)
#define ROCK_CY (ROCK_PY+2*CELL)
static int fa[NVCELL],fb[NVCELL],fc[NVCELL],fd[NVCELL],fp[NVCELL],fq[NVCELL];
static int fe[NCELL],ff[NCELL];
static int *u=fa,*v=fb,*ut=fc,*vt=fd,*den=fe,*dt=ff;
static unsigned char vtype[NVCELL],nbmask[NVCELL],htype[NCELL];
static unsigned char fcache[NX*NY*4];
static int fl_stage,fl_inlet_u=150,fl_inlet_d=1800,fl_vort=12,fl_iters=4;
static int fl_inlet_wobble;
static unsigned fl_steps;
static unsigned char fl_ramp[5]={1,2,3,4,5};    /* logical colors, cold to hot */
#define NPART 14
static int part_x[NPART],part_y[NPART],part_dx[NPART],part_dy[NPART];
static unsigned char lvc[NCELL],lvn[NCELL];
static unsigned part_seed=12345;
static void mark_bg(int x0,int y0,int x1,int y1);

static int clampv(int x){return x>VMAX?VMAX:(x<-VMAX?-VMAX:x);}
/* Interpolate a..b at w quarters, w 0..3, with shifts: an 8088 MUL costs
   as much as a dozen ordinary instructions. */
static int lerp4(int a,int b,int w)
{
    int d=b-a;
    switch(w) {
    case 0:return a;
    case 1:return a+(d>>2);
    case 2:return a+(d>>1);
    default:return b-(d>>2);
    }
}
static int vrow[VY+2],hrow[NY+2],vrow_of_h[NY+2];
static void fl_tables(void)
{
    int y;
    for(y=0;y<VY+2;y++)vrow[y]=y*SVX;
    for(y=0;y<NY+2;y++){hrow[y]=y*SX;vrow_of_h[y]=((y+1)>>1)*SVX;}
}
static unsigned rnd16(void){part_seed=(part_seed*25173U+13849U)&0xffffU;return part_seed;}
/* Coarse cell under fine cell (x,y), both in storage coordinates. */
#define VC(x,y) ((((y)+1)>>1)*SVX+(((x)+1)>>1))

static void fl_layout(void)
{
    int x,y,i;
    for(i=0;i<NVCELL;i++)vtype[i]=T_FLUID;
    for(x=0;x<SVX;x++){vtype[x]=T_WALL;vtype[(VY+1)*SVX+x]=T_WALL;}
    for(y=0;y<VY+2;y++) {
        vtype[y*SVX]=(unsigned char)(y>=INLET_C0 && y<=INLET_C1?T_INLET:T_WALL);
        vtype[y*SVX+SVX-1]=(unsigned char)(y>=INTAKE_C0 && y<=INTAKE_C1?T_OPEN:T_WALL);
    }
    for(y=0;y<2;y++)for(x=0;x<2;x++)vtype[(ROCK_R0+y)*SVX+ROCK_C0+x]=T_WALL;
    /* Which neighbours mirror pressure (solid or inlet: zero normal flow). */
    for(i=SVX;i<NVCELL-SVX;i++) {
        nbmask[i]=0;
        if(vtype[i]!=T_FLUID)continue;
        if(vtype[i-1]==T_WALL || vtype[i-1]==T_INLET)nbmask[i]|=1;
        if(vtype[i+1]==T_WALL || vtype[i+1]==T_INLET)nbmask[i]|=2;
        if(vtype[i-SVX]==T_WALL || vtype[i-SVX]==T_INLET)nbmask[i]|=4;
        if(vtype[i+SVX]==T_WALL || vtype[i+SVX]==T_INLET)nbmask[i]|=8;
    }
    for(y=0;y<NY+2;y++)for(x=0;x<SX;x++)htype[y*SX+x]=vtype[VC(x,y)];
}
static void fl_reset(void)
{
    int i;
    memset(fa,0,sizeof(fa));memset(fb,0,sizeof(fb));memset(fc,0,sizeof(fc));
    memset(fd,0,sizeof(fd));memset(fe,0,sizeof(fe));memset(ff,0,sizeof(ff));
    memset(fp,0,sizeof(fp));memset(fq,0,sizeof(fq));
    u=fa;v=fb;ut=fc;vt=fd;den=fe;dt=ff;
    memset(fcache,255,sizeof(fcache));
    fl_tables();fl_layout();fl_stage=0;fl_steps=0;part_seed=12345;fl_inlet_wobble=0;
    memset(lvc,255,sizeof(lvc));
    for(i=0;i<NPART;i++)part_dx[i]=-1;
    for(i=0;i<NPART;i++){part_x[i]=(int)(256+rnd16()%(NX*256));part_y[i]=(int)(256+rnd16()%(NY*256));}
}
/* Boundary cells: inflow fixed, intake copies its neighbour, walls zero. */
static void fl_bounds(void)
{
    int y,i;
    for(y=INLET_C0;y<=INLET_C1;y++) {
        i=y*SVX;u[i]=fl_inlet_u;v[i]=fl_inlet_wobble;
        if(u[i+1]<fl_inlet_u)u[i+1]=fl_inlet_u;
    }
    for(y=INTAKE_C0;y<=INTAKE_C1;y++){i=y*SVX+SVX-1;u[i]=u[i-1]>0?u[i-1]:0;v[i]=0;}
}
static void fl_heat_bounds(void)
{
    int y;
    for(y=INLET_Y0;y<=INLET_Y1;y++)den[y*SX]=fl_inlet_d;
    for(y=INTAKE_Y0;y<=INTAKE_Y1;y++)den[y*SX+SX-1]=den[y*SX+SX-2];
}
/* Bilinear backtrace of a coarse field along the current. */
static void fl_advect(int *out,const int *in)
{
    int x,y,i,j,px,py,fx,fy,top,bot;
    for(y=1;y<=VY;y++) {
        i=y*SVX+1;
        for(x=1;x<=VX;x++,i++) {
            if(vtype[i]!=T_FLUID){out[i]=in[i];continue;}
            px=(x<<8)-u[i];py=(y<<8)-v[i];
            if(px<128)px=128;
            if(px>(VX<<8)+127)px=(VX<<8)+127;
            if(py<128)py=128;
            if(py>(VY<<8)+127)py=(VY<<8)+127;
            j=vrow[py>>8]+(px>>8);fx=(px>>6)&3;fy=(py>>6)&3;
            top=lerp4(in[j],in[j+1],fx);bot=lerp4(in[j+SVX],in[j+SVX+1],fx);
            out[i]=clampv(lerp4(top,bot,fy));
        }
    }
}
/* Heat on the fine grid, carried by the coarse current (two fine cells
   per coarse cell, so the displacement doubles). */
static void fl_advect_heat(void)
{
    int x,y,i,j,k,kr,px,py,fx,fy,a,top,bot;
    for(y=1;y<=NY;y++) {
        i=hrow[y]+1;kr=vrow_of_h[y];
        for(x=1;x<=NX;x++,i++) {
            if(htype[i]!=T_FLUID){dt[i]=0;continue;}
            k=kr+((x+1)>>1);
            px=(x<<8)-(u[k]<<1);py=(y<<8)-(v[k]<<1);
            if(px<128)px=128;
            if(px>(NX<<8)+127)px=(NX<<8)+127;
            if(py<128)py=128;
            if(py>(NY<<8)+127)py=(NY<<8)+127;
            j=hrow[py>>8]+(px>>8);fx=(px>>6)&3;fy=(py>>6)&3;
            top=lerp4(den[j],den[j+1],fx);bot=lerp4(den[j+SX],den[j+SX+1],fx);
            a=lerp4(top,bot,fy);
            dt[i]=a<0?0:a-(a>>7);   /* slow heat loss */
        }
        if((y&3)==3)audio_poll();
    }
}
static void fl_project(void)
{
    int x,y,i,k,m,c,s;
    for(y=1;y<=VY;y++) {
        i=y*SVX+1;
        for(x=1;x<=VX;x++,i++) {
            fq[i]=vtype[i]==T_FLUID?-(((u[i+1]-u[i-1])+(v[i+SVX]-v[i-SVX]))>>1):0;
            fp[i]=0;
        }
    }
    for(i=0;i<NVCELL;i++)if(vtype[i]!=T_FLUID)fp[i]=0;
    for(k=0;k<fl_iters;k++) {
        for(y=1;y<=VY;y++) {
            i=y*SVX+1;
            for(x=1;x<=VX;x++,i++) {
                if(vtype[i]!=T_FLUID)continue;
                m=nbmask[i];
                /* Pairwise halving keeps every sum inside 16 bits. */
                if(!m)s=((fp[i-1]+fp[i+1])>>1)+((fp[i-SVX]+fp[i+SVX])>>1);
                else {
                    c=fp[i];
                    s=(((m&1?c:fp[i-1])+(m&2?c:fp[i+1]))>>1)+(((m&4?c:fp[i-SVX])+(m&8?c:fp[i+SVX]))>>1);
                }
                s=(s>>1)+(fq[i]>>2);
                fp[i]=s>PMAX?PMAX:(s<-PMAX?-PMAX:s);
            }
        }
        audio_poll();
    }
    for(y=1;y<=VY;y++) {
        i=y*SVX+1;
        for(x=1;x<=VX;x++,i++) {
            if(vtype[i]!=T_FLUID){u[i]=v[i]=0;continue;}
            m=nbmask[i];c=fp[i];
            u[i]=clampv(u[i]-(((m&2?c:fp[i+1])-(m&1?c:fp[i-1]))>>1));
            v[i]=clampv(v[i]-(((m&8?c:fp[i+SVX])-(m&4?c:fp[i-SVX]))>>1));
        }
    }
}
/* Vorticity confinement with sign-normalised gradients (no divides). */
static void fl_confine(void)
{
    int x,y,i,w,gx,gy,ax,ay,nx,ny;
    if(!fl_vort)return;
    for(i=SVX;i<NVCELL-SVX;i++) {
        if(vtype[i]!=T_FLUID){fq[i]=0;fp[i]=0;continue;}
        w=((v[i+1]-v[i-1])-(u[i+SVX]-u[i-SVX]))>>1;
        fq[i]=w;fp[i]=w<0?-w:w;
    }
    for(y=2;y<VY;y++) {
        i=y*SVX+2;
        for(x=2;x<VX;x++,i++) {
            if(vtype[i]!=T_FLUID)continue;
            gx=fp[i+1]-fp[i-1];gy=fp[i+SVX]-fp[i-SVX];
            ax=gx<0?-gx:gx;ay=gy<0?-gy:gy;
            if(ax+ay<8)continue;
            nx=ax*2<ay?0:(gx<0?-1:1);ny=ay*2<ax?0:(gy<0?-1:1);
            w=(fq[i]*fl_vort)>>8;
            u[i]=clampv(u[i]+ny*w);v[i]=clampv(v[i]-nx*w);
        }
    }
}
/* Script hooks, in fine cell coordinates: push water, add heat, stir. */
static void fl_push(int x,int y,int du,int dv)
{
    int i;
    if(x<1 || x>NX || y<1 || y>NY)return;
    i=VC(x,y);
    if(vtype[i]!=T_FLUID)return;
    u[i]=clampv(u[i]+du);v[i]=clampv(v[i]+dv);
}
static void fl_ink(int x,int y,int amt)
{
    int i,d;
    if(x<1 || x>NX || y<1 || y>NY)return;
    i=y*SX+x;
    if(htype[i]!=T_FLUID)return;
    d=den[i]+amt;den[i]=d>DMAX?DMAX:(d<0?0:d);
}
static void fl_swirl(int x,int y,int s)
{
    fl_push(x,y-2,s,0);fl_push(x+2,y,0,s);fl_push(x,y+2,-s,0);fl_push(x-2,y,0,-s);
}
/* Where actors are (or were last picture), in 4x4 sub-blocks of the pond.
   Water elsewhere goes straight to the screen; under an actor it waits
   for the picture's commit so the actor never flickers. */
#define OCC_W (NX*2/8)
static unsigned char occ[NY*2][OCC_W];
static void occ_rect(const Rect *r)
{
    int x0=(r->c0*2-POND_X)>>2,x1=(r->c1*2+1-POND_X)>>2,y0=((int)r->y0-POND_Y)>>2,y1=((int)r->y1-POND_Y)>>2,x,y;
    if(x1<0 || y1<0 || x0>=NX*2 || y0>=NY*2)return;
    if(x0<0)x0=0;
    if(y0<0)y0=0;
    if(x1>=NX*2)x1=NX*2-1;
    if(y1>=NY*2)y1=NY*2-1;
    for(y=y0;y<=y1;y++)for(x=x0;x<=x1;x++)occ[y][x>>3]|=(unsigned char)(1<<(x&7));
}
static void occ_build(void)
{
    unsigned i;
    memset(occ,0,sizeof(occ));
    for(i=0;i<rn[rcur];i++)occ_rect(&rl[rcur][i]);
    for(i=1;i<MAXACT;i++)if(act_live[i])occ_rect(&act_r[i]);
}
static unsigned fl_direct,fl_marked;
/* Heat colors: nine steps over five ramp colors, dithered between.
   full: paint every block into bg and frm only (backdrop, repaint). */
static void fl_draw(int full)
{
    int x,y,i,q,minx,maxx,px0,py0,lev,dd,sx,sy,r,d,sy0;
    unsigned char e,o,a,b,cc;unsigned char *cp;unsigned off;
    for(i=0;i<NCELL;i++){d=den[i]>>8;lvn[i]=(unsigned char)(d>8?8:d);}
    if(!full)occ_build();
    for(y=1;y<=NY;y++) {
        minx=99;maxx=-1;
        i=y*SX+1;cp=fcache+(y-1)*NX*4;
        for(x=1;x<=NX;x++,i++,cp+=4) {
            if(htype[i]==T_WALL)continue;
            /* Nothing round here changed level: leave the block alone. */
            if(!full && lvn[i]==lvc[i] && lvn[i-1]==lvc[i-1] && lvn[i+1]==lvc[i+1]
               && lvn[i-SX]==lvc[i-SX] && lvn[i+SX]==lvc[i+SX])continue;
            for(q=0;q<4;q++) {
                sx=q&1?1:-1;sy=q&2?SX:-SX;
                /* Banks and boulder hold no heat: water darkens a little there. */
                dd=(den[i]*2+den[i+sx]+den[i+sy])>>2;
                lev=dd>>8;
                if(lev>8)lev=8;
                if(!full && cp[q]==(unsigned char)lev)continue;
                cp[q]=(unsigned char)lev;
                a=fl_ramp[lev>>1];b=fl_ramp[(lev+1)>>1];
                e=(unsigned char)((a<<4)|b);o=(unsigned char)((b<<4)|a);
                px0=POND_X+(x-1)*CELL+(q&1)*4;sy0=(y-1)*2+(q>>1);py0=POND_Y+sy0*4;
                cc=(unsigned char)(!full && !(occ[sy0][((x-1)*2+(q&1))>>3]&(1<<((((x-1)*2)+(q&1))&7))));
                for(r=0;r<4;r++) {
                    off=row_ofs[py0+r]+(px0>>1);
                    bg[off]=bg[off+1]=frm[off]=frm[off+1]=(r&1)?o:e;
                    if(cc)video[off]=video[off+1]=(r&1)?o:e;
                }
                if(cc){fl_direct++;continue;}
                fl_marked++;
                if(x<minx)minx=x;
                if(x>maxx)maxx=x;
            }
        }
        if(maxx>=0 && !full)mark_bg(POND_X+(minx-1)*CELL,POND_Y+(y-1)*CELL,POND_X+maxx*CELL-1,POND_Y+y*CELL-1);
        if(y&1)audio_poll();
    }
    for(i=0;i<NCELL;i++)lvc[i]=lvn[i];
}
/* Flotsam: leaves and foam carried by the current, drawn with the water. */
static unsigned char part_c=7;
static void fl_particles(void)
{
    int k,i;
    for(k=0;k<NPART;k++) {
        i=vrow_of_h[part_y[k]>>8]+(((part_x[k]>>8)+1)>>1);
        part_x[k]+=u[i];part_y[k]+=v[i];
        if(part_x[k]<256 || part_x[k]>=(NX+1)*256 || part_y[k]<256 || part_y[k]>=(NY+1)*256
           || htype[hrow[part_y[k]>>8]+(part_x[k]>>8)]==T_WALL) {
            part_x[k]=256+(int)(rnd16()&511);
            part_y[k]=(INLET_Y0<<8)+(int)(rnd16()%((INLET_Y1-INLET_Y0+1)<<8));
        }
    }
}
static int occupied(int x,int y)
{
    int sx=(x-POND_X)>>2,sy=(y-POND_Y)>>2;
    if(sx<0 || sy<0 || sx>=NX*2 || sy>=NY*2)return 1;
    return (occ[sy][sx>>3]>>(sx&7))&1;
}
static void part_byte(int x,int y,int put)
{
    unsigned off=row_ofs[y]+(x>>1);
    if(put)frm[off]=(unsigned char)((x&1)?(frm[off]&0xf0)|part_c:(frm[off]&0x0f)|(part_c<<4));
    else frm[off]=bg[off];
    if(occupied(x,y))mark_bg(x&~1,y,x|1,y);
    else video[off]=frm[off];
}
static void fl_draw_particles(void)
{
    int k,x,y;
    for(k=0;k<NPART;k++) {
        if(part_dx[k]>=0)part_byte(part_dx[k],part_dy[k],0);
        /* Cells are 8 pixels: (pos-256)*8/256 without a 16-bit overflow. */
        x=POND_X+((part_x[k]-256)>>5);y=POND_Y+((part_y[k]-256)>>5);
        part_byte(x,y,1);part_dx[k]=x;part_dy[k]=y;
    }
}
/* One fifth of a solver step. Stage 4 draws; the others touch no pixels. */
#define FL_DRAW_STAGE 4
static int fl_stage_run(void)
{
    int *t;
    switch(fl_stage) {
    case 0:
        /* Confinement on alternate steps: the curl survives, the 8088 breathes. */
        fl_bounds();if(fl_steps&1)fl_confine();fl_advect(ut,u);break;
    case 1:
        fl_advect(vt,v);
        t=u;u=ut;ut=t;t=v;v=vt;vt=t;fl_bounds();break;
    case 2:
        fl_project();fl_bounds();break;
    case 3:
        fl_heat_bounds();fl_advect_heat();
        t=den;den=dt;dt=t;fl_heat_bounds();fl_particles();break;
    default:
        fl_draw(0);fl_draw_particles();fl_stage=0;fl_steps++;return 1;
    }
    fl_stage++;
    return 0;
}
/* Current under a screen point, in fine cells per step (8.8). */
static void pond_vel(int sx,int sy,int *pu,int *pv)
{
    int x=(sx-POND_X)/CELL+1,y=(sy-POND_Y)/CELL+1,i;
    if(x<1)x=1;
    if(x>NX)x=NX;
    if(y<1)y=1;
    if(y>NY)y=NY;
    i=VC(x,y);*pu=u[i]<<1;*pv=v[i]<<1;
}
