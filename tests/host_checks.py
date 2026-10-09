from pathlib import Path
import argparse,subprocess,shutil,json
ROOT=Path(__file__).resolve().parents[1]
def main():
    parser=argparse.ArgumentParser();parser.add_argument('--compiler');parser.add_argument('--watcom',action='store_true');args=parser.parse_args()
    cc=args.compiler or shutil.which('cc') or shutil.which('gcc')
    if not cc:raise SystemExit('A host C compiler is required; it is not bundled.')
    out=ROOT/'build/host';out.mkdir(parents=True,exist_ok=True)
    suites=[('reader',['CORE.C','TEST.C']),('kroz',['GAME.C','TEST.C']),('fillers',['TEST.C']),('fillers-next',['TEST.C']),('alfredo',['EQUIV.C','REFERENCE.C']),('flow88',['FLOWQA.C']),('screen',['HOSTQA.C'])]
    results={}
    for name,files in suites:
        exe=out/(name+('.exe' if args.watcom else ''))
        sources=[str(ROOT/'src'/name/f) for f in files]
        if args.watcom:cmd=[cc,'-q','-bt=nt','-l=nt','-fe='+str(exe)]+sources
        else:cmd=[cc,'-x','c','-std=c89','-O2','-o',str(exe)]+sources
        if name in ('fillers','fillers-next'):cmd.insert(1,'-dHOST' if args.watcom else '-DHOST')
        subprocess.run(cmd,cwd=out,check=True)
        result=subprocess.run([str(exe)],cwd=out,check=True,text=True,capture_output=True)
        if 'PASS' not in result.stdout:raise AssertionError(name)
        results[name]=result.stdout.strip();print(name+': '+result.stdout.strip(),flush=True)
    (out/'results.json').write_text(json.dumps(results,indent=2))
if __name__=='__main__':main()
