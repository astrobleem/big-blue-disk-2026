"""Audit proposed contributions separately from the frozen October issue.
This checks recorded bytes and source closure, not native execution or hardware.
"""
from pathlib import Path
import json, hashlib, struct
ROOT=Path(__file__).resolve().parents[1]
def main():
    manifests=sorted((ROOT/'docs/candidates').glob('*.json'))
    if not manifests: raise AssertionError('No candidate manifests')
    for p in manifests:
        m=json.loads(p.read_text()); assert m['native']['result']=='PASS'
        assert m['native']['physical_qualified'] is False
        for section in ['source','runtime','evidence']:
            for name,e in m[section].items():
                rel=Path(name);assert not rel.is_absolute() and '..' not in rel.parts
                b=(ROOT/rel).read_bytes()
                assert len(b)==e['bytes'] and hashlib.sha256(b).hexdigest()==e['sha256'],name
                if rel.suffix.upper()=='.EXE':
                    h=struct.unpack_from('<14H',b);assert b[:2]==b'MZ' and h[6]<=4096,name
        print('PASS candidate',m['name'],'exact source/runtime/evidence; emulator only')
if __name__=='__main__':main()
