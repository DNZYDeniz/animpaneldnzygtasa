# GTA SA AnimPanel

Singleplayer animation browser for classic GTA San Andreas.

This release is native-only:
- No CLEO required
- No CLEO 4 or CLEO 5 required
- No Mod Loader required

## Requirements

- GTA San Andreas 1.0 US
- An ASI Loader

Recommended:
- SilentPatch SA

Not required:
- CLEO
- Mod Loader
- Visual C++ Redistributable for this build

## Included Files

- `AnimPanel.asi`
- `AnimPanel\data\anim-catalog.json`
- `AnimPanel\data\favorites.json`
- `AnimPanel\data\recents.json`
- `AnimPanel\fonts\Rajdhani-Bold.ttf`

## Installation

Copy these into the GTA San Andreas game root:

1. `AnimPanel.asi`
2. The whole `AnimPanel` folder

Final layout should look like this:

```text
gta_sa.exe
AnimPanel.asi
AnimPanel\
  data\
    anim-catalog.json
    favorites.json
    recents.json
  fonts\
    Rajdhani-Bold.ttf
```

## Required External Components

### ASI Loader
The game must have an ASI Loader. Any standard GTA SA ASI Loader is fine.

Examples:
- Ultimate ASI Loader
- The ASI loading setup many users already have through SilentPatch or other GTA SA mod stacks

### SilentPatch SA
Not mandatory, but recommended for a cleaner GTA SA 1.0 US runtime.

## Controls

- `F8` open or close the panel
- `ESC` close the panel
- `8 / 2` move up or down
- `4 / 6` page
- `5` preview animation

## Notes

- The panel blocks gameplay input while open.
- Favorites and recents are saved automatically in `AnimPanel\data\`.
- The shipped catalog is filtered for solo CJ preview safety and does not try to include every raw game animation.

## Source Layout

- `native-src/anim-panel/` native source and Visual Studio solution
- `tools/` catalog generator and helper files
- `docs/anim-panel/` implementation notes

## Build From Source

Open:

- `native-src/anim-panel/AnimPanel.sln`

Build:

- `Release GTASA | Win32`

Output:

- `native-src/anim-panel/output/AnimPanel.asi`
