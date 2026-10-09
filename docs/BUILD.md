# Build and validate

From the repository root, with Python 3 and a C host compiler:

```
python tests/host_checks.py
python tools/validate.py
python tools/package.py
```

Host checks compile portable reader, Kroz, filler and Alfredo-renderer tests.
They produce host executables under `build/`; those are not DOS programs and
never enter the issue. The Windows checkout can use the existing OpenWatcom
host compiler with `--compiler PATH --watcom`. Linux CI uses the runner C compiler.

The committed DOS executables were qualified with separately licensed Microsoft
C 6, 8088-compatible `/G0 /AS` small-model builds. DOS source `BUILD.BAT` files
record the linker/stack options. They expect an existing compiler setup; adjust
the drive/path variables for yours. No compiler, `.LIB`, `.OBJ`, OS or installation
files are redistributed. CI does not pretend to rebuild these DOS binaries.

Reader: `src/reader`. Kroz: `src/kroz`. Alfredo: `src/alfredo`; its DOS batch
expects the C/H files beneath a `src` directory, so place them there or adjust
that relative path. Fillers: `src/fillers`. Sound: `src/sound`; retain the
`win30/DRIVER/PITCORE.H` relative dependency. Runtime hashes must be requalified
after any new native build. Do not silently replace tested payloads.

The packaging tool builds `build/release` deterministically: `BBD2610.IMG`,
`BBD2610.ZIP`, `SOURCE26.ZIP`, `READTHIS.TXT`, `SHA256SUMS`. The image is a
non-OS data disk. The source archive includes current source/authoring assets,
tests, instructions and notices. GitHub Actions runs validation and uploads
these packages as CI artifacts; public releases are separately published.

The source ZIP omits DOS executables, but includes issue configuration/articles
and data assets. To run payload validation from that archive, restore the
qualified DOS folder into `issues/2026-10/BBD2026` from BBD2610.ZIP first.
