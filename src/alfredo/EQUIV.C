#define HOST_TEST
#include "ALFREDO.C"
#include <time.h>
void reference(int n,int q,unsigned char *out);
static unsigned char golden[32000],host_frame[32768],host_previous[32768],host_video[32768];
static unsigned long checks;
static void check(int n,int q){
    unsigned y,x,k;reference(n,q,golden);draw(n,q);
    for(y=0;y<200;y++)for(x=0;x<160;x++){
        k=(y&3)*8192+(y>>2)*160+x;
        if(frame[k]!=golden[y*160+x]||video[k]!=golden[y*160+x]){
            printf("FAIL scene %d q %d y %u byte %u\n",n,q,y,x);exit(1);
        }
    }
    checks++;
}
int main(void){
    int n,q;unsigned seed=179;clock_t start=clock();
    frame=host_frame;previous=host_previous;video=host_video;
    for(n=0;n<16;n++)for(q=0;q<=1000;q++)check(n,q);
    /* Reverse motion and arbitrary scene/replay jumps must erase old pixels. */
    for(n=15;n>=0;n--)for(q=1000;q>=0;q-=17)check(n,q);
    for(q=0;q<2048;q++){seed=(seed*25173U+13849U)&65535U;check(seed%16,(seed>>4)%1001);}
    if(fallback_frames){printf("FAIL: touch list fallback was used\n");return 1;}
    printf("PASS %lu exact visible framebuffer comparisons; max touched %u; zero fallback.\n",checks,max_touch);
    printf("Host workload: cleared %lu bytes, compared %lu bytes; baseline clear/scan each %lu bytes.\n",clear_bytes,compare_bytes,checks*32000UL);
    printf("Host equivalence/profile seconds %.3f (includes golden rendering and pixel comparison).\n",(double)(clock()-start)/CLOCKS_PER_SEC);
    return 0;
}
