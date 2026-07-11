# qBittorrent Material visual tour

The native Windows captures use an isolated, empty test profile. They show the
real Qt Quick interface without personal torrents, paths, tracker data, or
account details. The stable `01`–`13` filenames are retained for existing links;
their captions below describe the current full-screen capture set.

## Capture matrix

| File | Target | Theme | Surface |
| --- | ---: | --- | --- |
| `01-main-window.png` | 960×900 | Light | Transfers and complete shell |
| `02-toolbar-and-filter.png` | 960×900 | Dark | Transfers and complete shell |
| `03-filter-sidebar.png` | 960×640 | Light | Compact Transfers shell |
| `04-transfer-list.png` | 960×768 | Light | Transfers table and filters |
| `05-properties-tabs.png` | 960×768 | Dark | Transfers properties |
| `06-statusbar.png` | 960×900 | Light | Execution Log |
| `07-navigation-and-toolbar.png` | 960×900 | Dark | Search |
| `08-main-workspace.png` | 960×900 | Light | RSS reader |
| `09-custom-workspace-tabs.png` | 960×900 | Light | Personal Workspace |
| `10-tab-context-menu.png` | 960×900 | Light | Options |
| `11-tab-typography-color.png` | 960×900 | Dark | Options |
| `12-workspace-portability.png` | 960×768 | Light | Download from URLs dialog |
| `13-restored-workspace.png` | 960×768 | Dark | About qBittorrent dialog |

## Shared desktop shell

Every workspace uses the same compact geometry: a 64px command bar, persistent
248px grouped navigation, 24px content gutters, flat bordered panels with 24px
corners, 40px controls, and a 32px status footer. Operational values use
monospace type, while pale-blue selection and explicit labels keep state clear.

System mode follows the operating system. Light and Dark can also be selected
explicitly; the first two Transfers captures show both palettes.

![Light Transfers workspace](images/app/01-main-window.png)

![Dark Transfers workspace](images/app/02-toolbar-and-filter.png)

The 960×640 capture verifies that the native desktop hierarchy remains usable
at the supported compact window size without pretending to be a phone layout.

![Compact Transfers workspace](images/app/03-filter-sidebar.png)

## Transfers and properties

Transfers keeps state/category navigation, filtering, row actions, progress,
speed, peers, ETA, and selection in one dense operational surface.

![Transfers table and filters](images/app/04-transfer-list.png)

The lower detail region preserves General, Trackers, Peers, HTTP Sources,
Content, and Speed without leaving the selected torrent context.

![Dark Transfers properties](images/app/05-properties-tabs.png)

## Search, RSS, and Execution Log

Execution Log provides timestamped runtime messages with text and severity
filters while retaining the persistent shell and status footer.

![Execution Log workspace](images/app/06-statusbar.png)

Search combines query controls, installed-plugin scope, result filtering, and
download actions in the dark palette.

![Dark Search workspace](images/app/07-navigation-and-toolbar.png)

RSS uses feed/article panes and keeps release actions close to automation
context.

![RSS reader workspace](images/app/08-main-workspace.png)

## Personal Workspace

Workspace provides persistent browser-style plain-text pages with per-tab
appearance and local Git-backed history. It is a first-class destination beside
Transfers, Search, RSS, and Execution Log.

![Persistent personal Workspace](images/app/09-custom-workspace-tabs.png)

## Options and dialogs

Options keeps its settings categories and staged Apply/Cancel workflow inside a
large Material dialog. The paired captures verify Light and Dark rendering.

![Options dialog in Light mode](images/app/10-tab-context-menu.png)

![Options dialog in Dark mode](images/app/11-tab-typography-color.png)

The remaining captures cover shared dialog foundations: 24px corners,
restrained elevation, 40px actions, visible labels, and keyboard-safe focus.

![Download from URLs dialog](images/app/12-workspace-portability.png)

![About qBittorrent dialog in Dark mode](images/app/13-restored-workspace.png)

## Documentation experience

The GitHub Pages landing page presents the application at 80% of the viewport
on large screens, then reflows navigation, calls to action, and screenshots for
phones and tablets.

![Material documentation landing page](images/site/01-landing-desktop.png)

The complete project corpus is searchable inside the page. Results are grouped
by document and highlight matching text without sending the query anywhere.

![Embedded wiki search](images/site/02-wiki-search.png)

The regex builder includes token shortcuts, flags, sample text, live match
highlighting, and portable pattern import/export.

![Regex builder dialog](images/site/03-regex-builder.png)

![Mobile documentation landing page](images/site/04-mobile-landing.png)
