from pathlib import Path
import json,hashlib,zipfile,shutil,sys
import fat12
from validate import validate,ROOT
DATE=(2026,10,1,12,0,0)
def zip_bytes(path,items):
    with zipfile.ZipFile(path,'w',zipfile.ZIP_DEFLATED) as z:
        for name,b in sorted(items.items()):
            i=zipfile.ZipInfo(name,DATE);i.create_system=0;i.compress_type=zipfile.ZIP_DEFLATED;z.writestr(i,b)
    with zipfile.ZipFile(path) as z:assert z.testzip() is None
def main():
    actual=validate();work=ROOT/'build';work.mkdir(exist_ok=True)
    rt=work/'runtime/BBD2026';rt.mkdir(parents=True,exist_ok=True)
    for n,p in actual.items():
        dest=rt/n;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(p.read_bytes())
    # A stale diagnostic must never silently enter a release.
    assert {p.relative_to(rt).as_posix() for p in rt.rglob('*') if p.is_file()}==set(actual)
    fat12.HERE=work;fat12.build()
    payload=fat12.read_image(work/'ISSUE26.IMG')
    assert set(payload)=={'BBD2026/'+n for n in actual}
    for n,p in actual.items():assert payload['BBD2026/'+n]==p.read_bytes()
    release=work/'release';release.mkdir(exist_ok=True)
    shutil.copyfile(work/'ISSUE26.IMG',release/'BBD2610.IMG')
    zip_bytes(release/'BBD2610.ZIP',payload)
    source={}
    for p in ROOT.rglob('*'):
        if not p.is_file():continue
        rel=p.relative_to(ROOT);parts=rel.parts
        if parts[0] in {'build','.git'} or '__pycache__' in parts or p.suffix.upper()=='.EXE':continue
        source[rel.as_posix()]=p.read_bytes()
    zip_bytes(release/'SOURCE26.ZIP',source)
    instructions='''BIG BLUE DISK 2026 -- OCTOBER 2026, ISSUE 1 (PUBLIC PREVIEW)
Unofficial revival; original new content, not historical programs.

BBD2610.IMG: 720KB FAT12 DOS data disk, not an operating system.
BBD2610.ZIP: ready-to-run BBD2026 folder. Boot your own DOS, CD BBD2026,
then GO. Arrows/1-6 select, Enter opens, Esc exits. Camera article: D diagram.
Kroz: arrows/numpad move, W whip, Space grenade, R retry, N skip, Esc return.
See individual PLAY/START articles for controls. Target: 8088 Tandy1000 EX,
640KB installed RAM. Tandy graphics/sound are required by relevant pieces.
No OS, compiler, installed driver changes or private media included.

SOURCE26.ZIP: complete corresponding source, assets, notices and build recipes.
GPLv3. Browsable source and credits:
https://github.com/astrobleem/big-blue-disk-2026

Host/native checks passed. Expanded Kroz physical playthrough is pending;
earlier Grenade Garden hardware feedback does not qualify these changes.
Emulator tests do not establish physical speed, memory, smoothness or listening.
SHA256SUMS identifies these exact release downloads.
'''
    (release/'READTHIS.TXT').write_bytes(instructions.replace('\n','\r\n').encode('ascii'))
    names=['BBD2610.IMG','BBD2610.ZIP','SOURCE26.ZIP','READTHIS.TXT']
    hashes={n:hashlib.sha256((release/n).read_bytes()).hexdigest() for n in names}
    (release/'SHA256SUMS').write_bytes(''.join(h+'  '+n+'\n' for n,h in hashes.items()).encode('ascii'))
    assert (release/'BBD2610.IMG').stat().st_size==737280
    assert fat12.read_image(release/'BBD2610.IMG')==payload
    with zipfile.ZipFile(release/'BBD2610.ZIP') as z:assert {n:z.read(n) for n in z.namelist()}==payload
    print('PASS exact public FAT12/folder payload and companion packages:',json.dumps(hashes,indent=2))
if __name__=='__main__':main()
