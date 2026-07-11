# Contributing

## Keep changes focused

Use a short-lived `codex/` feature branch, make one logical commit per task, push for CI, merge into `master`, and delete the branch locally and remotely after verification.

## Code and UI expectations

- Preserve persisted preference keys and enum values.
- Keep engine work out of QML; expose it through controllers and models.
- Use the shared Material theme, spacing, typography, icons, and reusable controls.
- Make dialogs responsive and keep wide tables inside their own scroll areas.
- Add accessible names, keyboard behavior, empty states, and error feedback.

## Documentation workflow

1. Update the canonical Markdown or JSON file.
2. Run `scripts/generate-pages-content.ps1`.
3. Open `docs/index.html` through a local HTTP server.
4. Test plain, regex, filtered, imported, and exported searches.
5. Check desktop, tablet, and mobile widths in light and dark themes.
6. Add or refresh screenshots when visible behavior changes.

## Before pushing

Run the relevant build, syntax checks, `git diff --check`, and a visual smoke test. The release workflow will perform the full Windows installer gate for every pushed commit.
