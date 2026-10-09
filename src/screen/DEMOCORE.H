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

#endif
