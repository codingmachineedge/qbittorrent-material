# qBittorrent Material

A ground-up rewrite of [qBittorrent](https://www.qbittorrent.org/) with a **Qt 6 / QML** front end styled entirely with **Material Design** (Qt Quick Controls 2 Material style). Every window and every dialog is Material.

- **Language:** C++20 (engine + model layer) and QML (all UI)
- **Engine:** wraps `libtorrent-rasterbar` 2.x
- **UI:** Qt Quick Controls 2, Material style, light + dark themes
- **Goal:** feature-for-feature clone of qBittorrent's desktop client, rebuilt as Material Design

## Status

Feature-by-feature rewrite in progress. See [`docs/FEATURE_SPEC.md`](docs/FEATURE_SPEC.md) for the full inventory of cloned features and [`docs/PARITY.md`](docs/PARITY.md) for the checklist.

## Building

Requires:

- Qt 6.5+ (`Quick`, `QuickControls2`, `Qml`, `Core`, `Network`, `Widgets` for tray/native dialogs)
- libtorrent-rasterbar 2.0+
- CMake 3.21+, a C++20 compiler, Ninja

```sh
cmake -B build -G Ninja -DCMAKE_PREFIX_PATH="<Qt6 dir>;<libtorrent dir>"
cmake --build build
```

See [`docs/BUILDING.md`](docs/BUILDING.md).

## License

GPLv3+, matching upstream qBittorrent.
