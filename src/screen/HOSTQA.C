/* Host checks for DEMOCORE.H. Build with any C compiler; no hardware access. */
#include <stdio.h>
#include <string.h>
#include "DEMOCORE.H"
static long checks;static int failures;
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
            CHECK(((unsigned)row[i]>>4)==pixelIndex(2u*i,y));
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
    /* Demo 2: 160x200 layout, bar bounce and byte/pixel agreement. */
    memset(cover,0,sizeof cover);
    for(y=0;y<SCR_H;y++) {
        unsigned o=rowOffset8(y);
        CHECK(o+ROW8_BYTES<=0x3f40u);
        CHECK((o&0x1fffu)<8000u);
        for(i=0;i<ROW8_BYTES;i++){CHECK(!cover[o+i]);cover[o+i]=1;}
    }
    n=0;for(i=0;i<sizeof cover;i++)n+=cover[i];
    CHECK(n==SCR_H*ROW8_BYTES);
    CHECK(rowOffset8(0)==0&&rowOffset8(1)==0x2000u&&rowOffset8(2)==80u);
    {
        unsigned prev=barPos(0),f,bx,bar;
        CHECK(prev==0);
        for(f=1;f<2000;f++){
            bar=barPos(f);
            CHECK(bar<=160u-BAR_W);
            CHECK((bar>prev?bar-prev:prev-bar)<=3u);
            prev=bar;
        }
        for(bar=0;bar<=160u-BAR_W;bar+=1)
            for(y=FLIP_TOP;y<FLIP_BOTTOM;y+=7)
                for(bx=0;bx<ROW8_BYTES;bx++) {
                    unsigned char b=flipByte(bx,bar,gridByte(bx));
                    CHECK(((unsigned)b>>4)==flipPixel(2u*bx,y,bar));
                    CHECK((b&15u)==flipPixel(2u*bx+1u,y,bar));
                }
        for(bx=0;bx<ROW8_BYTES;bx++){
            unsigned char b=flipByte(bx,NO_BAR,gridByte(bx));
            CHECK(((unsigned)b>>4)==flipPixel(2u*bx,100,NO_BAR));
            CHECK((b&15u)==flipPixel(2u*bx+1u,100,NO_BAR));
        }
    }
    /* Demo 3: indices stay inside the palette entries the split reloads. */
    {
        unsigned k,sp=SPLIT_MIN,hit[SPLIT_N+1];
        memset(hit,0,sizeof hit);
        for(y=0;y<SCR_H;y++)for(x=0;x<SCR_W;x++){
            p=splitIndex(x,y);
            CHECK(p<=SPLIT_N);
            if(y<SCENE_TOP||y>=SCENE_BOTTOM)CHECK(p==0u);else CHECK(p>=1u);
            hit[p]++;
        }
        for(k=1;k<=SPLIT_N;k++)CHECK(hit[k]>0);
        for(y=0;y<SCR_H;y+=13){
            splitRow(y,row);
            for(i=0;i<ROW_BYTES;i++){
                CHECK(((unsigned)row[i]>>4)==splitIndex(2u*i,y));
                CHECK((row[i]&15u)==splitIndex(2u*i+1u,y));
            }
        }
        for(k=0;k<SPLIT_N;k++){CHECK(splitTop[k]<16u&&splitBottom[k]<16u);CHECK(splitTop[k]!=splitBottom[k]);}
        for(k=0;k<200;k++){sp=moveSplit(sp,1);CHECK(sp>=SPLIT_MIN&&sp<=SPLIT_MAX);}
        CHECK(sp==SPLIT_MAX);
        for(k=0;k<200;k++){sp=moveSplit(sp,-1);CHECK(sp>=SPLIT_MIN&&sp<=SPLIT_MAX);}
        CHECK(sp==SPLIT_MIN);
        CHECK(moveSplit(100,0)==100);
    }
    for(y=0;y<SCR_H;y++) {
        sceneRow8(y,row);for(i=0;i<ROW8_BYTES;i++) {
            CHECK(((unsigned)row[i]>>4)==pixelIndex(4u*i,y));CHECK((row[i]&15u)==pixelIndex(4u*i+2u,y));
        }
        splitRow8(y,row);for(i=0;i<ROW8_BYTES;i++) {
            CHECK(((unsigned)row[i]>>4)==splitIndex(4u*i,y));CHECK((row[i]&15u)==splitIndex(4u*i+2u,y));
        }
    }
    printf("%s %ld checks, %d failures\n",failures?"FAIL":"PASS",checks,failures);
    return failures?1:0;
}
