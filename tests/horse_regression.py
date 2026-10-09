"""Prove that the invalid-index regression observes accesses before validation.
The controlled host-only mutant stops at the access hook before touching an
invalid pointer. Never build or run this mutant in DOS or on hardware.
"""
from pathlib import Path
import argparse, subprocess, shutil
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler');p.add_argument('--watcom',action='store_true');a=p.parse_args()
 cc=a.compiler or shutil.which('cc') or shutil.which('gcc');assert cc
 out=ROOT/'build/horse-mutant';out.mkdir(parents=True,exist_ok=True)
 for n in ['HORSE.H','COMMON.H','TRADE.H','TEST.C']:shutil.copyfile(ROOT/'src/fillers-next'/n,out/n)
 h=out/'HORSE.H';s=h.read_text();assert 'rank,slots' in s and 'HORSE_BOUNDS_CHECK(a)' in s
 s=s.replace('int i,ties=0,rank,slots;','int i,ties=0,rank=rankhorse(r,h),slots;').replace('if(h<0||h>=HORSES)return 0;\n    rank=rankhorse(r,h);\n    if(rank>=(place?2:1))return 0;','if(h<0||h>=HORSES||rank>=(place?2:1))return 0;')
 h.write_text(s)
 exe=out/('mutant.exe' if a.watcom else 'mutant')
 cmd=[cc,'-q','-bt=nt','-l=nt','-dHOST','-fe='+str(exe),'TEST.C'] if a.watcom else [cc,'-x','c','-std=c89','-O2','-DHOST','-o',str(exe),'TEST.C']
 subprocess.run(cmd,cwd=out,check=True)
 result=subprocess.run([str(exe)],cwd=out,text=True,capture_output=True)
 assert result.returncode==99 and 'INVALID HORSE MEMORY ACCESS' in result.stdout,result.stdout
 print('PASS: old-order mutant detected invalid access; controlled exit99, no invalid read')
if __name__=='__main__':main()
