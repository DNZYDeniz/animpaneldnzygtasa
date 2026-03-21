# AnimPanel Native Source

This module contains the native ASI source for the GTA SA animation browser panel.

Build prerequisites:
- Visual Studio Build Tools with MSVC Win32 toolset
- `PLUGIN_SDK_DIR` environment variable pointing to a plugin-sdk checkout
- `CLEO_SDK_SA_DIR` environment variable pointing to `D:\GTASAVIDEOCEKME\cleo_sdk`
- `IMGUI_DIR` environment variable pointing to a Dear ImGui checkout

Output target:
- `AnimPanel.asi` in the GTA SA root

Runtime content:
- `modloader/AnimPanel/config/anim-panel.ini`
- `modloader/AnimPanel/data/anim-catalog.json`
- `modloader/AnimPanel/data/favorites.json`
- `modloader/AnimPanel/data/recents.json`

Notes:
- The current workspace does not expose `cl.exe`, `msbuild`, or `cmake`, so the project files are prepared but not compiled here.
- The source is split into a small data core and an ImGui-facing panel layer to keep search/filter logic testable outside of render hooks.
