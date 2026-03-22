GTA SA AnimPanel
================

Animation browser for classic GTA San Andreas singleplayer.

Requirements:
- GTA San Andreas 1.0 US
- ASI Loader

Recommended:
- SilentPatch SA

Not required:
- CLEO 4
- CLEO 5
- Mod Loader

Files included:
- AnimPanel.asi
- AnimPanel\data\anim-catalog.json
- AnimPanel\data\favorites.json
- AnimPanel\data\recents.json
- AnimPanel\data\settings.ini if created by the mod
- AnimPanel\fonts\Rajdhani-Bold.ttf

Installation:
1. Copy AnimPanel.asi into your GTA San Andreas game folder.
2. Copy the whole AnimPanel folder into your GTA San Andreas game folder.

Final layout:

gta_sa.exe
AnimPanel.asi
AnimPanel\
  data\
    anim-catalog.json
    favorites.json
    recents.json
  fonts\
    Rajdhani-Bold.ttf

Controls:
- F8 open or close the panel
- ESC close the panel
- 8 / 2 move up or down
- 4 / 6 page
- 5 preview animation
- Settings > Auto Play lets the current selection preview automatically while browsing

Notes:
- Gameplay input is blocked while the panel is open.
- Favorites and recents are saved automatically.
- Auto Play setting is saved automatically.
- Faulted animations are logged to AnimPanel\logs\faultanims.txt.
- If F8 does nothing, your game is usually missing an ASI Loader.
