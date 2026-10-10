"""Preview video or stills for "Alfredo: After Hours" from a capture.

A capture is what ALFREDO.EXE /RENDER (or the host check, given a folder)
writes on its virtual clock: Fnnnnn.RAW per presented frame (160 bytes x 200
rows, Tandy mode 9 nibbles), PSG.LOG ("<fine time> <byte>", 1/256 BIOS tick)
and PAL.LOG ("<frame> <16 physical colors>"). Every frame is exactly half a
BIOS tick, so the video is frame-exact and independent of emulator speed.

    python tools/alfredo2_video.py CAPTURE out.mp4
    python tools/alfredo2_video.py CAPTURE sheet.png --sheet 100,900,2400

The PSG model is a plain SN76496 (3.579545 MHz, 2 dB steps, 15-bit noise
register): a preview, not a recording of a real Tandy. Needs numpy, Pillow
for stills and ffmpeg for video.
"""
import argparse, subprocess, wave
from pathlib import Path
import numpy as np

PAL = [(0,0,0),(0,0,170),(0,170,0),(0,170,170),(170,0,0),(170,0,170),(170,85,0),(170,170,170),
       (85,85,85),(85,85,255),(85,255,85),(85,255,255),(255,85,85),(255,85,255),(255,255,85),(255,255,255)]
PIT = 1193182; CLOCK = 3579545; RATE = 44100
FINE_SECONDS = 65536 / PIT / 256
FPS = PIT / 65536 * 2


def noise_run(lfsr, white, steps):
    out = np.empty(steps, dtype=np.int8)
    for k in range(steps):
        bit = ((lfsr & 1) ^ ((lfsr >> 1) & 1)) if white else (lfsr & 1)
        lfsr = (lfsr >> 1) | (bit << 14)
        out[k] = 1 if lfsr & 1 else -1
    return lfsr, out


def synth(log, seconds):
    events = []
    for line in Path(log).read_text().split('\n'):
        if line.strip():
            t, b = line.split()
            events.append((int(t) * FINE_SECONDS, int(b)))
    n = int(seconds * RATE); out = np.zeros(n)
    div = [1, 1, 1]; att = [15, 15, 15, 15]; noise_ctl = 0; latch = 0
    vol = [0 if a == 15 else 10 ** (-a / 10) for a in range(16)]
    phase = [0.0, 0.0, 0.0]; lfsr = 0x4000; nphase = 0.0
    bounds = [int(t * RATE) for t, _ in events] + [n]
    pos = 0
    for i in range(len(events) + 1):
        end = min(bounds[i], n)
        if end > pos:
            seg = np.arange(end - pos)
            for ch in range(3):
                if att[ch] < 15 and div[ch] > 1:
                    f = CLOCK / (32 * div[ch])
                    ph = phase[ch] + seg * f / RATE
                    out[pos:end] += np.where((ph % 1.0) < 0.5, 1.0, -1.0) * vol[att[ch]] * 0.22
                    phase[ch] = (phase[ch] + (end - pos) * f / RATE) % 1.0
            if att[3] < 15:
                rate = [CLOCK / 512, CLOCK / 1024, CLOCK / 2048, CLOCK / (32 * max(div[2], 1))][noise_ctl & 3]
                if not noise_ctl & 4:
                    rate /= 15          # periodic noise: 1/15 duty buzz
                pos_steps = nphase + seg * rate / RATE
                steps = int(pos_steps[-1]) + 1
                lfsr, bits = noise_run(lfsr, noise_ctl & 4, steps)
                out[pos:end] += bits[pos_steps.astype(int).clip(0, steps - 1)] * vol[att[3]] * 0.22
                nphase = (nphase + (end - pos) * rate / RATE) % 1.0
            pos = end
        if i == len(events):
            break
        b = events[i][1]
        if b & 0x80:
            latch = (b >> 5) & 3
            if b & 0x10:
                att[latch] = b & 15
            elif latch == 3:
                noise_ctl = b & 7; lfsr = 0x4000
            else:
                div[latch] = (div[latch] & 0x3f0) | (b & 15)
        elif latch < 3:
            div[latch] = (div[latch] & 15) | ((b & 63) << 4)
    # Gentle low-pass so square waves are less harsh on laptop speakers.
    k = np.ones(3) / 3
    return np.clip(np.convolve(out, k, mode='same'), -1, 1)


def palettes(cap):
    changes = []
    if (cap / 'PAL.LOG').exists():
        for line in (cap / 'PAL.LOG').read_text().split('\n'):
            v = line.split()
            if len(v) == 17:
                changes.append((int(v[0]), [int(c) for c in v[1:]]))
    changes.sort(key=lambda c: c[0])
    return changes


def frame_rgb(path, pal):
    lut = np.array(PAL, dtype=np.uint8)[pal]
    b = np.frombuffer(path.read_bytes(), dtype=np.uint8)
    idx = np.empty(b.size * 2, dtype=np.uint8); idx[0::2] = b >> 4; idx[1::2] = b & 15
    return lut[idx].reshape(200, 320, 3)


def pal_at(changes, n):
    pal = list(range(16))
    for f, p in changes:
        if f > n:
            break
        pal = p
    return pal


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('capture', type=Path); p.add_argument('output', type=Path)
    p.add_argument('--scale', type=int, default=3)
    p.add_argument('--sheet', help='comma-separated frame numbers for a contact sheet PNG')
    p.add_argument('--cols', type=int, default=4)
    a = p.parse_args()
    changes = palettes(a.capture)
    if a.sheet:
        from PIL import Image
        nums = [int(x) for x in a.sheet.split(',')]
        cols = min(a.cols, len(nums)); rows = (len(nums) + cols - 1) // cols
        sheet = Image.new('RGB', (cols * 320, rows * 240))
        for k, n in enumerate(nums):
            im = Image.fromarray(frame_rgb(a.capture / ('F%05d.RAW' % n), pal_at(changes, n))).resize((320, 240), Image.NEAREST)
            sheet.paste(im, ((k % cols) * 320, (k // cols) * 240))
        sheet.save(a.output)
        print(a.output)
        return
    frames = sorted(a.capture.glob('F*.RAW'))
    seconds = len(frames) / FPS
    audio = synth(a.capture / 'PSG.LOG', seconds)
    wav = a.output.with_suffix('.wav')
    with wave.open(str(wav), 'wb') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(RATE)
        w.writeframes((audio * 32000).astype('<i2').tobytes())
    W, H = 320 * a.scale, 240 * a.scale
    cmd = ['ffmpeg', '-y', '-loglevel', 'error', '-f', 'rawvideo', '-pix_fmt', 'rgb24', '-s', '320x200',
           '-framerate', f'{FPS:.6f}', '-i', '-', '-i', str(wav),
           '-vf', f'scale={W}:{H}:flags=neighbor', '-c:v', 'libx264', '-pix_fmt', 'yuv420p',
           '-crf', '18', '-c:a', 'aac', '-b:a', '160k', '-shortest', str(a.output)]
    proc = subprocess.Popen(cmd, stdin=subprocess.PIPE)
    pal = list(range(16)); ci = 0
    for f in frames:
        n = int(f.stem[1:])
        while ci < len(changes) and changes[ci][0] <= n:
            pal = changes[ci][1]; ci += 1
        proc.stdin.write(frame_rgb(f, pal).tobytes())
    proc.stdin.close(); proc.wait()
    wav.unlink()
    print(f'{a.output}: {len(frames)} frames, {seconds:.1f} s')


if __name__ == '__main__':
    main()
