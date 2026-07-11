# Design drops

This folder holds design handoffs from [Claude Design](https://claude.ai/design)
and the curated artifacts extracted from them.

```
design/
├── incoming/<yyyy-mm-dd>-<label>/   raw unzipped handoff — never edited
├── tokens/                          curated token extractions (optional)
└── screens/                         per-screen specs / references (optional)
```

## Drop procedure

1. Unzip a new handoff into `design/incoming/<yyyy-mm-dd>-<label>/` and commit
   it verbatim (it is the diffable provenance for the design).
2. Read the primary `*.dc.html` file top to bottom — it is a self-contained
   prototype: the `PAL`/`ST` tables define the palette, the `renderVals()`
   function defines every screen's data and layout, and the markup shows the
   exact geometry.
3. Map the artifacts to code using [`docs/DESIGN_INTAKE.md`](../docs/DESIGN_INTAKE.md).
4. Recreate the visuals in the QML/C++ app — match the output, don't copy the
   prototype's HTML structure.

## Current design

`incoming/2026-07-11-claude/` — the **Material 3 rebuild** handoff: one
unified window with three switchable UI styles (Tonal Rail / Split Dock /
Card Flow), each with a full light+dark Material 3 palette, plus a git
history manager, notification center, and a regex builder. Implemented across
`src/quick/theme/thememanager.cpp` (palettes) and `src/quick/qml/shell/`
(the redesigned shell). See `docs/DESIGN_INTAKE.md` for the full mapping.
