"""Write docs/candidates/alfredo2.json from the exact source, payload and logs.

    python tools/alfredo2_candidate.py

Records sizes and SHA-256 of every file that makes up the candidate, the MZ
allocation bounds of ALFREDO.EXE, and the emulator results read from the
evidence logs. tools/validate_candidates.py re-checks the hashes in CI.
"""
import hashlib, json, struct, subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = sorted(str(p.relative_to(ROOT)) for p in (ROOT / 'src/alfredo2').iterdir() if p.is_file()) + [
    'src/sound/DOSSND.C', 'src/sound/DOSSND.H', 'src/sound/DOSGUARD.H', 'src/sound/win30/DRIVER/PITCORE.H',
    'tools/alfredo2_score.py', 'tools/alfredo2_build_ow.sh', 'tools/alfredo2_cap.py',
    'tools/alfredo2_video.py', 'tools/alfredo2_candidate.py', 'tests/alfredo2_dos.py', 'docs/ALFREDO2.md']
RUNTIME = ['issues/2026-11/BBD2026/ALFREDO/' + n for n in ('ALFREDO.EXE', 'README.TXT', 'LICENSE.TXT')]
EVID = ROOT / 'docs/candidates/alfredo2'


def entry(rel):
    b = (ROOT / rel).read_bytes()
    return {'bytes': len(b), 'sha256': hashlib.sha256(b).hexdigest()}


def kv(path):
    d = {}
    for line in path.read_text().splitlines():
        if line.startswith('scene0') or line.startswith('scene1'):
            continue
        for t in line.split():
            if '=' in t:
                k, v = t.split('=', 1); d[k] = v
    return d


def main():
    exe = (ROOT / RUNTIME[0]).read_bytes()
    h = struct.unpack_from('<14H', exe)
    image = (h[2] - 1) * 512 + (h[1] or 512) - h[4] * 16
    alloc = {'file_bytes': len(exe), 'image_bytes': image, 'minimum_extra_paragraphs': h[5],
             'maximum_extra_paragraphs': h[6], 'far_buffers_bytes': 65536,
             'minimum_total_bytes_excluding_psp_environment': (image + 15) // 16 * 16 + h[5] * 16 + 65536,
             'maximum_total_bytes_excluding_psp_environment': (image + 15) // 16 * 16 + h[6] * 16 + 65536,
             'note': 'MZ bounds plus two 32 KB far buffers from DOS; not a physical memory measurement.'}
    evidence = sorted(str(p.relative_to(ROOT)) for p in EVID.iterdir() if p.is_file())
    logs = {}
    for name in ('QA.TXT', 'AUTO3000.TXT', 'AUTO450.TXT'):
        if (EVID / name).exists():
            d = kv(EVID / name)
            logs[name] = {k: d.get(k) for k in ('pictures', 'frames', 'fluid_steps', 'music_late',
                                                 'music_late_max_fine', 'sfx', 'scenes', 'previous_mode', 'restored_mode')}
    m = {
        'name': 'alfredo2',
        'base_head': subprocess.run(['git', 'rev-parse', 'origin/main'], cwd=ROOT, capture_output=True,
                                    text=True).stdout.strip(),
        'roster_status': 'PROPOSED_ONLY for the November Alfredo slot; October unchanged',
        'mz_allocation': {'ALFREDO': alloc},
        'native': {
            'result': 'PASS',
            'compiler': 'OpenWatcom 2.0 wcl -bt=dos -ms -0 -ox -w4 (tools/alfredo2_build_ow.sh); '
                        'MSC6 BUILD.BAT provided but not yet run',
            'msc6_built': False,
            'machine': 'DOSBox-X machine=tandy, cputype=8086_prefetch, core=normal, memsize=1',
            'checks': 'tests/alfredo2_dos.py: /QA virtual clock 42 stills equal host frames; /AUTO at 3000 and 450 cycles; non-Tandy refusal',
            'physical_qualified': False,
            'logs': logs,
        },
        'limits': ['emulator cycles uncalibrated against a physical Tandy 1000 EX',
                   'no physical speed, memory, CRT or listening qualification',
                   'no MSC6 build yet; the executable is an OpenWatcom 2 build of the same source',
                   'integrated GO launch/return not yet run'],
        'source': {r: entry(r) for r in SOURCE},
        'runtime': {r: entry(r) for r in RUNTIME},
        'evidence': {r: entry(r) for r in evidence},
    }
    (ROOT / 'docs/candidates/alfredo2.json').write_text(json.dumps(m, indent=1) + '\n')
    print('wrote docs/candidates/alfredo2.json:', len(m['source']), 'source,', len(m['runtime']), 'runtime,',
          len(m['evidence']), 'evidence files')


if __name__ == '__main__':
    main()
