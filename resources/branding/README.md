# qBittorrent Material brand assets

This directory contains the original visual identity for the Material desktop
rewrite. The mark combines three product ideas in one compact shape:

- the white circular bowl and descending stroke form a lowercase **q**;
- the cyan center arrow expresses **download motion**;
- three cyan points represent **connected peers**.

The artwork was created specifically for this project and does not reuse the
official qBittorrent logo or third-party artwork.

## Files

| Asset | Best use |
| --- | --- |
| `logo-mark.svg` | App icon, favicon, social avatar, compact navigation |
| `logo-horizontal.svg` | Hero areas, repository headers, release pages |
| `logo-monochrome.svg` | Single-color printing, embossing, symbolic UI |

The documentation site keeps a byte-for-byte copy of `logo-mark.svg` at
`docs/assets/logo-mark.svg`, allowing GitHub Pages to serve it without coupling
the site to the application resource path.

## Palette

| Token | Value | Purpose |
| --- | --- | --- |
| Primary indigo | `#4F46E5` | Core Material identity |
| Deep indigo | `#4338CA` | Gradient depth |
| Cyan | `#0891B2` | Gradient destination |
| Bright cyan | `#67E8F9` | Motion and peer accents |
| Ink | `#172554` | Monochrome default |
| White | `#FFFFFF` | High-contrast q silhouette |

## Usage rules

- Keep clear space around the mark equal to at least one node diameter.
- Use the full-color mark at **24 CSS pixels or larger**. At smaller sizes, use
  the monochrome asset so the peer nodes remain crisp.
- Do not rotate, stretch, recolor individual pieces, or remove the q tail.
- The full-color mark is light/dark safe because its contrast surface is part
  of the asset. The horizontal lockup also carries its own dark surface.
- `logo-monochrome.svg` uses `currentColor`; set CSS `color` on the embedded SVG
  when a different one-color treatment is needed.
- Preserve the SVG `<title>` and `<desc>` when embedding inline. When the logo
  is decorative, use an empty HTML `alt` attribute to avoid duplicate labels.

## Qt resource path

The application bundles the mark at `:/branding/logo-mark.svg`. Qt uses it as
the global window icon and as the system-tray fallback when platform-specific
tray artwork is unavailable.

## Licensing

These assets are distributed with the project under
`GPL-3.0-or-later`, consistent with the repository source.
