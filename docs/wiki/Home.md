# qBittorrent Material Wiki

Welcome to the living handbook for qBittorrent Material: a native C++20 and Qt 6/QML rewrite of qBittorrent with a data-first Material interface.

This wiki is maintained in the main repository so the GitHub Wiki and the searchable documentation site share the same source. Use the site search for plain text, regular expressions, field filters, saved search profiles, and imported Markdown.

The completed desktop shell uses a 64px command bar, persistent 248px navigation, and a compact 32px footer. Transfers, Search, RSS, Execution Log, and the personal Workspace are native Qt Quick workspaces. Their content follows a consistent 24px page gutter, flat 24px-radius panels, 40px controls, and compact tables and split views. Choose **System**, **Light**, or **Dark** without changing the underlying workflows or data models.

![qBittorrent Material dashboard](images/app/01-main-window.png)

## Start here

- [Getting started](Getting-Started.md) — install a prerelease, build locally, and launch the app.
- [Interface tour](Interface-Tour.md) — learn the navigation, filters, transfer table, properties, and status areas.
- [Workspace tabs](Workspace-Tabs.md) — create persistent pages, customize typography, and move snapshots or complete local Git history.
- [Search, filters, and portability](Search-Import-Export.md) — use regex search, the filter builder, and JSON/Markdown import and export.
- [Releases and automation](Releases.md) — understand the installer pipeline and per-push prereleases.
- [Architecture](Architecture.md) — explore the engine, controller, model, and QML boundaries.
- [Troubleshooting](Troubleshooting.md) — solve common build, packaging, launch, and documentation issues.
- [Contributing](Contributing.md) — keep code, visuals, and docs consistent.

## Project principles

1. Preserve qBittorrent behavior and configuration compatibility.
2. Keep every native workspace and first-party workflow in one coherent Material design system.
3. Make repeatable builds and self-contained Windows installers the default path.
4. Treat documentation and screenshots as tested product surfaces.
5. Keep personal workspace data local and portable by default.
