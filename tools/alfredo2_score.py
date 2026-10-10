"""Soundtrack source for "Alfredo: After Hours" -> src/alfredo2/MUSIC.H.

Original music written for this episode. Each cue is a sixteenth-note grid
of three PSG tone voices (0 lead, 1 bass, 2 harmony/arp; voice 2 is lent to
sound effects) plus noise-channel drums. One token per sixteenth:

    c5 d#4 bb3   start a note        -   hold        .   silence

    python tools/alfredo2_score.py           write MUSIC.H
    python tools/alfredo2_score.py --check   fail if MUSIC.H is stale
"""
import argparse, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'src/alfredo2/MUSIC.H'
TICK_HZ = 1193182 / 65536          # BIOS ticks per second
FINE_PER_SEC = TICK_HZ * 256

# att0, decay (1/16 tick per attenuation step, 0 none), floor, release, vibrato
INSTS = [
    ('LEAD',     1, 7, 6, 2, 0),
    ('BASS',     0, 9, 5, 1, 0),
    ('PAD',      7, 0, 7, 6, 0),
    ('ARP',      4, 3, 15, 1, 0),
    ('TROMBONE', 2, 0, 2, 5, 9),
    ('WHISTLE',  3, 0, 3, 3, 3),
    ('BELL',     3, 9, 15, 4, 0),
    ('STAB',     1, 4, 15, 1, 0),
    ('SOFTBASS', 3, 12, 8, 3, 0),
    ('BRASS',    1, 0, 1, 2, 2),
]
I = {n[0]: k for k, n in enumerate(INSTS)}
DRUMS = {'K': 0, 'S': 1, 'H': 2, 'O': 3, 'C': 4, 'L': 5, 'T': 6}
NOTE = {'c': 0, 'd': 2, 'e': 4, 'f': 5, 'g': 7, 'a': 9, 'b': 11}


def midi(tok):
    n = NOTE[tok[0]]
    rest = tok[1:]
    while rest and rest[0] in '#b':
        n += 1 if rest[0] == '#' else -1
        rest = rest[1:]
    m = n + 12 * (int(rest) + 1)
    if not 45 <= m <= 96:
        raise ValueError(f'{tok} outside the PSG range A2..C7')
    return m


class Cue:
    def __init__(self, name, bpm, loop=False):
        self.name, self.bpm, self.loop = name, bpm, loop
        self.step = round(FINE_PER_SEC * 60 / bpm / 4)
        self.ev = {}       # step -> list of (order, bytes)
        self.end = 0

    def add(self, step, order, data):
        self.ev.setdefault(step, []).append((order, data))
        self.end = max(self.end, step + 1)

    def inst(self, voice, step, name):
        self.add(step, 0, [0x50 + voice, I[name]])

    def voice(self, voice, at, pattern, inst=None):
        """Lay a token string on voice from step `at`; returns the next step."""
        if inst:
            self.inst(voice, at, inst)
        toks = pattern.split()
        on = False
        for k, t in enumerate(toks):
            s = at + k
            if t == '-':
                continue
            if on:
                self.add(s, 1, [0x48 + voice])
                on = False
            if t != '.':
                self.add(s, 2, [0x40 + voice, midi(t)])
                on = True
        if on:
            self.add(at + len(toks), 1, [0x48 + voice])
        self.end = max(self.end, at + len(toks))
        return at + len(toks)

    def drums(self, at, pattern):
        for k, t in enumerate(pattern.replace(' ', '')):
            if t != '.':
                self.add(at + k, 3, [0x58, DRUMS[t]])
        self.end = max(self.end, at + len(pattern.replace(' ', '')))

    def length(self, steps):
        self.end = max(self.end, steps)

    def encode(self):
        out, last = [], 0
        for s in sorted(self.ev):
            gap = s - last
            while gap > 0:
                n = min(gap, 64)
                out.append(n - 1)
                gap -= n
            for _, d in sorted(self.ev[s], key=lambda e: e[0]):
                out += d
            last = s
        gap = self.end - last
        while gap > 0:
            n = min(gap, 64)
            out.append(n - 1)
            gap -= n
        out.append(0x60 if self.loop else 0x61)
        return out

    def seconds(self):
        return self.end * self.step / FINE_PER_SEC


