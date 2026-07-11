# Interface Tour

The application opens at roughly 80% of the active screen's available size, centered, and adapts when moved to another display. The same layout remains user-resizable.

## Application shell

![Full dashboard](images/app/01-main-window.png)

The top app area combines native menus, Material actions, global filtering, and transfer controls. Central tabs keep Transfers primary while Search, RSS, and Execution Log can be enabled when needed.

![Navigation and toolbar](images/app/07-navigation-and-toolbar.png)

## Find and organize transfers

The sidebar groups status, category, tag, tracker, and tracker-health filters. The toolbar filter narrows the transfer table without changing the session.

![Filter sidebar](images/app/03-filter-sidebar.png)

![Transfer workspace](images/app/04-transfer-list.png)

## Inspect torrent details

The lower responsive panel exposes General, Trackers, Peers, HTTP Sources, Content, and Speed tabs. Each data-heavy view owns its scrolling so the surrounding shell does not drift horizontally.

![Properties tabs](images/app/05-properties-tabs.png)

![Main workspace](images/app/08-main-workspace.png)

## Build a personal workspace

The always-available **Workspace** top-level tab contains a second,
browser-style tab strip. Each tab opens one plain-text page and remembers its
name, content, position, selected state, and individual typography. Create a
page with **+** or `Ctrl+T`; close it with the tab button, a middle-click, or
`Ctrl+W` while Workspace is active.

Right-click a tab to edit its name and appearance, duplicate it, close the other
tabs, or close it. The appearance dialog supports installed font families and
styles, 6–144 point sizes, bold, italic, and an unrestricted HSV/alpha or hex
font color. The Workspace header also exposes application renaming, local Git
status, manual save-and-commit, repository access, and portability actions.

Read [Workspace Tabs](Workspace-Tabs.md) for the complete workflow.

<!-- WORKSPACE_SCREENSHOT_SLOTS
Insert verified installed-app captures when available:
images/app/09-custom-workspace-tabs.png
images/app/10-tab-context-menu.png
images/app/11-tab-typography-color.png
images/app/12-workspace-portability.png
-->

## Session status

Connection state, alternative speed state, free disk space, and live transfer rates remain visible in the compact footer.

![Status bar](images/app/06-statusbar.png)
