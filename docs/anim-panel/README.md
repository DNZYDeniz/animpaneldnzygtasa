# Anim Panel

This document describes the shipped AnimPanel runtime for classic GTA San Andreas singleplayer.

Current runtime:
- Native `.asi` plugin only
- No CLEO dependency
- No Mod Loader dependency
- Runtime data lives in `AnimPanel\` at the GTA SA root

Required files:
- `AnimPanel.asi`
- `AnimPanel\data\anim-catalog.json`
- `AnimPanel\data\favorites.json`
- `AnimPanel\data\recents.json`
- `AnimPanel\fonts\Rajdhani-Bold.ttf`

Game target:
- GTA San Andreas 1.0 US

Recommended environment:
- ASI Loader installed
- SilentPatch SA installed

Controls:
- `F8` toggle panel
- `ESC` close panel
- `8 / 2` move selection
- `4 / 6` page
- `5` play selected animation

Catalog notes:
- The animation catalog is generated from the official open.mp animation dataset.
- The shipped catalog is filtered to remove obvious vehicle-only, paired-scene, prop-dependent, and pose-transition-risky entries that are not safe for solo CJ preview.
