/* Original teaching helpers for SCREEN. Pure functions: no ports, no BIOS,
 * so HOSTQA.C can check them on the host. C89; MSC6 /G0 /AS compatible.
 *
 * Tandy 16-colour graphics store two pixels per byte, first pixel in the
 * HIGH nibble (Tandy 1000 EX Technical Reference, 1986, 16-colour mode
 * format). 320x200 uses four interleaved banks of 8000 bytes; scan line y
 * starts at (y&3)*0x2000 + (y>>2)*160 from the video window.
 */
#ifndef SCREEN_DEMOCORE_H
#define SCREEN_DEMOCORE_H

#define SCR_W        320u
#define SCR_H        200u
#define ROW_BYTES    160u
#define BANK_STRIDE  0x2000u

/* Palette index plan: 0 background, 1..8 water, 9..14 flame, 15 text. */
#define WATER_BASE   1u
#define WATER_N      8u
#define FLAME_BASE   9u
#define FLAME_N      6u
#define TEXT_IDX     15u
#define SCENE_TOP    8u      /* rows 0..7 and 192..199 hold BIOS text */
#define SCENE_BOTTOM 192u
#define HORIZON      100u

/* IRGB colour codes: bit0 blue, bit1 green, bit2 red, bit3 intensity. */
static const unsigned char waterRamp[WATER_N] = {1,1,9,3,11,15,11,3};
static const unsigned char flameRamp[FLAME_N] = {4,12,6,14,12,4};
static const unsigned char waveShift[16] =
    {2,3,4,5,5,5,4,3,2,1,0,0,0,1,1,2};
static const unsigned char flameAmp[4] = {70,92,80,58};

static unsigned rowOffset(unsigned y)
{
    return (unsigned)((y&3u)*BANK_STRIDE + (y>>2)*ROW_BYTES);
}

/* Colour a ramp entry shows after the ramp has rotated by 'phase' steps. */
static unsigned char rampColor(const unsigned char *ramp,unsigned n,
    unsigned i,unsigned phase)
{
    return ramp[(i+phase)%n];
}

/* Height of a flame tongue with concave sides: tall in the middle, thin at
   the tip. 'half' is half the tongue width in pixels. */
static unsigned tongue(unsigned col,unsigned half,unsigned amp)
{
    unsigned d=col<half?half-col:col-half,t;
    if(d>=half)return 0;
    t=half-d;
    return (unsigned)(((unsigned long)amp*t*t)/((unsigned long)half*half));
}

/* Palette index (0..15) of the scene pixel at (x,y). */
static unsigned pixelIndex(unsigned x,unsigned y)
{
    unsigned v,f,g,hgt;
    if(y<SCENE_TOP||y>=SCENE_BOTTOM)return 0;
    if(y<HORIZON) {
        /* Wavy vertical bands; palette rotation makes them slide sideways. */
        v=(x>>2)+waveShift[(y>>2)&15u];
        return WATER_BASE+v%WATER_N;
    }
    hgt=(SCENE_BOTTOM-1u)-y;
    f=tongue(x%64u,32u,flameAmp[(x>>6)%4u]);
    g=tongue((x+20u)%40u,20u,56u);
    if(g>f)f=g;
    if(hgt>=f)return 0;
    return FLAME_BASE+((hgt>>2)+((x>>3)&3u))%FLAME_N;
}

/* Pack one scan line into 160 bytes: two pixels per byte. */
static void sceneRow(unsigned y,unsigned char *row)
{
    unsigned i;
    for(i=0;i<ROW_BYTES;i++)
        row[i]=(unsigned char)((pixelIndex(2u*i,y)<<4)|pixelIndex(2u*i+1u,y));
}

/* Downsample the original 320-wide teaching scene into one 160-wide
   mode 8 frame. No 32K framebuffer or extra frame is needed. */
#define ROW8_BYTES 80u
static unsigned rowOffset8(unsigned y)
{ return (unsigned)((y&1u)*BANK_STRIDE+(y>>1)*ROW8_BYTES); }
static void sceneRow8(unsigned y,unsigned char *row)
{ unsigned i;for(i=0;i<ROW8_BYTES;i++)row[i]=(unsigned char)((pixelIndex(4u*i,y)<<4)|pixelIndex(4u*i+2u,y)); }

