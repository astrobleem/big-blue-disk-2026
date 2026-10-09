# Flow-88: separate experimental contribution

Original design/draft: Gemini. MSC6 port: Claude. Safety corrections: Codex, submitted by Chad. Original Claude source is preserved at PR #1 commit `9be7cb8`; this branch does not depend on the fillers contribution.

Plain DOS and a compatible Tandy are required, already in 80x25 BIOS text mode 3. The program refuses Windows before changing the screen or hooks, saves the active text page and cursor, and restores them on normal, Escape, Ctrl-C and INT 23h exits. No resident code, video mode change, timer programming, PIT writes, PSG writes or speaker writes. Audio has been omitted deliberately: PSG registers are write-only and arbitrary caller audio state cannot be restored accurately. Reading port 61h does not change it; tests compare its writable control bits, excluding changing timer input bits.

The toy solver uses signed 16-bit storage on host and DOS. Arithmetic widens before divergence, pressure sums, gradient subtraction and coordinate clamping, and saturates before narrowing. All seven density characters are reachable. This is an interactive toy, not a quantitatively validated fluid simulator. 8088 speed is unmeasured and it may be slow; Escape is handled before the next solver frame.

Arrows move; Space/Enter toggles injection; B toggles obstacle; V toggles vectors; O cycles presets; C clears fluid; Q/Escape/Ctrl-C exits. No sound control is offered. Candidate source/core tests and the proposed executable are separately hashed in `docs/candidates/flow88.json`. It is not on an issue roster.

Build with your own MSC6: stage the C/H files in a fresh 8.3-compatible work directory, compile `FLOW88.C` with `/G0 /AS /Os /W3`, then LINK `/NOI /MAP /CP:4096`. `FLOWQA.C` is pure C and runs on host and DOS. Native fixtures in `tests/native` must run only inside an owned, allocated test guest. Do not run a Windows failure-injection test by launching the hardware program in Windows.
