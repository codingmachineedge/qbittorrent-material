# Architecture Guide

qBittorrent Material separates the torrent engine from the presentation layer so Qt Quick screens bind to stable, testable models instead of reaching into libtorrent.

## Layers

```text
Qt Quick Material screens and reusable controls
                 ↓
Controllers and QAbstractItemModels
                 ↓
Application/session interfaces and settings
                 ↓
libtorrent-rasterbar, OpenSSL, Boost, and the OS
```

## Key directories

- `src/base` — domain types, session state, persistence, networking, search, and RSS.
- `src/quick/controllers` — UI-facing commands and orchestration.
- `src/quick/models` — list/table/filter models exposed to QML.
- `src/quick/qml` — the complete Material application shell, screens, dialogs, and components.
- `resources` — fonts, icons, palettes, branding, and platform assets.
- `docs` — architecture, contracts, screen inventories, design tokens, generated site corpus, and wiki.

For deeper details, read [Architecture](../ARCHITECTURE.md), [Engine API](../ENGINE_API.md), and [Contracts](../CONTRACTS.md).
