# Custom Workspace Tabs

The **Workspace** area is a persistent, browser-style collection of plain-text
pages inside qBittorrent Material. Each inner tab owns its content and visual
style, while the application display name, tab order, active tab, and complete
page collection survive restarts.

The workspace is stored in a managed local Git repository. Saving and committing
use the libgit2 library shipped with the application, so the Git command-line
client is not required.

## Open and navigate the workspace

Select **Workspace** in the top-level application tabs or press `Alt+5`. The row
inside that view behaves like a browser tab strip:

- Select a tab to open its page.
- Select **+** or press `Ctrl+T` to create a page.
- Select a tab's close button, middle-click it, or press `Ctrl+W` while the
  Workspace view is active to close it.
- Right-click a tab to customize, duplicate, close the other tabs, or close it.
- Double-click a tab to open its name and appearance dialog.

Closing every tab is supported. The empty view offers a **Create tab** action to
start again. A workspace can contain up to 100 tabs.

![Persistent browser-style Workspace tabs](images/app/09-custom-workspace-tabs.png)

> **LowLevel Workspace Studio** in these screenshots is a user-selected display
> name that demonstrates the rename feature. It is not a separate application
> edition or project rebrand.

![Tab actions anchored to the selected page](images/app/10-tab-context-menu.png)

## Rename the application display

Choose **Workspace > Rename application**, select **Rename app** in the workspace
header, or use the workspace portability menu. The chosen name is restored at
the next launch and is used in the window title, Workspace header, and system
tray tooltip.

This is a display-name customization. It does not rename the executable,
installer entry, application data directory, profile identity, or managed
repository path.

## Customize a page

Right-click a tab and choose **Name & appearance**, or double-click the tab. Each
page stores these settings independently:

- tab name;
- installed font family and the styles supplied by that family;
- font size from 6 to 144 points;
- bold and italic emphasis;
- font color, including alpha.

The color editor is unrestricted: adjust hue, saturation, value, and alpha, or
enter `#RRGGBB` or `#AARRGGBB` directly. Its preview shows the combined font and
color settings before **Apply** is selected.

![Per-tab name, font, style, size, emphasis, and color controls](images/app/11-tab-typography-color.png)

Pages use a plain-text editor. Their repository files use the `.md` extension so
they remain convenient to inspect and diff, but the application does not render
Markdown formatting inside the editor.

## Automatic saving and local history

Editing a page or changing workspace state schedules an atomic save and local
commit after a short debounce. The status below the workspace name changes from
**Changes pending** to **Synced to local Git** and shows the latest short commit
ID. Press `Ctrl+S`, choose **Save & Commit Workspace**, or select the sync button
to flush pending changes immediately.

Choose **Open managed repository** to inspect the folder in the platform file
manager. The repository contains:

```text
workspace-tabs/
|-- .git/                 Complete automatic local history
|-- README.md             Description of the managed repository
|-- workspace.json        Display name, tab order, active tab, and appearance
`-- tabs/
    `-- <tab-uuid>.md      One UTF-8 plain-text page per tab
```

Tab UUIDs keep filenames stable when a tab is renamed. Closing a tab removes its
current page file in the next commit, so earlier content remains recoverable
from Git history. The app writes managed files atomically before creating the
commit.

The normal repository location is selected through Qt's per-user application
data location. Use **Open managed repository** instead of assuming a fixed path;
the exact directory varies by operating system and application profile.

## JSON snapshots and complete repository transfers

The portability menu and the **Workspace** application menu offer two formats:

| Format | Includes | Best for |
| --- | --- | --- |
| Workspace JSON | Display name, active tab, ordered pages, content, timestamps, and appearance | A compact snapshot or exchange with another profile |
| Complete Git repository | `workspace.json`, page files, README, and the entire `.git` history | Backup, migration, or continued version history on another computer |

![Workspace JSON and complete Git repository portability menu](images/app/12-workspace-portability.png)

### Export or import JSON

Choose **Export workspace JSON** and select a `.json` file. The export is a
versioned `qbt-material-workspace` document with all page content embedded.

Choose **Import workspace JSON** to replace the current display name, tabs,
content, and appearance with a snapshot. The confirmation is intentional: JSON
import replaces the live workspace. Every imported tab must contain its full
page text; the repository's internal metadata-only `workspace.json` is not a
portable snapshot and is rejected instead of creating empty pages.

