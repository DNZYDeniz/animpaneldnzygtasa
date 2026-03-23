# AnimPanel Native Source

Native-only source for the GTA San Andreas singleplayer animation browser.

Current architecture:
- `AnimPanel.asi` handles UI, input, camera, state, and animation playback.
- No CLEO bridge is used.
- Runtime assets are read from `AnimPanel\...` in the game root.
- Legacy fallback reads `modloader\AnimPanel\...` only for backward compatibility.

Runtime files required by the released mod:
- `AnimPanel.asi`
- `AnimPanel\data\anim-catalog.json`
- `AnimPanel\data\favorites.json`
- `AnimPanel\data\recents.json`
- `AnimPanel\fonts\Rajdhani-Bold.ttf`

Build prerequisites:
- Visual Studio 2022 Build Tools or Visual Studio 2022 with MSVC Win32 toolset
- Windows SDK with Direct3D 9 libraries
- Bundled `third_party\imgui`
- Bundled `third_party\plugin-sdk`

Build target:
- Open `AnimPanel.sln`
- Build `Release GTASA | Win32`
- Output: `native-src\anim-panel\output\AnimPanel.asi`

Notes:
- The project links with `/MT`, so the release build does not require a separate Visual C++ runtime installer.
- Target game is classic GTA San Andreas 1.0 US singleplayer.
