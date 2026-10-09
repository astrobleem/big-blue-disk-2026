# Publication provenance

New original implementations and maps were written for this project. Historical
Kroz instructions were inspected read-only to identify exploration, gems,
keys/doors, whip, terrain, stairs and teleports. No historical executables were
run or included; no copied historical maps or extracted graphics are published.

Kroz's documented new choices include an eight-neighbor whip costing one charge,
guaranteed nearby terrain clearing, two-gem contact, generous entry supplies,
automatic free retry, unlimited safe grenades, fixed paired portals and bounded
blocking forest fire. These are faithful-style design choices, not asserted
exact historical parameters. No full save/load, classic whip-power system or
thirty-level feature coverage is claimed.

The camera's five-file source bundle is preserved in `assets/camera`, with its
original reference notes. The runtime monochrome CGA diagram was losslessly
encoded from the supplied 640x200 image. Its encoded bytes are unchanged.

The sound article/examples are original. The cooperative adapter is used from
`astrobleem/oemsound-tandy` at `61d6c3cb5580d5f1c31a2950fc9eb67ef4d846fa`.
The upstream repository identifies GPL-3.0; its used source and license are
included. The project adopts GPL-3.0-only for its new original material, retaining
the upstream license. Compiler and OS files are excluded.

The public package audit uses an allowlist of source, assets, documentation,
tests and eighteen issue runtime files. `runtime-manifest.json` records both
public hashes and the matching qualification payload. Only the two root public
README/source notices differ from the private tested disk. No claim is made
that a compiler was rebuilt or bundled for release.
