# Big Blue Disk 2026

**October 2026 · Issue 1 · public preview**

A new monthly DOS disk magazine for the 8088-era Tandy 1000. An unofficial
revival made from original new programs, articles and assets; no affiliation
with or endorsement by the original publishers or creators is claimed.

[Download the October issue](https://github.com/astrobleem/big-blue-disk-2026/releases/tag/issue-2026-10-preview.1)

| Download | What it contains |
| --- | --- |
| [BBD2610.IMG](https://github.com/astrobleem/big-blue-disk-2026/releases/download/issue-2026-10-preview.1/BBD2610.IMG) | 720 KB FAT12 DOS **data** disk; boot DOS separately |
| [BBD2610.ZIP](https://github.com/astrobleem/big-blue-disk-2026/releases/download/issue-2026-10-preview.1/BBD2610.ZIP) | Ready-to-run `BBD2026` folder |
| [SOURCE26.ZIP](https://github.com/astrobleem/big-blue-disk-2026/releases/download/issue-2026-10-preview.1/SOURCE26.ZIP) | Complete corresponding source, authoring assets and build/validation scripts |
| [READTHIS.TXT](https://github.com/astrobleem/big-blue-disk-2026/releases/download/issue-2026-10-preview.1/READTHIS.TXT) | Short DOS-friendly instructions and limits |
| [SHA256SUMS](https://github.com/astrobleem/big-blue-disk-2026/releases/download/issue-2026-10-preview.1/SHA256SUMS) | Download checksums |

## Play

Unzip the DOS folder, or mount/write the data image using your existing disk
tools. Boot your own DOS, `CD BBD2026`, then run `GO`. The target is a Tandy
1000 EX with an 8088 and 640 KB installed RAM. Other compatible DOS machines
may run the text programs; Alfredo needs Tandy graphics and the sound examples
need compatible Tandy sound. No installation or OS/driver replacement is required.

The blue menu uses arrows or 1–6 to select, Enter to open, and Esc to exit.
Articles use PgUp/PgDn, Space, Home/End and Esc. Press D in the camera article
for its diagram. Programs return to the magazine.

## In this issue

| Entry | Content |
| --- | --- |
| Alfredo | An original silent sixteen-scene cartoon with pause/replay controls |
| Kroz: Grenades | Three original dungeons with whip, gems, keys/doors, stairs, portals, rivers, forest fire and additive grenades |
| Horse racing | Win/place betting and a small race simulation |
| Drug Wars | A fictional trading, travel, banking and debt game |
| Camera obscura | An 844-word illustrated article opening a civilization/reboot series |
| Your Tandy Has Another Voice | An article and three original sound-programming demonstrations |

Kroz controls: arrows/numpad move, W whips, Space throws, R retries, N skips,
Esc returns. Supplies are generous; grenades are unlimited and cannot hurt the
player or destroy pickups. Read `KROZ/PLAY.TXT` for the documented changes from
classic Kroz. The last stairs complete this small descent; no mandatory Staff quest.

## Qualification

Host and 16-bit emulator checks passed, including 65,620 Kroz checks, three
key-to-stairs routes, 308 reader checks, six blue-menu selections, child
directory/video restoration, article paging and diagram readback. The original
Grenade Garden prototype received positive user hardware feedback. **The
expanded game in this release still awaits a physical playthrough.**

The final integrated functional test used Tandy/8086 emulation at 50,000 cycles;
it does not establish physical speed or smoothness. Alfredo separately rendered
941/941 scheduled frames at 3,000 emulator cycles. No physical Alfredo timing
or sound listening qualification is claimed. See [qualification](docs/QUALIFICATION.md).

The released executable/content bytes match the tested private assembly. Only
public README/source-download framing changed for publication. Packaging and
CI check the exact public hashes and FAT12 contents; no new guest was launched.

## Make the next issue

Monthly contributions are welcome: a small game, show, puzzle, practical
article, art or music demo. Start with [CONTRIBUTING](CONTRIBUTING.md) and the
[monthly issue checklist](docs/MONTHLY-ISSUE.md). Contributions are credited
when actually included. No contributions from other recruited assistants or
services are invented or implied here.

The browsable [source](src) builds with a separately obtained licensed Microsoft
C 6 DOS toolchain. CI compiles the portable C checks with a host compiler and
validates/packages already qualified DOS executables; **CI does not rebuild
those legacy DOS executables**. See [build instructions](docs/BUILD.md).

GPLv3: see [LICENSE](LICENSE), [credits](CREDITS.md), and [provenance](docs/PROVENANCE.md).
