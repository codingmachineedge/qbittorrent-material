# qBittorrent Material visual tour

These screenshots were captured from the installed Windows package using a fresh,
empty test profile. They show the shipped Qt Quick Material interface without
including any personal torrents, paths, or tracker data.

## Full dashboard

![Full qBittorrent Material dashboard](images/app/01-main-window.png)

## Navigation and filtering

The top-level menus, Material toolbar, global filter, and transfer-list filter
are visible in the navigation strip.

![Navigation and Material toolbar](images/app/07-navigation-and-toolbar.png)

![Toolbar and transfer filters](images/app/02-toolbar-and-filter.png)

The status sidebar exposes the familiar qBittorrent workflow states while using
the Material palette and spacing system.

![Status filters sidebar](images/app/03-filter-sidebar.png)

## Transfer workspace

The main table starts empty in a fresh profile and retains the prominent search,
column, and overflow controls expected in a production torrent client.

![Transfer list](images/app/04-transfer-list.png)

The lower panel groups per-torrent data into General, Trackers, Peers, HTTP
Sources, Content, and Speed tabs.

![Torrent property tabs](images/app/05-properties-tabs.png)

![Main workspace](images/app/08-main-workspace.png)

## Persistent custom workspace

These views were captured from the verified installed application. **LowLevel
Workspace Studio** is a user-defined display name used to demonstrate renaming;
it is still qBittorrent Material.

The Workspace combines a browser-style tab strip, one plain-text page per tab,
per-tab appearance, and visible local Git status.

![Custom Workspace tabs](images/app/09-custom-workspace-tabs.png)

Right-clicking a tab keeps page actions anchored to that tab in a compact,
bordered menu.

![Workspace tab context menu](images/app/10-tab-context-menu.png)

Each page owns its name, installed font family and style, 6–144 point size,
bold/italic state, and unrestricted HSV/alpha or hexadecimal font color.

![Tab typography and color editor](images/app/11-tab-typography-color.png)

The portability menu separates compact JSON snapshots from complete Git
repository transfers that include history.

![Workspace import and export menu](images/app/12-workspace-portability.png)

After relaunch, the user display name, tabs, active page, content, typography,
and color return from the managed repository.

![Workspace restored after relaunch](images/app/13-restored-workspace.png)

## Status information

The compact status bar remains visible at the bottom of the application shell.

![Status bar](images/app/06-statusbar.png)

## Documentation experience

The GitHub Pages landing page presents the application at 80% of the viewport
on large screens, then reflows navigation, calls to action, and screenshots for
phones and tablets.

![Material documentation landing page](images/site/01-landing-desktop.png)

The complete project corpus is searchable inside the page. Results are grouped
by document and highlight the matching text without sending the query anywhere.

![Embedded wiki search](images/site/02-wiki-search.png)

The regex builder includes token shortcuts, flags, sample text, live match
highlighting, and portable pattern import/export.

![Regex builder dialog](images/site/03-regex-builder.png)

![Mobile documentation landing page](images/site/04-mobile-landing.png)
