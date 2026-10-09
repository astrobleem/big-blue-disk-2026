from pathlib import Path
import json,hashlib,re
ROOT=Path(__file__).resolve().parents[1]
RUNTIME=ROOT/'issues/2026-10/BBD2026'
def digest(b):return hashlib.sha256(b).hexdigest()
def validate():
    manifest=json.loads((ROOT/'docs/runtime-manifest.json').read_text())
    actual={p.relative_to(RUNTIME).as_posix():p for p in RUNTIME.rglob('*') if p.is_file()}
    assert set(actual)==set(manifest['files']) and len(actual)==18
    for n,p in actual.items():
        b=p.read_bytes();e=manifest['files'][n]
        assert len(b)==e['bytes'] and digest(b)==e['sha256'],n
        if p.suffix=='.EXE':assert b[:2]==b'MZ',n
        for part in Path(n).parts:assert re.fullmatch(r'[A-Z0-9_-]{1,8}(\.[A-Z0-9_-]{1,3})?',part),n
    rows=[x.split('|') for x in actual['ISSUE.DAT'].read_text().splitlines() if x and not x.startswith(';')]
    assert [x[0] for x in rows]==['ALFREDO','KROZ','HORSES','DRUGWARS','CAMERA','TANDSND']
    for row in rows:
        assert len(row)==5 and row[4]==''
        assert row[3].replace('\\','/') in actual
    assert len(actual['ARTICLES/CAMERA.TXT'].read_text().split())==844
    assert actual['ARTICLES/CAMERA.TXT'].read_bytes()==(ROOT/'assets/camera/CAMERA.TXT').read_bytes()
    assert len(actual['ARTICLES/CAMERA.CGA'].read_bytes())==16384
    assert actual['SOUND/LICENSE.TXT'].read_bytes()==(ROOT/'src/sound/LICENSE.TXT').read_bytes()==(ROOT/'LICENSE').read_bytes()
    source=json.loads((ROOT/'docs/source-manifest.json').read_text())
    for n,h in source.items():assert digest((ROOT/n).read_bytes())==h,n
    assert (ROOT/'src/sound/win30/DRIVER/PITCORE.H').is_file()
    # Files eligible for publication; build outputs and .git are intentionally excluded.
    roots={'src','assets','issues','docs','tests','tools','.github'}
    rootfiles={'README.md','LICENSE','CREDITS.md','CONTRIBUTING.md','.gitignore','.gitattributes'}
    for p in ROOT.rglob('*'):
        if not p.is_file():continue
        rel=p.relative_to(ROOT);first=rel.parts[0]
        if first in {'build','.git'} or '__pycache__' in rel.parts:continue
        assert (len(rel.parts)==1 and p.name in rootfiles) or first in roots,str(rel)
        assert p.suffix.lower() not in {'.lib','.obj','.map','.dll','.sys','.wav','.mp3','.avi','.zip','.img'},str(rel)
        if p.suffix.upper()=='.EXE':assert first=='issues',str(rel)
        if p.suffix.lower() in {'.md','.txt','.c','.h','.py','.bat','.json','.yml'}:
            b=p.read_bytes()
            for private in [b'C:\\Users\\',b'E:\\budmaker',b'Calpico'+b'File_',b'lib'+b'file_',b'gh'+b'o_']:
                assert private not in b,str(rel)+' private marker'
    print('PASS: 18 exact runtime files, six entries, corresponding-source hashes, public allowlist and license audit.')
    return actual
if __name__=='__main__':validate()
