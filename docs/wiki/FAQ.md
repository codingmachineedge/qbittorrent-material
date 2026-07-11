# Frequently Asked Questions

## Is this a theme for upstream qBittorrent?

No. It is a ground-up Qt 6/QML interface and application rewrite that aims for feature-for-feature behavior while using a Material design system everywhere.

## Does the site call a search service?

No. The complete documentation corpus is embedded in the static site. Search, regex evaluation, filters, imports, and exports run locally in the browser.

## Are imported documents uploaded?

No. Imported files stay in local browser storage unless you explicitly export and share a bundle.

## Why is each push a prerelease?

The project favors traceable test builds. Each release points at one commit and contains the exact installer that passed the automated smoke gate.

## Why can the first Windows build take a long time?

Qt downloads and vcpkg native dependencies dominate a cold build. Caches make later runs much faster.

## Where should I report a problem?

Open a GitHub issue with the build tag, operating system, expected behavior, actual behavior, and a screenshot or log excerpt that does not contain private paths or tracker data.
