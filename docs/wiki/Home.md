# qBittorrent Material Wiki

Welcome to the living handbook for qBittorrent Material: a C++20 and Qt 6/QML rewrite of qBittorrent with a fully Material interface.

This wiki is maintained in the main repository so the GitHub Wiki and the searchable documentation site share the same source. Use the site search for plain text, regular expressions, field filters, saved search profiles, and imported Markdown.

![qBittorrent Material dashboard](images/app/01-main-window.png)

## Start here

- [Getting started](Getting-Started.md) — install a prerelease, build locally, and launch the app.
- [Interface tour](Interface-Tour.md) — learn the navigation, filters, transfer table, properties, and status areas.
- [Search, filters, and portability](Search-Import-Export.md) — use regex search, the filter builder, and JSON/Markdown import and export.
- [Releases and automation](Releases.md) — understand the installer pipeline and per-push prereleases.
- [Architecture](Architecture.md) — explore the engine, controller, model, and QML boundaries.
- [Troubleshooting](Troubleshooting.md) — solve common build, packaging, launch, and documentation issues.
- [Contributing](Contributing.md) — keep code, visuals, and docs consistent.

## Project principles

1. Preserve qBittorrent behavior and configuration compatibility.
2. Keep every first-party window and dialog in the Material design system.
3. Make repeatable builds and self-contained Windows installers the default path.
4. Treat documentation and screenshots as tested product surfaces.
