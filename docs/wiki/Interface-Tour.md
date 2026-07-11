# Interface Tour

The application opens at roughly 80% of the active screen's available size, centered, and adapts when moved to another display. The same layout remains user-resizable.

## Application shell

![Full dashboard](images/app/01-main-window.png)

The data-first shell reserves stable space for three pieces of native chrome:

- A 64px command bar holds the application menu, frequent torrent actions, live transfer rates, Options, and UI locking.
- A persistent 248px navigation surface switches among Transfers, Search, RSS reader, Execution Log, and Workspace. When Transfers is active, its lower section also hosts the status, category, tag, tracker, and tracker-health filters.
- A compact 32px footer keeps session connectivity, alternative-speed state, free disk space, DHT information, and transfer rates visible without displacing the active workspace.

Each workspace uses 24px page gutters and a dominant flat, outlined panel with a 24px radius. Controls use a consistent 40px target, while tables, log rows, filter lists, and split panes stay compact enough for desktop data work. The same geometry is available in **System**, **Light**, and **Dark** themes.

![Search workspace with persistent navigation and command bar](images/app/07-navigation-and-toolbar.png)

## Native workspaces

- **Transfers** is the primary operational workspace. Its compact torrent table and properties surface share a persisted vertical split.
- **Search** runs installed search plugins, keeps query history, and places each query in its own results tab.
- **RSS reader** combines feed navigation, matching articles, previews, refresh controls, and automated-download rules.
- **Execution Log** exposes the general application log and blocked-IP history without leaving the shell.
- **Workspace** provides persistent personal pages backed by the managed local Git repository described below.

Search, RSS, and Execution Log remain optional features: selecting one enables and loads its native QML workspace. Options is available from both the command bar and navigation and opens the native preferences surface.

![RSS reader workspace](images/app/08-main-workspace.png)

## Find and organize transfers

The Transfers section of the persistent navigation groups status, category, tag, tracker, and tracker-health filters. The filter field aligned with the compact transfer table narrows its shared proxy model without changing the session.

![Filter sidebar](images/app/03-filter-sidebar.png)

![Transfer workspace](images/app/04-transfer-list.png)

## Inspect torrent details

The lower panel exposes General, Trackers, Peers, HTTP Sources, Content, and Speed tabs. Each data-heavy view owns its scrolling, and the persisted vertical split lets the compact transfer table retain useful height without making the shell drift horizontally.

![Properties tabs](images/app/05-properties-tabs.png)

## Build a personal workspace

The always-available **Workspace** navigation destination contains a second,
browser-style tab strip inside the same native panel system. Each tab opens one
plain-text page and remembers its name, content, position, selected state, and
individual typography. Create a page with **+** or `Ctrl+T`; close it with the
tab button, a middle-click, or `Ctrl+W` while Workspace is active.

Right-click a tab to edit its name and appearance, duplicate it, close the other
tabs, or close it. The appearance dialog supports installed font families and
styles, 6–144 point sizes, bold, italic, and an unrestricted HSV/alpha or hex
font color. The Workspace header also exposes application renaming, local Git
status, manual save-and-commit, repository access, and portability actions.

Read [Workspace Tabs](Workspace-Tabs.md) for the complete workflow.

![Persistent Workspace tabs](images/app/09-custom-workspace-tabs.png)

## Configure the application

Options keeps the nine settings categories in one large native dialog. Changes
are staged until **Apply** or **OK**, while **Cancel** discards the current edit
set. The same hierarchy and semantic colors remain readable in both palettes.

| Options · Light | Options · Dark |
| --- | --- |
| ![Options dialog in Light mode](images/app/10-tab-context-menu.png) | ![Options dialog in Dark mode](images/app/11-tab-typography-color.png) |

## Shared dialogs

| Download from URLs | About |
| --- | --- |
| ![Download from URLs dialog](images/app/12-workspace-portability.png) | ![About qBittorrent in Dark mode](images/app/13-restored-workspace.png) |

## Session status

Connection state, alternative-speed state, free disk space, DHT status, and live transfer rates remain visible in the 32px footer.

![Execution Log workspace with the shared status footer](images/app/06-statusbar.png)
