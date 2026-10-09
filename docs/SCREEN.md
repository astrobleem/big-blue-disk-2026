# SCREEN correction

Preserves Claude PR #3 and supersedes its stale captures. Actual qualified binaries and corresponding-source hashes are recorded separately from October in `docs/candidates/screen.json`.

The page-flip path is compiled as a refusal; default sequencing skips it. Remaining demos use mode 08h, one 16 KB frame in the caller's already-used matching odd CPU/CRT window, with 16 KB access bounds. The full window, active text page, cursor, mode and INT 23h hook are restored. No new frame is selected, and no unsafe opt-in flag exists. BIOS-standard text palette is restored; custom write-only palette state cannot be saved.

Source helpers still explain 320-wide layouts for comparison. The native loops render sampled 160-wide rows. A future 320-wide or two-frame version requires separate allocation proof and qualification. Raster timing and composite hue remain physical-test gates.

Host/native checks, repeat-build hashes and exact-source VRAM samples are captured after the correction. Captures show pixel-index/layout evidence and a program-side palette-write trace; they are not window screenshots, beam-timing evidence or physical monitor observations. Original screenshot files remain available in Claude's preserved head.
