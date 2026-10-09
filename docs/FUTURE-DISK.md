# Future disk fit (planning only)

The public original-song Rave package fits with the corrected candidates and a composite-demo allowance. No roster or payload has been assembled from this plan.

Starting from October's 549,888 free bytes, 1,024-byte FAT12 clusters give:

| Component, including its directory | Allocated bytes |
| --- | ---: |
| Public Rave original-song runtime and notices | 98,304 |
| Corrected SCREEN, article and notices | 61,440 |
| Flow-88 runtime and notices | 51,200 |
| Revised fillers alongside the old pair | 67,584 |
| Experimental composite-demo allowance | 98,304 |
| **Remaining** | **173,056 (169 KiB)** |

This conservatively duplicates licenses and the old filler pair. The private ISSUE02 baseline would leave 172,032 bytes. Source ZIPs, authoring PNGs, raw captures and QA logs belong in companion downloads, outside the floppy. Figures are recorded in `future-disk-budget.json`.

Recommend the [public Rave v0.6.0 showcase preview](https://github.com/astrobleem/radio-shack-rave/releases/tag/v0.6.0-showcase-experimental): `RSRAVE.EXE` 54,138 bytes, default `ORIGINAL.RBG` 1,920 bytes, and its launcher, instructions, GPL and music notices. "Circuit After Hours" has an explicit permissive composition/chart grant. These seven files use 97,280 file-cluster bytes plus one 1,024-byte directory. No configuration or logo is required. Private song arrangements and media are excluded.

GO changes to the executable's directory and launches without arguments, so Rave's default score works. Escape during play returns to title; Escape at title exits. GO then restores the issue root, mode 3 and menu. Rave's own cleanup restores IRQ1, releases sound ownership and restores its original video mode. The integrated reader-to-Rave-to-reader path still needs qualification. The reader currently supports exactly six entries: choose a replacement roster or separately qualify menu expansion.

Rave's maximum MZ image/extra block is 119,616 bytes, excluding PSP/environment; its optional far render cache needs another 9,216 bytes plus allocator overhead. Budget roughly 129 KB plus environment and its 32 KB video frame for the child. GO's MZ minimum is 27,984 bytes, but maxalloc is FFFF: its actual resident size and largest free child block have not been measured here. Disk fit is confirmed; integrated memory fit and physical v6 timing/listening remain gates.

A clearly labelled experimental composite piece with an RGB fallback can fit the 96 KiB allowance. Existing G1's original patterns use 84,992 allocated bytes including its directory: DIAG.EXE 13,919 bytes, two WBI files 32,782 each, README 1,863. This is a size reference, not a new implemented or qualified demo. Keep calibration static, native/RGB references and key-stepped phase comparison. Accept composite hues only after WEGA observations. Palette cycling does not establish artifact-color fidelity or arbitrary true-color capability. No private NASA/art assets or unverified high-color screenshots are proposed.
