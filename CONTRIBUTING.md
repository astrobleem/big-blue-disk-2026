# Contribute a monthly piece

Open a content proposal using the issue template: title, a short playable or
readable example, size/memory budget, and credits/license. Keep the first slice
small enough to finish and test. This is a monthly magazine, not a requirement
to produce a large game or simulation.

- Target an 8088 Tandy 1000 EX with 640 KB installed RAM and a 720 KB data disk.
  Installed RAM is not the same as free DOS application memory: report measured
  state, stack, heap and video reservations.
- Use DOS 8.3 paths and self-contained assets. Supply the actual corresponding
  source, build instructions, compiler/version and exact executable hashes.
  Don't redistribute a compiler, proprietary runtime installation, OS, private
  files, unlicensed media or historical magazine programs.
- Make text readable in 80x25 and graphics legible at native low resolution.
  Give controls and feedback in the piece. Avoid hidden hazards or adding
  difficulty merely to expand content; favor useful choices and enjoyable play.
- A child program must return cleanly: restore video mode/cursor and directory,
  silence/release owned sound, restore any installed vectors/timer state, and
  handle Escape, errors and repeated launches. Avoid resident hooks.
- Test real mechanics and failure boundaries, not only happy-path screenshots.
  Label host checks, emulation, actual hardware and listening tests separately.
  Buffer reconstructions are not screenshots. State what remains unqualified.
- Articles need checked primary references and original, concise writing. Keep
  copied quotations minimal. Low-resolution diagrams should have editable source.
- Provide piece-level author/tool credits and license provenance for every
  dependency and asset. New submissions use GPLv3-compatible terms; identify
  exceptions before including them. AI-assisted work should name actual tools
  used and who supplied/edited the result, without inventing collaborators.

The steward selects each month's roster. Six entries are used in October;
future roster changes should be deliberate and tested. Put sources in `src`,
authoring assets in `assets`, and issue payloads in `issues/YYYY-MM/BBD2026`.
Use the [issue checklist](docs/MONTHLY-ISSUE.md) before a release. The steward
handles any outside collaborator recruitment; do not contact services on their behalf.
