# Documentation Site and Wiki

The project website is a dependency-free Material 3 application published directly from the repository's `master/docs` directory at:

<https://codingmachineedge.github.io/qbittorrent-material/>

It combines the landing page, screenshot gallery, complete technical corpus, and curated wiki in one installable static site. No server, analytics service, runtime package manager, or search API is required.

## Content model

Canonical content comes from:

- `README.md`
- every Markdown and JSON file under `docs`
- curated guides under `docs/wiki`

Run the deterministic generator after changing any source document:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\generate-pages-content.ps1
```

The command writes `docs/content.generated.js`. The generated bundle is committed intentionally: GitHub Pages can publish the directory as-is without a build workflow or Actions artifact.

## Local preview

Serve the `docs` directory over HTTP so service-worker and file-import behavior match production:

```powershell
py -3 -m http.server 4173 --directory docs
```

Then open <http://127.0.0.1:4173/>.

Opening `index.html` directly still renders the site, but browsers disable some PWA features for `file:` URLs.

## Search modes

The search engine indexes title, path, category, format, headings, and full body text for every embedded document.

- Plain-text search escapes regex syntax by default.
- Regex mode validates patterns and supports case, multiline, dot-all, and Unicode flags.
- Whole-word mode wraps the search expression in word boundaries.
- Filter rules target title, path, category, format, body, or every field.
- Rules combine with all/any logic and support negation.

The regex builder offers token insertion, sample-text testing, match highlighting, and capture-group inspection before applying a pattern.

## Imports and exports

Markdown and text files can be imported as local wiki pages. A versioned wiki JSON bundle can contain multiple pages, while a search-profile JSON file stores the current query, options, and filter rules.

Imported documents are stored only in browser local storage. Rendering escapes raw HTML and rejects unsafe URL schemes. Export before clearing browser storage or moving the workspace to another browser.

## GitHub Wiki synchronization

The separate GitHub Wiki repository is generated from the same canonical content:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\export-github-wiki.ps1 `
  -WikiWorkingTree ..\qbittorrent-material.wiki
```

The exporter writes curated pages, complete references, JSON blueprints rendered as code, navigation, branding, and screenshots. Commit and push those changes from the Wiki repository after review.

## Publishing

GitHub Pages uses `master` and `/docs` as its source. This branch-based configuration is deliberate: it avoids the Pages upload artifact and keeps the existing installer workflow's no-artifact guarantee intact.

The site includes `.nojekyll`, a web manifest, an offline service worker, `404.html` route recovery, robots metadata, and a sitemap. Pages refreshes automatically after `master` changes.

## Site screenshots

![Material landing page](images/site/01-landing-desktop.png)

| Searchable wiki | Regex builder | Mobile landing |
| --- | --- | --- |
| ![Wiki search results](images/site/02-wiki-search.png) | ![Regex builder](images/site/03-regex-builder.png) | ![Mobile landing page](images/site/04-mobile-landing.png) |

## Visual QA checklist

1. Desktop width at 1440×1000 or larger.
2. Tablet width near 900 pixels.
3. Mobile width at 390 pixels.
4. Light, dark, reduced-motion, and high-contrast preferences.
5. Keyboard-only navigation and visible focus states.
6. Plain, regex, invalid-regex, filtered, and empty-result searches.
7. Markdown/JSON import, wiki export, and search-profile round trip.
8. Internal document links, table of contents, copy-code buttons, and gallery lightbox.