/* ---- Demo 2: page flipping in 160x200x16 --------------------------------
 * 160x200 uses two interleaved banks of 8000 bytes, 80 bytes per scan line:
 * scan line y starts at (y&1)*0x2000 + (y>>1)*80. Palette plan: 0 black,
 * 1 bar, 2 grid, 3 tearing marker, 4 clean marker, 15 text.
 */
#define FLIP_TOP     24u
#define FLIP_BOTTOM  176u
#define BAR_W        12u
#define BAR_COLOR    1u
#define GRID_COLOR   2u
#define MARK_TEAR    3u
#define MARK_CLEAN   4u
#define NO_BAR       0xffffu

/* Reference definition of one pixel; flipByte() must agree with it. */
static unsigned flipPixel(unsigned x,unsigned y,unsigned barx)
{
    if(y<FLIP_TOP||y>=FLIP_BOTTOM)return 0;
    if(x>=barx&&x<barx+BAR_W)return BAR_COLOR;
    if(x%20u==0)return GRID_COLOR;
    return 0;
}

/* Background (grid only) byte for byte column bx. */
static unsigned char gridByte(unsigned bx)
{
    return (unsigned char)(((2u*bx)%20u==0)?(GRID_COLOR<<4):0);
}

/* Fast form used by the demo: background byte plus the bar's pixels. */
static unsigned char flipByte(unsigned bx,unsigned barx,unsigned char bg)
{
    unsigned x0=2u*bx;
    unsigned char v=bg;
    if(x0>=barx&&x0<barx+BAR_W)v=(unsigned char)((v&0x0f)|(BAR_COLOR<<4));
    if(x0+1u>=barx&&x0+1u<barx+BAR_W)v=(unsigned char)((v&0xf0)|BAR_COLOR);
    return v;
}

/* Bouncing bar position, 3 pixels per frame, within 0..160-BAR_W. */
static unsigned barPos(unsigned frame)
{
    unsigned span=160u-BAR_W;
    unsigned p=(unsigned)(((unsigned long)frame*3ul)%(2ul*span));
    return p<span?p:2u*span-p;
}

/* ---- Demo 3: retrace palette split in 320x200x16 -------------------------
 * One picture of diagonal blocks uses palette indices 1..SPLIT_N. The demo
 * loads one set of colours in vertical retrace, counts scan lines, and loads
 * another set mid-frame, so the same pixels change colour at the split.
 */
#define SPLIT_N      7u
#define SPLIT_MIN    8u
#define SPLIT_MAX    190u
#define SPLIT_STEP   4u
static const unsigned char splitTop[SPLIT_N]    = {1,9,3,11,9,3,1};
static const unsigned char splitBottom[SPLIT_N] = {4,12,6,14,12,6,4};

static unsigned splitIndex(unsigned x,unsigned y)
{
    if(y<SCENE_TOP||y>=SCENE_BOTTOM)return 0;
    return 1u+((x>>4)+(y>>4))%SPLIT_N;
}
static void splitRow(unsigned y,unsigned char *row)
{
    unsigned i;
    for(i=0;i<ROW_BYTES;i++)
        row[i]=(unsigned char)((splitIndex(2u*i,y)<<4)|splitIndex(2u*i+1u,y));
}
static void splitRow8(unsigned y,unsigned char *row)
{ unsigned i;for(i=0;i<ROW8_BYTES;i++)row[i]=(unsigned char)((splitIndex(4u*i,y)<<4)|splitIndex(4u*i+2u,y)); }
static unsigned moveSplit(unsigned split,int dir)
{
    if(dir<0)return split>SPLIT_MIN+SPLIT_STEP?split-SPLIT_STEP:SPLIT_MIN;
    if(dir>0)return split+SPLIT_STEP<SPLIT_MAX?split+SPLIT_STEP:SPLIT_MAX;
    return split;
}

#endif
