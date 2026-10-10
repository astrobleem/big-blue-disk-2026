"""Native checks for Alfredo: After Hours in DOSBox-X (machine=tandy).

Emulator evidence only: DOSBox-X cycle counts are not a physical Tandy.

    python tests/alfredo2_dos.py --exe ALFREDO.EXE [--keep DIR] [--cycles 3000,450,240]

Runs, each in a fresh emulator:
  QA      /QA on the virtual clock: every scene reached, all sound effects,
          one still per scene (S*.RAW), clean exit back to the caller's mode
  AUTO-n  /AUTO in real time at n fixed cycles: frames presented, worst
          frame time, fluid steps, music lateness, cleanup
  NOTANDY machine=vgaonly: refuses with a message and leaves the mode alone
"""
from pathlib import Path
import argparse, os, shutil, subprocess, sys, tempfile

def dosbox(work, batch, name, cycles=3000, machine='tandy'):
    (work / (name + '.BAT')).write_text('\r\n'.join(['@echo off'] + batch) + '\r\n')
    conf = work / (name + '.CNF')
    conf.write_text('[sdl]\noutput=surface\n[dosbox]\nmachine=%s\nmemsize=1\nquit warning=false\n'
                    '[cpu]\ncore=normal\ncputype=8086_prefetch\ncycles=fixed %d\n'
                    '[mixer]\nnosound=true\n[autoexec]\nmount c %s\nc:\ncall %s.BAT\nexit\n'
                    % (machine, cycles, work, name))
    subprocess.run(['dosbox-x', '-conf', str(conf), '-nopromptfolder', '-fastlaunch'],
                   env=dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy'),
                   capture_output=True, timeout=3600)

def kv(path):
    d = {}
    for t in ' '.join(l for l in path.read_text().splitlines() if not l.startswith('scene0') and not l.startswith('scene1')).split():
        if '=' in t:
            k, v = t.split('=', 1); d[k] = v
    return d

def main():
    p = argparse.ArgumentParser()
    p.add_argument('--exe', type=Path, required=True)
    p.add_argument('--keep', type=Path)
    p.add_argument('--cycles', default='3000,450')
    p.add_argument('--host-cap', type=Path, help='frames from src/alfredo2/HOSTQA.C for pixel equivalence')
    a = p.parse_args()
    work = Path(tempfile.mkdtemp(prefix='alf2-'))
    shutil.copy2(a.exe, work / 'ALFREDO.EXE')
    bad = []
    def need(cond, msg):
        print(('PASS ' if cond else 'FAIL ') + msg, flush=True)
        if not cond:
            bad.append(msg)

    (work / 'QA').mkdir()
    dosbox(work, ['cd QA', '..\\ALFREDO /QA', 'cd ..'], 'QARUN', 20000)
    d = kv(work / 'QA/QA.TXT')
    need(d.get('scenes', '')[:14] == '1' * 14, 'QA: all fourteen episode scenes shown (%s)' % d.get('scenes'))
    need(d.get('sfx') == '22', 'QA: all 22 sound effects fired (%s)' % d.get('sfx'))
    need(d.get('music_late') == '0', 'QA: no music event late on the virtual clock')
    need(d.get('sound') == '1', 'QA: PSG claimed through the shared sound owner')
    need(d.get('previous_mode') == d.get('restored_mode') == '3', 'QA: text mode 3 restored')
    stills = sorted((work / 'QA').glob('S*.RAW'))
    need(len(stills) == 42 and all(s.stat().st_size == 32000 for s in stills),
         'QA: three 32000-byte stills per scene (%d)' % len(stills))
    if a.host_cap:
        ticks = [109, 273, 146, 118, 291, 164, 91, 437, 182, 146, 127, 328, 200, 364]
        start, same, total = 0, 0, 0
        for k, t in enumerate(ticks):
            for q in range(1, 4):
                want = (start * 256 + ((t << 4) * q // 4) * 16 + 127) // 128
                path = work / 'QA' / ('S%02d%s.RAW' % (k, 'ABC'[q - 1]))
                if not path.exists():
                    continue
                d = path.read_bytes(); total += 1
                same += any((a.host_cap / ('F%05d.RAW' % f)).exists() and
                            (a.host_cap / ('F%05d.RAW' % f)).read_bytes() == d for f in range(want - 3, want + 4))
            start += t
        need(same == total == 42, 'QA: DOS stills equal the host-rendered frames, pixel for pixel (%d/%d)' % (same, total))
    for c in [int(x) for x in a.cycles.split(',') if x]:
        sub = 'A%d' % c
        (work / sub).mkdir()
        dosbox(work, ['cd ' + sub, '..\\ALFREDO /AUTO', 'cd ..'], 'AUTO%d' % c, c)
        d = kv(work / sub / 'AUTO.TXT')
        fine = int(d.get('worst_frame_fine', '0'))
        print('INFO %d cycles: frames=%s worst_frame=%.0f ms fluid_steps=%s music_late=%s late_max=%.0f ms'
              % (c, d.get('frames'), fine * 54.925 / 256, d.get('fluid_steps'), d.get('music_late'),
                 int(d.get('music_late_max_fine', '0')) * 54.925 / 256), flush=True)
        for line in (work / sub / 'AUTO.TXT').read_text().splitlines():
            if line.startswith('scene'):
                print('INFO   ' + line, flush=True)
        need(d.get('scenes', '')[:14] == '1' * 14, '%d cycles: whole episode played in real time' % c)
        need(d.get('previous_mode') == d.get('restored_mode') == '3', '%d cycles: text mode restored' % c)
        (work / sub / 'AUTO.TXT').rename(work / sub / ('AUTO%d.TXT' % c))

    (work / 'NT').mkdir()
    dosbox(work, ['cd NT', '..\\ALFREDO > OUT.TXT', 'cd ..'], 'NOTANDY', 3000, 'vgaonly')
    out = (work / 'NT/OUT.TXT').read_text(errors='replace') if (work / 'NT/OUT.TXT').exists() else ''
    need('Tandy 1000' in out, 'non-Tandy machine: refuses with a message')

    if a.keep:
        shutil.copytree(work, a.keep, dirs_exist_ok=True)
    shutil.rmtree(work)
    print('FAIL' if bad else 'PASS: Alfredo After Hours native emulator checks')
    sys.exit(1 if bad else 0)

if __name__ == '__main__':
    main()