def rep(s, n):
    return ' '.join([s] * n)


def chord_arp(notes, steps=16):
    return ' '.join(notes[k % len(notes)] for k in range(steps))


def pump(lo, hi):
    """Offbeat octave bass, one bar."""
    return f'{lo} - {hi} . {lo} - {hi} . {lo} - {hi} . {lo} - {hi} .'


# ---------------------------------------------------------------- cues
def sting():
    c = Cue('STING', 120)
    c.voice(0, 0, 'c5 - e5 - g5 - c6 - - - - - - - - - - - - - - - - - - - - - - - - - - .', 'BRASS')
    c.voice(1, 0, '. . . . . . c3 - - - - - - - - - g3 - - - c3 - - - - - - - - - - .', 'BASS')
    c.voice(2, 0, '. . . . . . e5 - - - - - - - - - f5 - - - g5 - - - - - - - - - - .', 'PAD')
    c.drums(0, 'C............... ....T.L.C.......')
    c.voice(0, 32, 'g5 - e5 - c5 - g4 - c5 - - - - - - .', 'ARP')
    c.length(48)
    return c


RAVE_LEAD = [
    'e5 . e5 . d5 . c5 . d5 - e5 - . . a4 .',
    'c5 . c5 . d5 . e5 . f5 - e5 - d5 - c5 -',
    'g5 . g5 . e5 . c5 . d5 - e5 - . . c5 .',
    'b4 - - . d5 - - . g5 - - - f5 - e5 -',
]
RAVE_CHORDS = [('a2', 'a3', ['a4', 'c5', 'e5', 'a5']),
               ('f3', 'f4', ['f4', 'a4', 'c5', 'f5']),
               ('c3', 'c4', ['c5', 'e5', 'g5', 'c6']),
               ('g3', 'g4', ['g4', 'b4', 'd5', 'g5'])]
BEAT = 'K.H.S.H.K.H.S.HO'


def rave(name='RAVE', full=False):
    c = Cue(name, 128, loop=full)
    for bar in range(8):
        lo, hi, arp = RAVE_CHORDS[bar % 4]
        at = bar * 16
        c.voice(1, at, pump(lo, hi), 'BASS' if bar == 0 else None)
        c.drums(at, BEAT if (bar or full) else 'K...K...K...K.H.')
        if full or bar >= 2:
            c.voice(2, at, chord_arp(arp), 'ARP' if bar in (0, 2) else None)
        if full or bar >= 4:
            c.voice(0, at, RAVE_LEAD[bar % 4], 'LEAD' if bar in (0, 4) else None)
    c.length(128)
    return c


def lights():
    c = Cue('LIGHTS', 60)
    c.voice(0, 0, 'e5 - - - - - - - d5 - - - - - - - c5 - - - - - - - - - - - - - - .', 'PAD')
    c.voice(1, 0, 'f3 - - - - - - - e3 - - - - - - - a2 - - - - - - - - - - - - - - .', 'SOFTBASS')
    c.voice(2, 8, 'a4 - - - - - - - g4 - - - - - - - - - - - - - - .', 'PAD')
    c.length(32)
    return c


def punch():
    c = Cue('PUNCH', 120)
    c.drums(0, 'T.L.T.L.T.L.T.L. T.L.T.L.T.L.T.L.')
    c.voice(0, 0, 'c5 . e5 . g5 . e5 . c5 . e5 . g5 - - . a5 . g5 . e5 . d5 . c5 - - - - .', 'BELL')
    c.voice(1, 0, rep('c3 . . . g2 . . .', 2).replace('g2', 'g3') + ' ' + rep('f3 . . . g3 . . .', 2), 'SOFTBASS')
    # Ka-chunk lands at step 36; then "ding ding".
    c.voice(0, 40, 'b5 - c6 - - - - - - - - .', 'BELL')
    c.length(52)
    return c


