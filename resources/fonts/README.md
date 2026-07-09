# Fonts

These font files are bundled into the application binary by the `app_assets`
Qt resource collection in `src/CMakeLists.txt`. Because that collection uses
`PREFIX "/"` with `resources/` as its base, the QML loaders use these URLs:

- `qrc:/fonts/Roboto.ttf`
- `qrc:/fonts/RobotoMono.ttf`
- `qrc:/fonts/MaterialSymbolsOutlined.ttf`

## Required files

| File | Family (QML) | Format | Purpose |
| --- | --- | --- | --- |
| `Roboto.ttf` | `Roboto` | Variable (`wdth`, `wght`) | Primary UI type scale (`Typography` singleton). |
| `RobotoMono.ttf` | `Roboto Mono` | Variable (`wght`) | Tabular numerics (speeds, sizes, and ratios). |
| `MaterialSymbolsOutlined.ttf` | `Material Symbols Outlined` | Variable (`FILL`, `GRAD`, `opsz`, `wght`) | UI icons rendered by `MDIcon`. |

`Typography.family` and `Typography.monoFamily` come from the names reported by
their `FontLoader` instances. `MDIcon` loads the Material Symbols face directly.
The literal family-name fallbacks keep text readable if a package is damaged.

## Upstream sources

The checked-in files are unmodified upstream binaries pinned to revisions in
Google's official repositories:

```sh
curl -L -o Roboto.ttf \
  'https://raw.githubusercontent.com/google/fonts/6183fc0d26361f6ddfd6f6b7a736e1467c6d8a43/ofl/roboto/Roboto%5Bwdth%2Cwght%5D.ttf'

curl -L -o RobotoMono.ttf \
  'https://raw.githubusercontent.com/google/fonts/b01086459d9d41ece904d39155b93f7bd3a676f2/ofl/robotomono/RobotoMono%5Bwght%5D.ttf'

curl -L -o MaterialSymbolsOutlined.ttf \
  'https://raw.githubusercontent.com/google/material-design-icons/fe742c4072d4e3b8b899170109d9f710e89f082e/variablefont/MaterialSymbolsOutlined%5BFILL%2CGRAD%2Copsz%2Cwght%5D.ttf'
```

The `Icons` singleton (`src/quick/qml/theme/Icons.qml`) maps every legacy icon ID
to a Material Symbols codepoint. If a glyph renders as a tofu box, verify the
codepoint against the bundled font version before updating either asset.

## Licenses and attribution

- Roboto is distributed by Google Fonts under the SIL Open Font License 1.1;
  see `LICENSE-Roboto.txt`.
- Roboto Mono is distributed by Google Fonts under the SIL Open Font License
  1.1; see `LICENSE-RobotoMono.txt`.
- Material Symbols is distributed by Google under the Apache License 2.0; see
  `LICENSE-Material-Symbols.txt`.

The font names and Google trademarks remain the property of their respective
owners. The binaries are redistributed without modification.
