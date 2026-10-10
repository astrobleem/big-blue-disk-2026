# Alfredo: After Hours (episode 2, November candidate)

A new Alfredo episode proposed for the November issue's Alfredo slot. It is
**not on a roster** yet, and October's episode, payload and qualification are
unchanged. Story, pictures, music and code: Claude, for Chad. Pond solver:
Flow-88 (Gemini design, Claude port, Codex safety corrections).

## The episode

| # | Scene | What happens |
| --- | --- | --- |
| 1 | Title | Big Blue Disk presents *Alfredo*, episode 2: *After Hours* |
| 2 | The rave | 4:00 AM at the Radio Shack Rave. DJ Alfredo on the decks (a Tandy 1000 runs the visuals), glow-stick crowd, palette-cycled dance floor and light beams, LETS GO! |
| 3 | Last call | Record scratch, house lights up, the crowd leaves. Headphones off, fishing hat on. "GOOD NIGHT, RAVERS!" to an empty room |
| 4 | Clock out | The Punch-O-Matic stamps his card: 04:17 OUT |
| 5 | The walk | Past the dark club and the MOS Labs data center from last month (training 95%... 96%...). He stops and wonders |
| 6 | His spot | Pre-dawn at the pond the plant warms: WARM WATER OUTFLOW, NO SWIMMING. Chair, rod, a fish jumps |
| 7 | The cast | Wind-up, cast, plop |
| 8 | The pond | Top-down and live: the warm plume from the outflow pipe swirls round the boulder toward the intake. Small fish, a big shadow, two nibbles, a strike |
| 9 | Fish on | The fight, fin cutting the surface, sunrise |
| 10 | The catch | A three-eyed glowing warm-water monster. PERSONAL BEST! Flash. Catch and release |
| 11 | 5:00 AM | MOS Labs: TRAINING RUN COMPLETE. HEAT FLUSH IN 3, 2, 1. Klaxon |
| 12 | The surge | The flush hits the pond. Alfredo is washed in, spun twice round the boulder in the vortex and taken by the INTAKE. His hat is left to the real current |
| 13 | Calm | Sunrise. The hat floats. Sad trombone. The fish surfaces wearing it |
| 14 | Credits | ...CONTINUED IN LAST MONTH'S ISSUE (October's episode is the trip through the cooling system) |

After the credits: **F** stays at the pond, a small playable Flow-88 toy.
Arrows move the lure (the big fish follows it), Space stirs, H toggles the
heat flush, C calms the water, Esc returns to the end card.

Keys during the show: Space/P pause, Left/Right previous/next scene, R replay,
M mute, Esc exit.

## How it works

- **Picture engine** (`ENGINE.H`): Tandy mode 9 (320x200x16). Two far 32K
  buffers in video-bank layout hold the painted backdrop and the composed
  frame. Actors are drawn into the frame, and only their rectangles are
  restored and copied to the screen (REP MOVSW per row with the bank step done
  in registers). Keyed actors whose key has not changed (a raver between beats,
  the DJ between half-beats) are not touched at all. A host check proves the
  pictures are identical to always redrawing everything.
- **Cartoon timing**: a new picture every 384/256 of a BIOS tick (about 12 a
  second, "on threes"). Between pictures the machine keeps the music fed and
  works on the pond. Fades, sunrise, strobes and shimmer are palette
  register changes made in vertical retrace.
- **Clock** (`CLOCK.H`, from Radio Shack Rave): BIOS ticks plus a latched read
  of PIT 0, never reprogrammed; 1/256-tick resolution.
- **Sound** (`SOUND.H`, `MUSIC.H`): twelve original cues on the SN76496, three
  tone voices with software envelopes and vibrato, a noise-channel drum kit
  (Radio Shack Rave's), and fourteen procedural sound effects that borrow voice
  2 or the noise channel. The PSG is claimed through the shared `src/sound`
  owner and silenced on exit. Score source: `tools/alfredo2_score.py`.
- **Pond** (`FLUID.H`): Flow-88's stable-fluids method (forces, semi-Lagrangian
  advection, Gauss-Seidel projection, 8.8 fixed point), with the current on a
  coarse 16x8 grid and the heat it carries on a fine 32x16 grid. Every value is
  a 16-bit int and every product stays inside 16 bits. One step is spread over
  five stages; vorticity confinement keeps the boulder's wake curling. Heat is
  drawn as interpolated 4x4 blocks on a nine-step dithered ramp, written straight
  to the screen except under actors.

## Build

Reference: Microsoft C 6, `src/alfredo2/BUILD.BAT` (run in `src\ALFREDO2` with
`src\SOUND` beside it). Previews and tests: OpenWatcom 2,
`WATCOM=/opt/ow tools/alfredo2_build_ow.sh` (also caps MZ maxalloc at 4096
paragraphs, as `/CP:4096` does). The candidate executable was built with
OpenWatcom 2; **an MSC6 build has not been run yet**.

## Checks

- `python tests/host_checks.py` runs `HOSTQA.C`: the whole episode three times
  on the virtual clock (twice with actor keeping, once redrawing every actor):
  identical pictures, screen equal to the composed frame on every frame, no
  writes outside the visible bank area, every scene and sound effect reached,
  no music event late, fluid values inside their clamps, no erase conflicts,
  plus fish mode and scene skip/replay.
- `python tests/alfredo2_dos.py --exe ALFREDO.EXE --host-cap DIR` runs DOSBox-X
  (machine=tandy, 8086 prefetch): `/QA` on the virtual clock with 42 stills
  that must equal the host-rendered frames pixel for pixel, `/AUTO` in real time
  at chosen cycle counts, text mode restored, and refusal on a non-Tandy.
- `python tools/alfredo2_video.py CAPTURE out.mp4` turns a `/RENDER` capture
  (or the host check's frames) into a preview video with synthesized PSG sound.

Emulator results are in `docs/candidates/alfredo2.json` and its logs. At 3,000
cycles (the figure October's episode was qualified at) every scene draws its
full 12 pictures a second and no music event is late. At 450 cycles, a rough
stand-in for a 7.16 MHz Tandy 1000 EX, scenes draw 4 to 10 pictures a second,
the pond's solver manages under one step a second, and a few music events land
up to about 160 ms late. Scene changes take about 2 seconds there while the
screen is dark. **No physical Tandy timing, memory or listening is claimed.**

## Memory and disk

The MZ image is about 60 KB with its near data capped at 64 KB, plus two 32 KB
far buffers: about 150 KB free conventional memory is needed. The folder
(`ALFREDO.EXE`, `README.TXT`, `LICENSE.TXT`) uses about 100 KB of 1 KB
clusters, inside the 169 KiB the future-disk plan leaves free.
