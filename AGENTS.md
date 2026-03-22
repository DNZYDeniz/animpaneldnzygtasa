# GTA SA CLEO Agent Rules

This workspace is a GTA San Andreas CLEO-first modding project.

Default target:
- Classic GTA San Andreas 1.0 US
- CLEO 5
- CLEO+
- Sanny Builder 4
- NewOpcodes only when explicitly needed

Animation panel exception:
- For the in-game animation browser/panel explicitly requested in this workspace, use a native `ASI + plugin-sdk + Dear ImGui` implementation as the default architecture.
- For this panel class, keep UI, input, camera, state, and animation playback in the same native runtime layer.
- Do not use a native UI + CLEO gameplay bridge for this panel class unless the user explicitly asks for a hybrid architecture after the tradeoffs are explained.
- Do not use real Chromium/CEF for classic GTA SA singleplayer unless the user explicitly asks for a heavy webview stack and accepts the extra runtime and compatibility cost.
- Treat SA:MP/open.mp CEF projects as comparison references only, not as the default architecture for singleplayer.
- Target this panel at GTA SA 1.0 US singleplayer with ASI Loader and SilentPatch. Use a light test profile first, then validate on the user's heavier mod stack.
- Keep the panel data-driven with runtime files under `modloader\\AnimPanel\\` and native source under `native-src\\anim-panel\\`.
- V1 scope for this panel is base-game animation catalog data only. Do not runtime-scan arbitrary modded IFP files in the first version.
- Panel visuals should be GTA SA-compatible modern overlay, not literal web CEF. Use rounded panels, high-contrast yellow/black theme, thick readable fonts, and restrained transitions.
- Panel input rules: `F8` toggles, `ESC` closes first, gameplay input is blocked while open, and focus loss must close the panel and release locks immediately.
- Performance rules for this panel: no disk reads every frame, no per-frame full-list normalization, only draw visible rows, and only write favorites/recents/config on change or close.

Hard rules:
- Produce only `.cs` CLEO scripts unless the user explicitly asks for another artifact.
- Exception: for native UI panels like `AnimPanel`, produce and maintain a single-layer native `.asi` solution by default rather than splitting UI and playback across native + CLEO.
- Treat Sanny Builder docs, Sanny Builder Library, and CLEO docs as the source of truth for syntax, directives, and opcode usage.
- Use MTA Wiki only for GTA SA data reference such as vehicle IDs, skin IDs, object IDs, model names, and list lookups.
- Never use MTA Lua/API functions as if they were CLEO opcodes.
- Do not recommend `plugin-sdk`, ASI, or C++ unless the user explicitly asks for that path.
- Target CLEO 5, but prefer syntax and opcode usage that does not break CLEO 4 compatibility unless a CLEO 5 feature is truly needed.
- Every infinite loop must include `wait 0` or an intentional nonzero wait.
- Use `terminate_this_custom_script` / `terminate_this_script` correctly for the script type being written.
- Output full, compilable, single-file script content. Do not return partial snippets unless the user explicitly asks for one.
- Prefer numeric model IDs in headerless or header-light cases where Sanny `0083` can occur.
- Require guards before using CLEO 5-only, CLEO+, or NewOpcodes-specific features.
- After writing or updating a source script, run the compile loop with the local Sanny CLI and inspect `compile.log` until clean.
- Do not parallelize Sanny compilations. Compile one script at a time, then inspect `compile.log`, because concurrent compiles can race and hide the real failure.
- If `tools/compile-cleo.ps1` is noisy, blocked, or ambiguous, fall back to direct `SannyBuilder\\sanny.exe` CLI usage and still require a clean `compile.log`.
- For runtime diagnosis of `.cs` scripts, prefer `cleo.log`, optional `cleo_script.log`, `debug_on`, `trace`, `log_to_file`, and short on-screen debug text. Do not rely on the Sanny native debugger for CLEO scripts.
- After any user-reported runtime bug or correction, read the relevant logs first and fix from evidence before asking the user to retest.
- When a custom script owns mission-like state, add lightweight file logging or stage logging so crashes can be mapped to the exact stage.
- If a user reports HUD spam or unexpected on-screen text, check `cleo.log` for unrelated loaded scripts and remove or disable the conflicting script before changing the target script.
- Do not promise broad version compatibility by default. Safe baseline is SA 1.0 US + CLEO 5. Claim wider support only when the specific mod is guarded and validated for it.
- Do not use speculative CLEO extension opcodes just because they exist in local opcode lists. Confirm the opcode is available under the current script extensions and compile target before using it.
- Prefer stable classic opcodes when they solve the problem. Only use CLEO+, NewOpcodes, or other extension-specific forms when there is a verified need.
- For special actors like Sweet, Ryder, Smoke, Tenpenny-style slots, use the classic SA form `023C: load_special_actor 'NAME' as <slot>` and unload with `0296: unload_special_actor <slot>`. Do not improvise alternate `load_special_character` forms unless they are verified for the current extension set.
- If using special actor slots `290-299`, remember they are only placeholder model IDs. The load opcode is what binds the real character to the slot; a bad load call can cause runtime pointer crashes.
- For CJ clothing changes with `087B`, use the documented SA syntax `087B: set_player $PLAYER_CHAR clothes_texture \"...\" model \"...\" body_part <id>` and rebuild immediately with `070D: rebuild_player $PLAYER_CHAR`.
- For scripted scene markers and cutscene-style triggers, require the player to be on foot before entering the scene logic. Do not trigger interior or cutscene transitions while the player is driving.
- On script init and save restore, reset volatile actor handles, markers, and cutscene lock flags so a crash or mid-scene save cannot leave the mission in a corrupt state.
- Avoid `if not` forms that the current Sanny target parses unreliably; prefer the negated opcode form such as `80DF:` or explicit nested checks when writing low-level CLEO syntax.
- If a native panel and a bridge or helper layer disagree on payload strings or playback commands, treat the bridge architecture as suspect immediately and collapse the feature into one runtime layer instead of debugging downstream symptoms first.

Default workflow:
1. Author human-readable source in `modding-src/<mod-name>/<mod-name>.txt`.
2. Compile with `tools/compile-cleo.ps1`.
3. Emit compiled `.cs` into `modloader/<ModName>/cleo/<ModName>.cs`.
4. Read `compile.log` on failure and fix until the helper exits cleanly.
5. If the user found a bug or requested a correction, update `lessons.md` in the same session before finishing.

Session discipline:
- At the start of every session in this project, read `AGENTS.md` and then review the relevant parts of `lessons.md` before editing or debugging.
- If `lessons.md` does not exist, create it.
- After any user correction, runtime failure, compile failure, or workflow mistake, update `lessons.md` with the exact cause, the fix, and the prevention rule.
- Keep `lessons.md` brutally practical. Add rules that directly reduce repeat mistakes, and refine them until the error rate drops.

Reference pack:
- [workflow.md](/D:/GTASAVIDEOCEKME/docs/gta-sa-modding/workflow.md)
- [compatibility.md](/D:/GTASAVIDEOCEKME/docs/gta-sa-modding/compatibility.md)
- [source-index.md](/D:/GTASAVIDEOCEKME/docs/gta-sa-modding/source-index.md)
- [lessons.md](/D:/GTASAVIDEOCEKME/lessons.md)
