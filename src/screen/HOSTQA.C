/* Host checks for DEMOCORE.H. Build with any C compiler; no hardware access. */
#include <stdio.h>
#include <string.h>
#include "DEMOCORE.H"
static int checks,failures;
#define CHECK(x) do {++checks;if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);++failures;}}while(0)
static unsigned char cover[0x8000];
int main(void)
{
    unsigned y,x,i,p,n;
    unsigned char row[ROW_BYTES];
    unsigned seen[16];
    /* Scan-line layout: 200 disjoint 160-byte spans inside the 4 banks. */
    memset(cover,0,sizeof cover);
    for(y=0;y<SCR_H;y++) {
        unsigned o=rowOffset(y);
        CHECK(o+ROW_BYTES<=0x7f40u);
        CHECK((o&0x1fffu)<8000u);
        for(i=0;i<ROW_BYTES;i++){CHECK(!cover[o+i]);cover[o+i]=1;}
    }
    n=0;for(i=0;i<sizeof cover;i++)n+=cover[i];
    CHECK(n==SCR_H*ROW_BYTES);
    CHECK(rowOffset(0)==0&&rowOffset(1)==0x2000u&&rowOffset(2)==0x4000u);
    CHECK(rowOffset(3)==0x6000u&&rowOffset(4)==160u&&rowOffset(199)==0x6000u+49u*160u);
    /* Pixel indices stay in their reserved ranges. */
    memset(seen,0,sizeof seen);
    for(y=0;y<SCR_H;y++)for(x=0;x<SCR_W;x++) {
        p=pixelIndex(x,y);CHECK(p<16u);seen[p]++;
        if(y<SCENE_TOP||y>=SCENE_BOTTOM)CHECK(p==0u);
        else if(y<HORIZON)CHECK(p>=WATER_BASE&&p<WATER_BASE+WATER_N);
        else CHECK(p==0u||(p>=FLAME_BASE&&p<FLAME_BASE+FLAME_N));
        CHECK(p!=TEXT_IDX);
    }
    for(i=WATER_BASE;i<WATER_BASE+WATER_N;i++)CHECK(seen[i]>0);
    for(i=FLAME_BASE;i<FLAME_BASE+FLAME_N;i++)CHECK(seen[i]>0);
    /* Packing: first pixel is the high nibble. */
    for(y=0;y<SCR_H;y+=17) {
        sceneRow(y,row);
        for(i=0;i<ROW_BYTES;i++){
            CHECK((row[i]>>4)==pixelIndex(2u*i,y));
            CHECK((row[i]&15u)==pixelIndex(2u*i+1u,y));
        }
    }
    /* Rotation: periodic, a permutation, and identity at phase 0 and n. */
    for(i=0;i<WATER_N;i++){
        CHECK(rampColor(waterRamp,WATER_N,i,0)==waterRamp[i]);
        CHECK(rampColor(waterRamp,WATER_N,i,WATER_N)==waterRamp[i]);
        CHECK(rampColor(waterRamp,WATER_N,i,3)==waterRamp[(i+3)%WATER_N]);
        CHECK(waterRamp[i]<16u);
    }
    for(i=0;i<FLAME_N;i++){
        CHECK(rampColor(flameRamp,FLAME_N,i,0)==flameRamp[i]);
        CHECK(rampColor(flameRamp,FLAME_N,i,FLAME_N)==flameRamp[i]);
        CHECK(flameRamp[i]<16u);
    }
    printf("%s %d checks, %d failures\n",failures?"FAIL":"PASS",checks,failures);
    return failures?1:0;
}
