# Project notes (journal + UI rebuild)

- libgit2 (1.9.4, vcpkg static) rejects repository paths longer than ~190
  chars on Windows at INIT time with "path too long", regardless of the
  core.longpaths global-config shim (init-time validation runs before repo
  config exists). Normal profile locations (~85 chars) are far below the
  limit. `GitRepositoryStore::ensureRepository` probes a sibling init before
  ever moving an existing repo aside, so environmental failures never
  displace a healthy journal.
- The journal repos live at `<profile>/data/torrent-journal` and
  `<profile>/data/settings-journal`; WorkspaceManager's repo is separate and
  untouched (`%LOCALAPPDATA%/qBittorrent/qBittorrent/workspace-tabs`).
- Resume-data persistence is still stubbed in this fork; torrents do NOT
  survive restarts. The journal exposes journaledOnlyTorrentIds() +
  restoreMissingTorrents() instead of treating those as deletions.
- 2026-07-13 — Change handoff: scope covers keyboard/focus accessibility for
  header icons, the Add/navigation rail, and filter chips; shared
  menu/toolbar/tray action routing; the staged tray-icon setting (Apply commits,
  Cancel preserves the active value); the About GPL QRC notice; and the
  registered country-flag provider. Flag SVGs remain optional: a missing asset
  intentionally returns a transparent fallback. A package-only CPack smoke
  succeeded; the installer was not executed locally.