Before applying a valid JSON snapshot, the application flushes and commits all
pending edits in the current workspace. The imported state then becomes another
commit in that repository. If writing or committing the imported state fails,
the previous in-memory workspace is restored and the application attempts to
commit that restoration, so a failed JSON import does not silently discard the
pages that were open before it.

### Export or import the complete repository

Choose **Export complete Git repository** and select a destination directory.
The application first saves pending work, then creates a timestamped child
folder containing a standalone copy of the managed repository and its history.

Choose **Import complete Git repository** and select an exported repository
folder. A repository import replaces both the current workspace and its local
Git history with the selected copy. The importer commits pending work, validates
and stages the incoming copy, and only then switches the managed repository.

After a successful switch, the previous complete repository is retained as a
hidden sibling named `.workspace-backup-<timestamp>-<id>` rather than deleted.
The success message reports its full path. Keep that folder until the imported
workspace and history have been verified; it can be archived as an additional
recovery copy.

If activation or post-activation validation fails, the application restores the
previous repository when possible. When automatic restoration cannot complete,
it blocks further workspace writes and preserves every available copy in the
repository's parent directory, including `.workspace-backup-*`,
`.workspace-import-*`, or `.workspace-failed-import-*` folders. The status and
error message point to that parent directory instead of overwriting a recovery
copy.

At the next launch, if the normal managed repository is missing after an
interrupted import, the application checks the newest valid
`.workspace-backup-*` sibling and restores it automatically.

## Startup recovery

If the process stops after a new tab body reaches `tabs/<uuid>.md` but before
the matching metadata update reaches `workspace.json`, the next launch sees
that the body is untracked, adopts it as a **Recovered tab**, and commits it. If
the manifest already records a close but the formerly tracked body remains, the
next launch completes that intentional close instead of resurrecting the tab.
The embedded Git index makes the two crash windows unambiguous.

Existing workspace files are never overwritten just because they fail startup
validation. If the managed repository exists but its metadata or page files are
invalid, the application first moves the complete directory to a hidden sibling
named `.workspace-recovery-<timestamp>-<id>`. It then creates a fresh Welcome
workspace and reports the preserved path in the Workspace status.

If the invalid directory cannot be moved safely, it is left untouched. The app
shows an in-memory Welcome page but blocks workspace saves and Git operations so
the original files cannot be replaced accidentally. The editor and mutating
actions visibly switch to read-only while navigation and repository inspection
remain available.

The backup and recovery directories live beside the normal `workspace-tabs`
folder and may be hidden by the operating-system file manager. Use **Open managed
repository**, move up to its parent directory, and enable hidden items to locate
them. Do not remove a `.workspace-backup-*`, `.workspace-recovery-*`,
`.workspace-import-*`, or `.workspace-failed-import-*` directory until the
active workspace is verified and any needed files or history have been copied
somewhere safe.

The restored-state capture below shows the renamed application, ordered tabs,
active page, typography, and content available again after relaunch.

![Workspace state restored after relaunch](images/app/13-restored-workspace.png)

## Validation and privacy

Workspace data stays on the local computer unless it is explicitly exported or
shared. There is no remote push, cloud sync, or repository host configured by
the Workspace feature.

Imports enforce the current schema, unique UUID tab identifiers, valid colors,
font-size bounds, and these resource limits:

- 100 tabs;
- 4 MB of text per page;
- 32 MB per workspace JSON file;
- 256 MB per complete repository transfer.

Unsafe symbolic-link or reparse-point paths are rejected, as are repository
folders nested inside the managed repository. Imported JSON and repository data
should still be treated like any other local file: review the source before
replacing a workspace you care about.

## Keyboard reference

| Shortcut | Action |
| --- | --- |
| `Alt+5` | Open the top-level Workspace view |
| `Ctrl+T` | Create a workspace tab |
| `Ctrl+W` | Close the active workspace tab when Workspace is active |
| `Ctrl+S` | Save and commit pending workspace changes when Workspace is active |

See the [Workspace Tabs wiki guide](wiki/Workspace-Tabs.md) for the short
task-oriented walkthrough and [Troubleshooting](wiki/Troubleshooting.md) for
recovery guidance.
