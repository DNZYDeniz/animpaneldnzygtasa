# Anim Panel

This folder documents the native animation panel implementation for classic GTA San Andreas singleplayer.

Current state:
- Native ASI source lives under `native-src/anim-panel/`.
- Runtime data and config live under `modloader/AnimPanel/`.
- The seed animation catalog is generated from `data/animgrp.dat` with `tools/generate-anim-panel-catalog.ps1`.

Important limits:
- The generated seed catalog is a bootstrap dataset from animation groups. It is not yet a full `ped.ifp` dump.
- The panel architecture is native-first because search, input capture, clipboard, scalable fonts, and virtualized lists are a poor fit for a large pure CLEO UI.
- Build tools are not bundled in this workspace, so the source tree is scaffolded here and must be compiled on a machine with Visual Studio Build Tools, plugin-sdk, and Dear ImGui installed.
