/* Pure 16-bit solver regressions; no DOS, BIOS, ports or guest needed. */
#include <stdio.h>
#include "FLOWCORE.H"
static unsigned long checks;static int failures;
#define CHECK(x) do {checks++;if(!(x)){printf("FAIL line %d\n",__LINE__);failures++;}}while(0)
int main(void) {
 long raw;int seen[7]={0},i,j,k;Fix a;
 CHECK(sizeof(Fix)==2);CHECK(fix_sat(100000L)==32767);CHECK(fix_sat(-100000L)==-32767);
 CHECK(fix_mul(32767,32767)==32767);CHECK(fix_mul(-32767,32767)==-32767);
 CHECK(fix_abs((Fix)-32768)==32768L);
 for(raw=0;raw<=32767L;raw++){i=density_slot((Fix)raw);CHECK(i>=0&&i<=6);seen[i]=1;}
 for(i=0;i<7;i++)CHECK(seen[i]);CHECK(density_slot(0)==0);CHECK(density_slot(-32767)==0);CHECK(density_slot(1)==1);CHECK(density_slot(32767)==6);
 clear_simulation();u[11][10]=32767;u[9][10]=-32767;v[10][11]=32767;v[10][9]=-32767;
 project(u,v,pressure,div_grid);CHECK(div_grid[10][10]==-32767);
 clear_simulation();dens[5][5]=32767;dens_prev[5][5]=32767;add_source(dens,dens_prev,FIX_ONE);CHECK(dens[5][5]==32767);
 for(k=0;k<12;k++) {
  for(i=0;i<GRID_W;i++)for(j=0;j<GRID_H;j++) {
   a=((i+j+k)&1)?32767:-32767;u[i][j]=a;v[i][j]=(Fix)-a;dens[i][j]=(Fix)(a>0?a:0);
   u_prev[i][j]=a;v_prev[i][j]=(Fix)-a;dens_prev[i][j]=32767;
  }
  load_preset_obstacle(k%3);CHECK(fluid_step(FIX_HALF)>=0);
  for(i=0;i<GRID_W;i++)for(j=0;j<GRID_H;j++) {
   CHECK(u[i][j]>=-32767&&v[i][j]>=-32767&&dens[i][j]>=0);
   CHECK(u_prev[i][j]==0&&v_prev[i][j]==0&&dens_prev[i][j]==0);
   CHECK(density_slot(dens[i][j])>=0&&density_slot(dens[i][j])<=6);
  }
 }
 printf("%s %lu checks, %d failures\n",failures?"FAIL":"PASS",checks,failures);return failures?1:0;
}
