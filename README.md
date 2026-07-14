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
- **UI:** Qt Quick Controls 2, Material style, System + Light + Dark themes
- **Goal:** feature-for-feature clone of qBittorrent's desktop client, rebuilt as Material Design

## Status

The native desktop interface is fully rewritten in Qt Quick/Material: the shell,
all five workspaces, settings, shared controls, and dialogs use the same design
system in System, Light, and Dark modes. Backend parity and edge-case coverage
remain tracked in [`docs/FEATURE_SPEC.md`](docs/FEATURE_SPEC.md).

## Native Material workspace

The rewritten desktop shell follows one compact, data-first system: a 64px
command bar, persistent 248px workspace navigation, 24px content gutters,
flat bordered panels with 24px corners, 40px controls, and a 32px status
footer. Transfers, Search, RSS, Execution Log, and the personal Workspace stay
one click away; Options opens as the shared settings surface.

System theme follows the operating system, with explicit Light and Dark modes
available. The light palette uses the supplied cool-neutral Material tokens;
the dark palette uses their Google Material counterparts. Both retain visible
focus, semantic transfer states, and compact monospace operational data.

The shell's menu bar, header commands, and Material tray menu invoke shared
actions. Header icon controls, the Add/navigation rail, and status-filter chips
are keyboard reachable, expose descriptive accessible names, and show a clear
focus ring. The Behavior-page tray-icon choice is staged with the Options
transaction: **Apply** commits and refreshes the native tray icon, while
**Cancel** preserves the active icon.

The About dialog renders its bundled GPL notice directly from the QRC resource
bundle. Peer-country flags resolve through
the registered `image://flags` provider; flag SVGs are optional, and an absent
asset intentionally becomes a transparent placeholder rather than a broken
image.

## Persistent custom workspace

The built-in **Workspace** adds browser-style tabs for personal plain-text pages.
Tab names, order, content, active page, and per-tab typography are restored on
launch. Right-click a tab to choose any installed font and style, a 6–144 point
size, bold or italic emphasis, and an unrestricted font color with alpha.

The application display name can be changed without changing the executable or
profile identity. Workspace edits save atomically and commit automatically to a
managed local Git repository through bundled libgit2—no separate Git install or
remote service is required. Export a compact JSON snapshot or the complete Git
repository with its history, and import either format from the Workspace menu.

See [Custom Workspace Tabs](docs/WORKSPACE_TABS.md) for the complete guide.

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

Captured from the native Windows build with an isolated, empty test profile.
The complete 17-capture visual tour and capture matrix live in
[`docs/SCREENSHOTS.md`](docs/SCREENSHOTS.md). Captures `14`–`17` exercise
the compact 960×640 Split Dock and Card Flow layouts in Light and Dark themes.
The deterministic capture path saves each PNG at its documented logical target.

![Light Transfers workspace in the complete qBittorrent Material shell](docs/images/app/01-main-window.png)

| Dark Transfers workspace | Compact desktop shell |
| --- | --- |
| ![Dark theme Transfers workspace](docs/images/app/02-toolbar-and-filter.png) | ![Compact 960px Transfers layout](docs/images/app/03-filter-sidebar.png) |

| Transfer table | Dark torrent properties |
| --- | --- |
| ![Transfers table and filters](docs/images/app/04-transfer-list.png) | ![Dark Transfers properties panel](docs/images/app/05-properties-tabs.png) |

| Execution Log | Search |
| --- | --- |
| ![Execution Log workspace](docs/images/app/06-statusbar.png) | ![Dark Search workspace](docs/images/app/07-navigation-and-toolbar.png) |

| RSS | Personal Workspace |
| --- | --- |
| ![RSS reader workspace](docs/images/app/08-main-workspace.png) | ![Persistent personal Workspace](docs/images/app/09-custom-workspace-tabs.png) |

| Options · Light | Options · Dark |
| --- | --- |
| ![Options dialog in Light mode](docs/images/app/10-tab-context-menu.png) | ![Options dialog in Dark mode](docs/images/app/11-tab-typography-color.png) |

| Download from URLs | About |
| --- | --- |
| ![Download from URLs dialog](docs/images/app/12-workspace-portability.png) | ![About qBittorrent dialog in Dark mode](docs/images/app/13-restored-workspace.png) |

| Split Dock · Light | Split Dock · Dark |
| --- | --- |
| ![Compact Split Dock layout in Light mode](docs/images/app/14-split-dock-compact.png) | ![Compact Split Dock layout in Dark mode](docs/images/app/15-split-dock-dark-compact.png) |

| Card Flow · Light | Card Flow · Dark |
| --- | --- |
| ![Compact Card Flow layout in Light mode](docs/images/app/16-card-flow-compact.png) | ![Compact Card Flow layout in Dark mode](docs/images/app/17-card-flow-dark-compact.png) |

## Building

On Windows, the helper provisions Git, CMake, Ninja, Python, Qt 6.8.3, vcpkg,
and the remaining repository-local dependencies, then builds with MSVC 2022:

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

## Continuous releases

Every branch push runs a Windows installer build on GitHub Actions using
`windows-2022` and Qt 6.8.3. A successful run publishes the installer as a
release asset on a uniquely tagged, full GitHub release for that exact
commit. No separate Actions artifact is retained.

## License

GPLv3+, matching upstream qBittorrent.
