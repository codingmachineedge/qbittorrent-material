<p align="center">
  <img src="resources/branding/logo-horizontal.svg" width="620" alt="qBittorrent Material">
</p>

<p align="center">
  <a href="https://codingmachineedge.github.io/qbittorrent-material/">Material documentation site</a>
  ·
  <a href="https://github.com/codingmachineedge/qbittorrent-material/wiki">GitHub Wiki</a>
  ·
  <a href="https://github.com/codingmachineedge/qbittorrent-material/releases">Windows installers</a>
</p>

# qBittorrent Material

A ground-up rewrite of [qBittorrent](https://www.qbittorrent.org/) with a **Qt 6 / QML** front end styled entirely with **Material Design** (Qt Quick Controls 2 Material style). Every window and every dialog is Material.

- **Language:** C++20 (engine + model layer) and QML (all UI)
- **Engine:** wraps `libtorrent-rasterbar` 2.x
- **UI:** Qt Quick Controls 2, Material style, light + dark themes
- **Goal:** feature-for-feature clone of qBittorrent's desktop client, rebuilt as Material Design

## Status

Feature-by-feature rewrite in progress. See
[`docs/FEATURE_SPEC.md`](docs/FEATURE_SPEC.md) for the full feature inventory.

## Documentation website

The [GitHub Pages site](https://codingmachineedge.github.io/qbittorrent-material/)
puts the landing page, every Markdown and JSON specification, the complete visual
tour, and a curated wiki into one installable Material interface. It includes
plain-text and regex search, a rule-based filter builder, regex test dialog,
local Markdown/JSON imports, and portable wiki/search exports.

The same curated guides and full technical references are mirrored to the
[GitHub Wiki](https://github.com/codingmachineedge/qbittorrent-material/wiki).
See [`docs/PAGES.md`](docs/PAGES.md) for local preview, content generation,
publishing, and Wiki synchronization.

![qBittorrent Material documentation landing page](docs/images/site/01-landing-desktop.png)

| Embedded wiki search | Regex builder |
| --- | --- |
| ![Embedded documentation search](docs/images/site/02-wiki-search.png) | ![Regex search builder](docs/images/site/03-regex-builder.png) |

![Responsive mobile documentation](docs/images/site/04-mobile-landing.png)

## Screenshots

Captured from the installed Windows package with a fresh, empty test profile.
The full annotated gallery lives in [`docs/SCREENSHOTS.md`](docs/SCREENSHOTS.md).

![qBittorrent Material dashboard](docs/images/app/01-main-window.png)

| Navigation and toolbar | Status filters |
| --- | --- |
| ![Navigation and Material toolbar](docs/images/app/07-navigation-and-toolbar.png) | ![Status filter sidebar](docs/images/app/03-filter-sidebar.png) |

| Transfer workspace | Torrent properties tabs |
| --- | --- |
| ![Transfer list and filters](docs/images/app/04-transfer-list.png) | ![Torrent property tabs](docs/images/app/05-properties-tabs.png) |

| Toolbar filter controls | Main workspace |
| --- | --- |
| ![Toolbar filter controls](docs/images/app/02-toolbar-and-filter.png) | ![Main application workspace](docs/images/app/08-main-workspace.png) |

![Application status bar](docs/images/app/06-statusbar.png)

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
release asset on a uniquely tagged GitHub prerelease for that exact commit. No
separate Actions artifact is retained.

## License

GPLv3+, matching upstream qBittorrent.