WALK_LEAD = [
    'e5 - - g5 - - b5 - a5 - g5 - e5 - - -',
    'c5 - - e5 - - g5 - e5 - d5 - c5 - - -',
    'd5 - - f5 - - a5 - g5 - f5 - d5 - e5 -',
    'f5 - - e5 - - d5 - b4 - - - g4 - - .',
]
WALK_BASS = ['c3 - - - e3 - - - g3 - - - e3 - - -',
             'a2 - - - c3 - - - e3 - - - c3 - - -',
             'd3 - - - f3 - - - a3 - - - f3 - - -',
             'g3 - - - f3 - - - d3 - - - b2 - - -']
WALK_STAB = ['. . e4 . . . g4 . . . b4 . . . g4 .',
             '. . c5 . . . e5 . . . g4 . . . e5 .',
             '. . f4 . . . a4 . . . c5 . . . a4 .',
             '. . b4 . . . d5 . . . f5 . . . d5 .']


def walk():
    c = Cue('WALK', 84, loop=True)
    for bar in range(8):
        at = bar * 16
        c.voice(1, at, WALK_BASS[bar % 4], 'SOFTBASS' if bar == 0 else None)
        c.voice(2, at, WALK_STAB[bar % 4], 'STAB' if bar == 0 else None)
        if bar >= 2:
            c.voice(0, at, WALK_LEAD[bar % 4], 'WHISTLE' if bar == 2 else None)
        c.drums(at, 'K.......S.K.....' if bar % 2 == 0 else 'K.......S...K.H.')
    c.length(128)
    return c


def pond():
    c = Cue('POND', 70, loop=True)
    chords = [('f3', 'a4', 'f5 . . . c6 . . . a5 . . . . . . .'),
              ('e3', 'g4', '. . . . e5 . . . . . g5 . . . c6 .'),
              ('d3', 'f4', 'd5 . . . . . a5 . . . f5 . . . . .'),
              ('a#2', 'f4', '. . d6 . . . . . a#5 . . . f5 . . .')]
    for bar in range(4):
        root, fifth, bell = chords[bar]
        at = bar * 16
        c.voice(1, at, root + ' ' + ' '.join(['-'] * 14) + ' .', 'SOFTBASS' if bar == 0 else None)
        c.voice(2, at, fifth + ' ' + ' '.join(['-'] * 14) + ' .', 'PAD' if bar == 0 else None)
        c.voice(0, at, bell, 'BELL' if bar == 0 else None)
    c.length(64)
    return c


def tension():
    c = Cue('TENSION', 140, loop=True)
    leads = ['e5 - f5 - f#5 - g5 - g#5 - a5 - a#5 - b5 -',
             'c6 - b5 - a#5 - b5 - c6 - c#6 - d6 - d#6 -',
             'e6 - - - b5 - - - e6 - - - b5 - - -',
             'e6 . e6 . e6 . d6 . c6 . b5 . a#5 . b5 .']
    for bar in range(4):
        at = bar * 16
        c.voice(1, at, rep('e3 e3 e4 e3', 4), 'STAB' if bar == 0 else None)
        c.voice(2, at, '. . g4 . . . a#4 . . . g4 . . . b4 .', 'ARP' if bar == 0 else None)
        c.voice(0, at, leads[bar], 'LEAD' if bar == 0 else None)
        c.drums(at, 'L.T.L.T.K.S.L.TT' if bar % 2 else 'K.T.L.T.K.S.SSSS')
    c.length(64)
    return c


def fanfare():
    c = Cue('FANFARE', 120)
    c.voice(0, 0, 'g4 c5 e5 g5 - - e5 g5 c6 - - - - - - - a5 - - - b5 - - - c6 - - - - - - - - - - - - - - - - - - - - - - .', 'BRASS')
    c.voice(1, 0, 'c3 - - - - - - - - - - - - - - - f3 - - - g3 - - - c3 - - - - - - - - - - - - - - - - - - - - - - .', 'BASS')
    c.voice(2, 0, 'e4 g4 c5 e5 - - c5 e5 g5 - - - - - - - f5 - - - f5 - - - e5 - - - - - - - - - - - - - - - - - - - - - - .', 'PAD')
    c.drums(0, 'C.......K.S.S.S. C...K...C.......')
    c.length(64)
    return c


