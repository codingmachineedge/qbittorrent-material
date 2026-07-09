# qBittorrent Material

A ground-up rewrite of [qBittorrent](https://www.qbittorrent.org/) with a **Qt 6 / QML** front end styled entirely with **Material Design** (Qt Quick Controls 2 Material style). Every window and every dialog is Material.

- **Language:** C++20 (engine + model layer) and QML (all UI)
- **Engine:** wraps `libtorrent-rasterbar` 2.x
- **UI:** Qt Quick Controls 2, Material style, light + dark themes
- **Goal:** feature-for-feature clone of qBittorrent's desktop client, rebuilt as Material Design

## Status

Feature-by-feature rewrite in progress. See
[`docs/FEATURE_SPEC.md`](docs/FEATURE_SPEC.md) for the full feature inventory.

## Building

On Windows, the helper provisions Qt 6.8.3 and the remaining repository-local
dependencies, then builds with MSVC 2022 and Ninja:

```powershell
# Build and run
powershell -ExecutionPolicy Bypass -File .\run.ps1

# Build only
powershell -ExecutionPolicy Bypass -File .\run.ps1 -NoRun

# Build a self-contained Windows installer
powershell -ExecutionPolicy Bypass -File .\run.ps1 -Package
```

CPack writes local installers and SHA-256 checksums to `build\packages`.
See [`docs/BUILDING.md`](docs/BUILDING.md) for prerequisites, manual CMake
commands, and Linux/macOS instructions.

## Continuous prereleases

Every branch push runs a Windows installer build on GitHub Actions using
`windows-2022` and Qt 6.8.3. A successful run publishes the installer as a
workflow artifact and creates a uniquely tagged GitHub prerelease for that
exact commit.

## License

GPLv3+, matching upstream qBittorrent.
