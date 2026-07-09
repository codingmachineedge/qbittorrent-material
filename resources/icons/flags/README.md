# Country flags

369 country / region flag SVGs, reused **verbatim** from upstream qBittorrent
(there is no Material-Design equivalent for national flags). They are bundled as
Qt resources and served to QML through the C++ `FlagImageProvider`
(`src/quick/theme/iconprovider.{h,cpp}`) under the `image://flags/` scheme:

```qml
Image {
    source: "image://flags/" + peer.country   // e.g. "image://flags/us"
    sourceSize: Qt.size(20, 15)
}
```

The provider lower-cases the id, strips any extension, and loads `<code>.svg`
from this directory, rendering it to the requested size (default 20×15). A miss
yields a transparent placeholder (logged via `qbt.theme`), never a broken image.

## Filename pattern

Files are named by **lowercase code + `.svg`**:

- **ISO 3166-1 alpha-2** country codes — the vast majority, e.g.
  `ad.svg ae.svg af.svg … us.svg … zw.svg` (all 249 assigned two-letter codes).
- **Subdivision / regional** codes using a hyphen, e.g.
  `gb-eng.svg gb-sct.svg gb-wls.svg gb-nir.svg` (Home Nations),
  `es-ct.svg es-ga.svg es-pv.svg` (Spanish autonomous communities),
  `sh-ac.svg sh-hl.svg sh-ta.svg` (Saint Helena dependencies).
- **Organisation / grouping** codes, e.g. `arab.svg cefta.svg eac.svg`.

The full set matches upstream qBittorrent's `src/icons/flags/` directory. When a
peer's country cannot be resolved the provider is asked for an unknown id and
returns the transparent placeholder — callers do not need a special-case.

## Fetching

The flags come from the **flag-icons** project (MIT licensed), which is what
upstream qBittorrent vendors.

```sh
# Clone flag-icons and copy the 4x3 SVGs here:
git clone --depth 1 https://github.com/lipis/flag-icons.git /tmp/flag-icons
cp /tmp/flag-icons/flags/4x3/*.svg .

# Then add the regional/grouping SVGs that qBittorrent carries but flag-icons may
# name differently, by copying them from a qBittorrent checkout:
cp /path/to/qBittorrent/src/icons/flags/*.svg .
```

Alternatively copy the entire directory straight from an existing qBittorrent
source tree (`src/icons/flags/*.svg`) to guarantee the exact 369-file set the
app expects.

## Expected files (pattern reference)

```
# ISO 3166-1 alpha-2 (249):
ad ae af ag ai al am ao aq ar as at au aw ax az
ba bb bd be bf bg bh bi bj bl bm bn bo bq br bs bt bv bw by bz
ca cc cd cf cg ch ci ck cl cm cn co cr cu cv cw cx cy cz
de dj dk dm do dz  ec ee eg eh er es et  fi fj fk fm fo fr
ga gb gd ge gf gg gh gi gl gm gn gp gq gr gs gt gu gw gy
hk hm hn hr ht hu  id ie il im in io iq ir is it  je jm jo jp
ke kg kh ki km kn kp kr kw ky kz  la lb lc li lk lr ls lt lu lv ly
ma mc md me mf mg mh mk ml mm mn mo mp mq mr ms mt mu mv mw mx my mz
na nc ne nf ng ni nl no np nr nu nz  om  pa pe pf pg ph pk pl pm pn pr ps pt pw py
qa  re ro rs ru rw  sa sb sc sd se sg sh si sj sk sl sm sn so sr ss st sv sx sy sz
tc td tf tg th tj tk tl tm tn to tr tt tv tw tz  ua ug um us uy uz
va vc ve vg vi vn vu  wf ws  ye yt  za zm zw

# Regional / subdivision:
gb-eng gb-nir gb-sct gb-wls  es-ct es-ga es-pv  sh-ac sh-hl sh-ta

# Groupings:
arab cefta eac
```

## License
flag-icons is MIT licensed; keep its `LICENSE` alongside these SVGs. MIT is
compatible with the app's GPLv3.
