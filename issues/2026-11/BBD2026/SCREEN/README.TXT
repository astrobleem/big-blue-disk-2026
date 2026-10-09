YOUR TANDY HAS ANOTHER SCREEN - CORRECTED EXPERIMENTAL CANDIDATE

Not on the October release or a November roster.
SCREEN 1: palette cycling. SCREEN 3: raster palette split.
SCREEN alone runs 1 then 3. Space advances; Esc/Ctrl-C exits.
SCREEN 2 is disabled pending proof of two owned video buffers.
Mode08h only: 160x200x16, single16KB frame in the existing odd window.
Start in standard80-column DOS text mode3, compatible Tandy, no Windows.
Return restores the full16KB text window, active page and cursor/hook.
No sound/timer writes. BIOS-standard palette on return, not custom colors.

Fresh MSC6 /G0 /AS /Os /W3 build; repeats identical and warning-free.
Host/native helpers and bounded exit/fault fixtures passed in emulation.
No physical speed, memory layout on every BIOS, blanking or CRT result is
qualified. Never interpret helper counts or VRAM samples as hardware tests.
READ ARTICLE.TXT and REFS.TXT. Exact bytes, source and allocation bounds:
docs/candidates/screen.json in the source repository.
GPLv3. Requires your separately licensed toolchain to rebuild.
https://github.com/astrobleem/big-blue-disk-2026