def alarm():
    c = Cue('ALARM', 100, loop=True)
    for bar in range(2):
        at = bar * 16
        c.voice(1, at, 'a2 . a#2 . a2 . a#2 . a2 . a#2 . a2 . c3 .', 'STAB' if bar == 0 else None)
        c.voice(0, at, 'e6 - - - - - - - f6 - - - - - - -' if bar == 0 else 'e6 - - - - - - - d#6 - - - - - - -', 'PAD' if bar == 0 else None)
        c.drums(at, 'K..K....K..K....')
    c.length(32)
    return c


def surge():
    c = Cue('SURGE', 150, loop=True)
    runs = ['d6 c#6 c6 b5 a#5 a5 g#5 g5 f#5 f5 e5 d#5 d5 - - .',
            'a5 . a5 . g#5 . g5 . f#5 - f5 - e5 - d#5 -',
            'd6 c#6 c6 b5 a#5 a5 g#5 g5 f#5 f5 e5 d#5 d5 c#5 c5 b4',
            'a#4 - - - . . . . d5 - - - c#5 - - -']
    for bar in range(4):
        at = bar * 16
        c.voice(1, at, rep('d3 d3 f3 d3 g#3 d3 a3 d3', 2), 'STAB' if bar == 0 else None)
        c.voice(0, at, runs[bar], 'LEAD' if bar == 0 else None)
        c.voice(2, at, '. . . . a4 . . . . . . . g#4 . . .', 'STAB' if bar == 0 else None)
        c.drums(at, 'K.H.S.HKK.HKS.HS' if bar % 2 else 'K.H.S.H.K.KHS.HO')
    c.length(64)
    return c


def trombone():
    c = Cue('TROMBONE', 60)
    c.voice(0, 8, 'g4 - - . f#4 - - . f4 - - . e4 - - - - - - - - - - - - - .', 'TROMBONE')
    c.voice(1, 8, 'g3 - - . f#3 - - . f3 - - . e3 - - - - - - - - - - - - - .', 'TROMBONE')
    c.length(44)
    return c


def credits():
    return rave('CREDITS', full=True)


CUES = [sting(), rave(), lights(), punch(), walk(), pond(), tension(), fanfare(),
        alarm(), surge(), trombone(), credits()]


def generate():
    lines = ['/* Generated by tools/alfredo2_score.py: original soundtrack. Do not edit. */',
             '/* att0, decay, floor, release, vibrato */',
             'static const Inst inst_tab[%d]={' % len(INSTS)]
    lines.append(','.join('{%d,%d,%d,%d,%d}' % n[1:] for n in INSTS) + '};')
    total = 0
    for k, c in enumerate(CUES):
        b = c.encode()
        total += len(b)
        lines.append('#define CUE_%s %d /* %.2f s at %d BPM%s */' % (
            c.name, k, c.seconds(), c.bpm, ', loops' if c.loop else ''))
        lines.append('static const unsigned char cue_%s[%d]={' % (c.name.lower(), len(b)))
        for i in range(0, len(b), 24):
            lines.append(','.join(str(x) for x in b[i:i + 24]) + (',' if i + 24 < len(b) else ''))
        lines.append('};')
    lines.append('#define CUE_COUNT %d' % len(CUES))
    lines.append('static const Cue cues[CUE_COUNT]={')
    lines.append(',\n'.join('{cue_%s,%d}' % (c.name.lower(), c.step) for c in CUES))
    lines.append('};')
    lines.append('/* %d event bytes */' % total)
    return '\n'.join(lines) + '\n'


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--check', action='store_true')
    a = p.parse_args()
    text = generate()
    if a.check:
        if OUT.read_text() != text:
            sys.exit('MUSIC.H is stale: run tools/alfredo2_score.py')
        print('PASS: MUSIC.H matches the score source')
        return
    OUT.write_text(text)
    for c in CUES:
        print('%-9s %6.2f s  %3d BPM  %s' % (c.name, c.seconds(), c.bpm, 'loop' if c.loop else ''))


if __name__ == '__main__':
    main()
