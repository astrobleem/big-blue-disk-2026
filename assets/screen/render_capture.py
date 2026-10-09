"""Decode exact native mode08h VRAM samples. Not a beam/CRT screenshot."""
from pathlib import Path
import argparse
from PIL import Image
IRGB=[(0,0,0),(0,0,170),(0,170,0),(0,170,170),(170,0,0),(170,0,170),(170,85,0),(170,170,170),(85,85,85),(85,85,255),(85,255,85),(85,255,255),(255,85,85),(255,85,255),(255,255,85),(255,255,255)]
def main():
 p=argparse.ArgumentParser();p.add_argument('raw');p.add_argument('output');p.add_argument('--palette');a=p.parse_args()
 b=Path(a.raw).read_bytes();assert len(b)==16384
 palette=Path(a.palette).read_bytes() if a.palette else bytes(range(16));assert len(palette)==16 and max(palette)<16
 im=Image.new('P',(160,200));im.putpalette([v for c in IRGB for v in c]+[0]*720)
 px=im.load()
 for y in range(200):
  o=(y&1)*8192+(y>>1)*80
  for x in range(160):
   v=b[o+(x>>1)];idx=v&15 if x&1 else v>>4;px[x,y]=palette[idx]
 im.resize((320,200),Image.Resampling.NEAREST).save(a.output)
if __name__=='__main__':main()
