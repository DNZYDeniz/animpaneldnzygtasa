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
- AnimPanel\images\star.png

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
  images\
    star.png

Controls:
- F8 open or close the panel
- ESC close the panel
- 8 / 2 move up or down
- 4 / 6 page
- 5 preview animation
- F7 add or remove the current animation from favorites
- Favorite entries show a star icon in the list and a short on-panel toast notification when toggled
- Settings > Auto Play lets the current selection preview automatically while browsing
- Settings > Fast Mode lets you hold 8 or 2 to browse quickly

Notes:
- Gameplay input is blocked while the panel is open.
- Favorites and recents are saved automatically.
- Auto Play and Fast Mode settings are saved automatically.
- Faulted animations are logged to AnimPanel\logs\faultanims.txt.
- If F8 does nothing, your game is usually missing an ASI Loader.
