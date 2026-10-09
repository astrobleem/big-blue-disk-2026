# Revised fillers candidate

Correction to Claude PR #1, preserving his original commit. New Derby/Drug Wars sources are in `src/fillers-next`; October sources and all October executables remain byte-for-byte unchanged. Flow-88 is a separate contribution.

The horse index is checked before any Race read. Invalid-index regressions use a checked-access hook and null Race. The controlled host mutant restoring the old access order exits 99 before reading invalid memory; this remains observable under optimization. Game builds contain no hook instrumentation. The rumor test now tests the minimum city bias instead of `||1`.

Candidate binaries are proposed under `issues/2026-11/BBD2026/FILLERS/V3`; they do not enter any roster or release. `docs/candidates/fillers-next.json` records actual fresh MSC6 build inputs, binary hashes, linker allocation bounds and native results. Run the normal host suites, October validation and candidate validator. Select a future roster explicitly before packaging these executables.

Original work/changes remain GPLv3. Original authorship: Claude, edited/submitted by Chad; review corrections and test separation: Codex for Chad. No physical Tandy performance or long human playtest is claimed.
