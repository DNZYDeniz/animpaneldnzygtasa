# GTA SA Animation Panel Mod

Standalone export of the GTA San Andreas singleplayer animation panel mod from this workspace.

Contents:
- `AnimPanel.asi`: native panel binary
- `modloader/AnimPanel/`: runtime files used by the mod
- `modding-src/anim-panel/anim-panel.txt`: CLEO bridge source
- `native-src/anim-panel/`: native source and Visual Studio project
- `third_party/imgui/`: Dear ImGui files required to build the native plugin
- `tools/`: helper scripts used during development

Runtime layout:
- Put `AnimPanel.asi` in the GTA SA root.
- Put `modloader/AnimPanel/` into your `modloader/` folder.

Build notes:
- Open `native-src/anim-panel/AnimPanel.sln`
- Build `Release GTASA | Win32`
- Requires Visual Studio 2022 Build Tools with C++ and DirectX 9 SDK libraries from the Windows SDK

Licensing notes:
- `modloader/AnimPanel/fonts/Rajdhani-Bold.ttf` is included as the UI font.
- Dear ImGui keeps its own license in `third_party/imgui/LICENSE.txt`.

This export intentionally excludes workspace-wide logs, unrelated mods, and temporary build artifacts.
