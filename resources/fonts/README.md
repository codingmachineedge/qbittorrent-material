# Fonts

These font files are **bundled into the application binary** as Qt resources
(via `RESOURCES` in `src/quick/CMakeLists.txt`) and loaded at startup with
`QFontDatabase::addApplicationFont(...)`. They are not committed as binaries to
keep the repository lean — fetch them with the steps below before building.

## Required files

| File                           | Family (QML)     | Purpose                                             |
|--------------------------------|------------------|-----------------------------------------------------|
| `Roboto.ttf`                   | `Roboto`         | Primary UI type scale (`Typography` singleton).     |
| `RobotoMono.ttf`               | `Roboto Mono`    | Tabular numerics (speeds / sizes / ratios).         |
| `MaterialSymbolsOutlined.ttf`  | `Material Symbols Outlined` | Every UI icon (rendered by `MDIcon`, codepoints in the `Icons` singleton). |

`Typography.family` = `"Roboto"`, `Typography.monoFamily` = `"Roboto Mono"`, and
`MDIcon` uses the `"Material Symbols Outlined"` family — the family names above
must match exactly or text/glyphs fall back to a system font.

## Fetching

### Roboto & Roboto Mono
Both are Apache-2.0 licensed Google fonts.

```sh
# Roboto (static regular; the variable font also works if you prefer)
curl -L -o Roboto.ttf \
  https://github.com/googlefonts/roboto-2/raw/main/src/hinted/Roboto-Regular.ttf

# Roboto Mono
curl -L -o RobotoMono.ttf \
  https://github.com/googlefonts/RobotoMono/raw/main/fonts/ttf/RobotoMono-Regular.ttf
```

(Or download from <https://fonts.google.com/specimen/Roboto> and
<https://fonts.google.com/specimen/Roboto+Mono> and rename to the filenames above.)

### Material Symbols Outlined
Apache-2.0 licensed. This is the **variable** font — `MDIcon` drives the `FILL`,
`wght`, `GRAD` and `opsz` axes, so use the variable build (not the static
per-weight exports).

```sh
curl -L -o MaterialSymbolsOutlined.ttf \
  https://github.com/google/material-design-icons/raw/master/variablefont/MaterialSymbolsOutlined%5BFILL%2Cwght%2CGRAD%2Copsz%5D.ttf
```

The `Icons` singleton (`src/quick/qml/theme/Icons.qml`) maps every legacy icon id
to a Material Symbols codepoint. If a glyph renders as a "tofu" box, the bundled
font version is missing that symbol — update the font to the latest release
and/or re-verify the codepoint against
<https://fonts.google.com/icons?icon.set=Material+Symbols&icon.style=Outlined>.

## Licenses
Keep the upstream `LICENSE` (Apache-2.0) alongside these files. Roboto, Roboto
Mono and Material Symbols are all Apache-2.0, compatible with the app's GPLv3.
