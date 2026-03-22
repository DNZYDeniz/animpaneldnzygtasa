# GTA SA CLEO Lessons

This file stores project-specific lessons that must be reviewed at the start of each session.

## Session Rules
- Read this file together with `AGENTS.md` before changing CLEO scripts in this workspace.
- After any user correction, bug report, compile failure, crash, or workflow mistake, update this file in the same session.
- Record the exact symptom, root cause, fix, and prevention rule.
- Prefer blunt, low-level rules over vague advice. The goal is to reduce repeat errors, not to summarize work.

## 2026-03-21 Ramazan Bayrami Lessons

### Symptom
- `Codex baseline loop alive` appeared on screen continuously.

### Root Cause
- An unrelated script in `modloader\\CodexBaselineLoop\\cleo\\CodexBaselineLoop.cs` was still being loaded by CLEO and spamming HUD text.

### Fix
- Remove or disable the conflicting script after confirming it in `cleo.log`.

### Prevention
- When a user reports unexpected HUD text or loop spam, inspect `cleo.log` for all loaded CLEO scripts before editing the target script.
- Do not assume the active bug belongs to the script currently being developed.

## 2026-03-21 Special Actor Crash

### Symptom
- Entering the Grove Street home marker caused a loading-screen-like flicker and then a crash to desktop.

### Root Cause
- The script used an invalid `load_special_character` form for Sweet, Ryder, and Smoke. `cleo.log` showed `Invalid pointer ... 023C: LOAD_SPECIAL_CHARACTER`, which means the special actor loader was called with the wrong argument layout.

### Fix
- Replace the bad calls with classic SA syntax:
- `023C: load_special_actor 'SWEET' as 1`
- `023C: load_special_actor 'RYDER' as 2`
- `023C: load_special_actor 'SMOKE' as 3`
- Unload them with `0296: unload_special_actor <slot>`.

### Prevention
- For named mission characters mapped onto special slots `290-299`, always use the documented `023C` / `0296` pair unless a different syntax has been compile-checked and runtime-verified for the exact extension set.
- If `cleo.log` reports pointer or string-argument errors around special actor loading, stop changing scene logic and fix the loader syntax first.
- Remember that model IDs `290-299` are only special slots, not proof that the actor was loaded correctly.

## 2026-03-21 CJ Clothing Lesson

### Symptom
- The script said CJ wore the bayram outfit, but the clothes did not change.

### Root Cause
- Clothing application used the wrong opcode wording and did not follow the documented SA `087B` syntax closely enough in the active script path.

### Fix
- Use:
- `087B: set_player $PLAYER_CHAR clothes_texture "tuxedo" model "suit1" body_part 0`
- `087B: set_player $PLAYER_CHAR clothes_texture "suit1trgrey" model "suit1tr" body_part 2`
- `087B: set_player $PLAYER_CHAR clothes_texture "shoedressblk" model "shoe" body_part 3`
- Then rebuild with `070D: rebuild_player $PLAYER_CHAR`.

### Prevention
- For CJ clothing, copy the official SA opcode shape exactly and rebuild the player model immediately after the last clothing write.
- Verify texture and model names against `data\\shopping.dat` when clothes do not appear.

## 2026-03-21 Compile Workflow Lesson

### Symptom
- Compile results became ambiguous while checking more than one script.

### Root Cause
- Multiple Sanny compile processes touched the same `compile.log`, which makes failures easy to miss or misattribute.

### Fix
- Compile one script at a time, clear `SannyBuilder\\compile.log` before each run, then inspect it immediately after that single compile.

### Prevention
- Never parallelize Sanny compiles in this project.
- If helper scripts and main scripts both change, compile and validate them sequentially.

## 2026-03-21 Condition Syntax Lesson

### Symptom
- Sanny produced parse errors around negative conditions during hot fixes.

### Root Cause
- The `if not` form was brittle in the current low-level CLEO source and caused parser issues.

### Fix
- Use negated opcodes such as `80DF:` or explicit nested logic instead of relying on `if not`.

### Prevention
- When writing low-level CLEO syntax, prefer explicit negative opcode forms over stylistic negation.

## 2026-03-21 Scene Transition Safety

### Symptom
- Scene transitions were vulnerable to broken state after crashes or interrupted loads.

### Root Cause
- Volatile actor handles and cutscene lock values can persist in saved custom-script variables if they are not reset on init.

### Fix
- Reset transient actor handles, markers, and cutscene lock flags during script initialization before rebuilding the current stage.

### Prevention
- Any CLEO script with staged mission flow must separate persistent progress state from volatile runtime handles.
- On init, always zero transient handles and rebuild presentation from stable stage variables.

## 2026-03-21 Native Panel Toolchain Lesson

### Symptom
- Native animation panel implementation could not be compiled inside the current workspace session.

### Root Cause
- The workspace has CLEO SDK files and Sanny Builder, but no active native build toolchain in PATH. `cl.exe`, `msbuild`, `cmake`, and `ninja` were unavailable.

### Fix
- Scaffold the native source tree, project files, runtime config, and generated catalog data in-repo, then compile later on a machine with Visual Studio Build Tools, plugin-sdk, Dear ImGui, and the expected environment variables.

### Prevention
- Before promising a built `.asi`, verify that the native compiler toolchain is present, not just the GTA/CLEO toolchain.
- For native GTA SA plugins in this workspace, separate "source tree implemented" from "binary built and tested" until `cl.exe` or equivalent is confirmed.

## 2026-03-21 Ready-To-Test Claim Lesson

### Symptom
- The user pressed `F8`, but no panel opened in-game.

### Root Cause
- The repository contained native panel source and data files, but no compiled `AnimPanel.asi` binary was produced or copied into the game root.

### Fix
- State explicitly when a feature is only scaffolded versus actually runtime-ready, and do not present native UI work as playable until the `.asi` exists and has been smoke-tested in-game.

### Prevention
- For GTA SA native plugins, confirm the final runtime binary path before telling the user to test input like `F8`.
- A source tree, project file, and JSON data are not a playable deliverable by themselves.

## 2026-03-21 SBL Draw Expression Lesson

### Symptom
- The new CLEO animation panel failed to compile with `Unknown directive row == animIndex`.

### Root Cause
- A text draw call used inline arithmetic in an argument position (`rowY - 7.0`), which confused the SBL parser and broke the following `if` block.

### Fix
- Precompute draw coordinates in a temporary float variable, then pass that variable into the draw opcode.

### Prevention
- In SBL source for GTA SA, avoid inline arithmetic inside opcode argument lists when a temporary variable makes parsing unambiguous.

## 2026-03-21 Anim Panel Runtime Crash Lesson

### Symptom
- Pressing `F8` to open the first CLEO anim panel caused the game to close without a helpful runtime opcode error in `cleo.log`.

### Root Cause
- The panel relied on a heavier custom per-frame draw path with many dynamic text operations and UI state changes, which was the highest-risk runtime path even though it compiled cleanly.

### Fix
- Replace the fragile custom draw panel with a simpler `print_formatted_now` overlay and add persistent debug stage writes to `modloader\\AnimPanel\\logs\\anim-panel-debug.ini`.

### Prevention
- If a new CLEO UI compiles but hard-crashes the game without a clear log trace, reduce the render path first and add stage logging before changing gameplay opcodes.
- Treat compile success as syntax validation only; runtime-safe UI paths in CLEO should start simple and be expanded gradually.

## 2026-03-21 Silent Sanny Compile Lesson

### Symptom
- The user pressed `F8`, `cleo.log` showed `modloader\\animpanel\\cleo\\animpanel.cs` loading, but the source-side fixes were not reflected in runtime and the game still behaved like the old broken build.

### Root Cause
- The primary `D:\\GTASAVIDEOCEKME\\SannyBuilder\\sanny.exe` copy exited with code `0` but silently failed to create or update output `.cs` files. This left a stale compiled script in play and made source edits look ineffective.

### Fix
- Compile with the working secondary copy at `D:\\GTASAVIDEOCEKME - Kopya\\SannyBuilder\\sanny.exe`.
- Replace the broken SBL-heavy anim panel source with a minimal classic CLEO fallback, then verify that `modloader\\AnimPanel\\cleo\\AnimPanel.cs` gets a fresh timestamp and contains the new script name.

### Prevention
- Do not trust `exit=0` from Sanny Builder alone in this workspace. Always confirm the output file timestamp changed and the new binary actually exists.
- If runtime behavior does not match the edited source, suspect a stale `.cs` before chasing unrelated mod conflicts.

## 2026-03-21 Anim Panel Conflict Isolation Lesson

### Symptom
- The user saw `Codex baseline loop alive.` spamming on screen while testing the anim panel, and the panel text kept getting displaced.
- After aggressive input like `Up` and `Enter`, the game could still crash without a useful opcode error in `cleo.log`.

### Root Cause
- `modloader\\CodexBaselineLoop\\cleo\\CodexBaselineLoop.cs` was an unrelated test script still loaded by CLEO and constantly writing low-priority HUD text.
- The anim panel still had a risky animation-execution path (`0812`) without enough runtime evidence to prove which exact animation/state combination was crashing.

### Fix
- Remove the unrelated baseline loop script completely from runtime.
- Temporarily replace the risky `Enter` playback path with a harmless status path in the safe build until the debug signal is clean again.

### Prevention
- When two scripts write HUD text at the same time, remove the unrelated one first before touching panel rendering.
- If an `Enter` action can still crash the game and logs are weak, neutralize the gameplay opcode path first, stabilize input/render, then reintroduce animation playback one opcode at a time.

## 2026-03-22 Anim Panel Catalog Source Lesson

### Symptom
- The animation panel taxonomy stayed narrow and stale even after the runtime moved to a native, data-driven panel.

### Root Cause
- The catalog was still seeded from legacy local sources (`animmm`, `animgrp.dat`) and the native UI hard-coded category names, so the data source and the menu model diverged from the intended official animation list.

### Fix
- Use the official open.mp animation dataset as the catalog source for large animation imports.
- Generate the catalog with scripted filtering and overrides.
- Make the native menu read categories from the loaded catalog instead of a hard-coded list.

### Prevention
- For large data-driven panels, do not hard-code UI category lists against a mutable catalog.
- When an official upstream dataset exists, generate from that source instead of growing a hand-curated local subset.
- Keep override files separate from the generated catalog so category/filter corrections do not require editing the output JSON by hand.

## 2026-03-22 Anim Panel Unsafe Vehicle Context Lesson

## 2026-03-22 Anim Panel Release Packaging Lesson

### Symptom
- The GitHub backup still looked like a hybrid `modloader + CLEO + native` project even though the working runtime had already moved to a native-only `.asi`.

### Root Cause
- Runtime packaging was not collapsed after the architecture change, so old `modloader\AnimPanel` and CLEO-era files were still being treated as if they belonged to the final user-facing release.

### Fix
- Ship the runtime as:
- `AnimPanel.asi`
- `AnimPanel\data\anim-catalog.json`
- `AnimPanel\data\favorites.json`
- `AnimPanel\data\recents.json`
- `AnimPanel\fonts\Rajdhani-Bold.ttf`
- Keep source under `native-src\anim-panel\`, `tools\`, `docs\`, and required `third_party\` only.

### Prevention
- When a mod is migrated to native-only runtime, update the release layout immediately instead of keeping legacy `modloader` or CLEO packaging around.
- For AnimPanel-class releases, the public install target is GTA SA root plus `AnimPanel\...`, not `modloader\AnimPanel\...`.
- Before pushing a release backup, inspect the repo tree for stale bridge-era files and remove them if they are not part of the live runtime.

## 2026-03-22 Public Release Readme Lesson

### Symptom
- A mod can be technically complete but still confusing for end users if install steps, controls, or dependencies are only described in source docs.

### Root Cause
- Developer-facing notes and player-facing release instructions were treated as optional instead of mandatory release files.

### Fix
- Ship both `README.md` and `README.txt` with every public mod release.
- Write them for the player, not for the developer.
- Update both files whenever runtime files, dependencies, controls, or install steps change.

### Prevention
- Every publicly shared mod in this workspace must include `README.md` and `README.txt`.
- Both readmes must describe the current install path, required external components, controls, and usage.
- If a change affects what the player installs or how the player uses the mod, updating both readmes is part of the implementation, not a separate optional task.

## 2026-03-22 Native Fault Animation Lesson

### Symptom
- Some specific GTA SA animations can crash the game even though most neighboring animations in the same category work fine.

### Root Cause
- Certain animations are engine-unsafe outside their intended state, pose chain, or scene context. In a native ASI, some failures happen immediately in the play call and some happen later inside engine task processing.

### Fix
- Add best-effort native exception guards around the immediate play call.
- Log faulted animations to a dedicated file.
- Skip faulted entries during fast preview workflows.
- Continue filtering known unsafe animation families out of the shipped catalog instead of assuming runtime guards can catch every crash.

### Prevention
- Do not promise that every engine-level animation crash can be intercepted in-process.
- For fast preview tools, combine three layers: catalog filtering, immediate-call exception guards, and fault logging/blacklisting.
- If a crash happens after the play call returns, treat it as a catalog safety problem first and filter that animation family out.

## 2026-03-23 Basketball Animation Family Lesson

### Symptom
- Previewing `BSKTBALL/BBALL_JUMP_CANCEL` crashed the game even though earlier basketball entries and many unrelated animations had worked in the same session.

### Root Cause
- The basketball library is prop- and minigame-state-dependent. The crash happened at the exact play call for `BBALL_JUMP_CANCEL`, which indicates the family is unsafe for generic on-foot CJ preview even if some entries appear to work.
- The generated catalog also still allowed raw `Missing animation` entries because the filter only rejected `[missing animation]` with brackets, not the plain phrase used in the generated notes.

### Fix
- Remove the entire `BSKTBALL` library from the shipped preview catalog.
- Treat `missing animation` without brackets as a blocked condition too.

### Prevention
- If one member of a prop/minigame library crashes, remove the whole library instead of waiting for more single-ID crashes.
- Filter both bracketed and plain-text missing-animation markers when generating the catalog from upstream descriptions.

## 2026-03-23 Vehicle Door And Object Scene Lesson

### Symptom
- `PED/CAR_CLOSEDOOR_RHS` crashed the game, and nearby entries were clearly tied to doors, safes, crates, or car interaction scenes.

### Root Cause
- These clips are not free-standing on-foot previews. They depend on vehicle doors, safes, crates, or scene objects and can crash or behave unpredictably when bound directly onto CJ without the required world state.

### Fix
- Remove object-bound families at generator level:
- `PED:CAR_*`
- `PED:DOOR_*`
- `RYDER:VAN_*`
- `ROB_BANK:CAT_SAFE*`
- `GHETTO_DB:GDB_CAR_*`
- `POLICE:*GETOUTCAR*`

### Prevention
- If the animation name itself encodes an external object or scene dependency, do not wait for runtime crash proof from every sibling entry. Remove the whole family from the preview catalog.
- For this panel, vehicle-door, safe, crate, and in-car scene animations are not preview-safe and must stay out of the shipped list.

### Symptom
- Some animations previewed correctly, but specific entries such as `PED/DRIVE_R_WEAK_SLOW` crashed the game immediately on play.

### Root Cause
- The generated catalog still included vehicle-seat and driving-context animations that are valid in the upstream dataset but unsafe or meaningless for on-foot CJ preview. The crash family was identifiable from the official description text: steering, driver/passenger, riding, lowrider-seat, truck/boat driving context.

### Fix
- Filter these families at catalog generation time using description/name heuristics instead of waiting for runtime failures.
- Remove `PED:DRIVE_*` and broader vehicle-context descriptions (`driver`, `passenger`, `riding`, `lowrider`, `windshield`, `truck`, `boat`, `kart`) from the generated preview catalog.

### Prevention
- For animation preview catalogs, treat official description text as the primary safety signal.
- If one member of an animation family crashes and the descriptions show the same vehicle-seat context, remove the whole family from the preview catalog instead of patching single IDs one by one.

## 2026-03-21 Negative File Handle Lesson

### Symptom
- Pressing `Up` inside the anim panel raised `0AD9: WRITE_FORMATTED_STRING_TO_FILE` with `Invalid or already closed ... file handle` and suspended the script.

### Root Cause
- `open_file` returned a negative invalid handle, but the script only checked `<> 0`, so it treated the handle as valid and attempted to write to it.

### Fix
- Remove all optional file logging and cache-writing from the safe anim panel build.

### Prevention
- In this workspace, do not treat `open_file <> 0` as a safe validity check.
- For crash isolation builds, avoid all file I/O unless it is absolutely required and verified on the active game setup.

## 2026-03-21 Safe Build Communication Lesson

### Symptom
- The panel became stable, but `Enter` did not animate CJ because playback had been intentionally disabled during crash isolation.

### Root Cause
- The temporary safe build removed real animation playback, but that limitation was no longer acceptable once the panel itself stopped crashing.

### Fix
- Re-enable only the verified `PED` playback path with `04ED`, `84EE`, `0687`, and `0812`, while keeping file I/O disabled.

### Prevention
- Once a safety-only downgrade is in place, restore the core user-facing feature as soon as the crash source is isolated.
- A stable panel that cannot actually preview animations is only an intermediate diagnostic build, not a usable deliverable.

## 2026-03-21 Player Preview Playback Lesson

### Symptom
- The panel stayed stable, but pressing `Enter` to preview an animation on CJ could still hard-crash the game without a useful CLEO opcode error.

### Root Cause
- The first restored playback path used `0812` with a more mission-style actor animation flow. That was too risky for a lightweight live preview path on the player during diagnostic builds.

### Fix
- Switch the preview path to `0605: actor ... perform_animation ...`.
- Keep `04ED`/`04EE` loading logic and add `Enter` key release waiting to prevent rapid retrigger spam.

### Prevention
- For CJ preview panels, start with the simpler direct actor animation opcode before moving to AS-style animation opcodes.
- Debounce `Enter` and similar activation keys so a held key does not retrigger animation playback multiple times in the same interaction.

## 2026-03-21 Extension Opcode Availability Lesson

### Symptom
- Attempting to harden playback with `0E96` and `0E97` failed at compile time.

### Root Cause
- Those task-clearing opcodes are not available in the current guaranteed baseline extension set for this script target.

### Fix
- Revert to classic `0687: clear_actor ... task`.

### Prevention
- In this workspace, do not switch to CLEO+/NewOpcodes task helpers unless the script explicitly opts into the required extension and the compile target is validated for it.

## 2026-03-22 AnimPanel Block Casing Fallback Lesson

### Symptom
- `bridge-debug.ini` showed `play_stage=133` and `play_result=941`: command and non-PED load succeeded, but animation still did not apply.

### Root Cause
- The panel now correctly used `selection.block` for playback, but catalog values are lowercase (`crib`, `gangs`, `ped`) while many successful runtime calls expect uppercase library/block names.
- Result: playback with `block` could fail even when `ifp_file` had already loaded successfully.

### Fix
- Keep primary playback path on `block` (as designed).
- Add a minimal fallback: if `TASK_PLAY_ANIM_NON_INTERRUPTABLE` + `0605` with `block` both fail, retry once with `ifp_file`.
- Preserve all existing `bridge.ini` keys and stage logging; add explicit fallback stages `134` and `135`.

### Prevention
- When switching playback to `block`, verify runtime casing compatibility with real catalog data.
- If load works but play fails (`941`), add staged fallback or normalize block casing instead of assuming load logic is broken.

## 2026-03-22 AnimPanel Stage126 Crash Isolation

### Symptom
- Selecting `on_lookers:wave_loop` and pressing play caused immediate game exit.
- `bridge.ini` and `bridge-debug.ini` stopped at `stage=126`, `play_stage=126`, which means crash happened right at non-PED load entry.

### Root Cause
- The non-PED load path was using raw low-level `04ED/04EE/04EF` calls with runtime string variables.
- In this build path, that form was unstable for live selection-driven strings and could hard-crash before stage advanced.

### Fix
- Replace low-level forms with the same command family used by the known working `animmm` script:
- `REQUEST_ANIMATION <ifp>`
- `HAS_ANIMATION_LOADED <ifp>`
- `REMOVE_ANIMATION <ifp>`
- Keep existing stage/debug writes and the rest of the bridge flow unchanged.

### Prevention
- For dynamic animation names from INI/runtime selection, prefer stable keyword forms already proven in a working reference script.
- If logs freeze at a pre-load stage (`126`) with no opcode error, suspect the load opcode form first.

## 2026-03-22 Animation String Type Compatibility

### Symptom
- Non-PED animation load path could hard-crash near load stage during live INI-driven playback.

### Root Cause
- Runtime animation arguments were passed through generic long string vars (`@v`) while working reference scripts use text-label style vars (`TEXT_LABEL16` / `@s`) for animation dictionary/name fields.

### Fix
- Use `@s` string vars for runtime animation fields read from bridge selection:
- `anim_name`, `ifp_file`, `block`, and cached loaded IFP name.
- Keep gameplay/state logic unchanged; only normalize argument string type at opcode boundaries.

### Prevention
- For `REQUEST_ANIMATION`, `HAS_ANIMATION_LOADED`, `REMOVE_ANIMATION`, `TASK_PLAY_ANIM_NON_INTERRUPTABLE`, `0605`, and `0611`, prefer `@s` text-label variables for animation names/groups in this workspace.

## 2026-03-22 Catalog Coverage And Format Reference

### Observation
- Current panel catalog has `TOTAL=146`, `PED=13`.
- Grinch Trainer SA animation dataset contains `PED=294` entries (matches the requested `995..1288` PED range size).

### Practical Rule
- Treat animation catalog completeness as a first-class runtime dependency, not only a UI list issue.
- Use a proven source format where animation name maps to an uppercase dictionary/IFP key, then derive panel records from it.
- Keep play dictionary casing explicit; avoid mixed lowercase/uppercase ambiguity between `block` and `ifp_file`.

## 2026-03-22 Grinch Trainer Reference Boundary

### Observation
- `D:\GTASAVIDEOCEKME\GrinchTrainer-III-VC-SA-1.0` contains resources, API headers, import libs, and data files.
- It does not contain the actual SA trainer gameplay source that performs animation preview logic.

### Practical Rule
- Treat Grinch as a runtime/data/reference package, not as full source code for direct code-copying.
- Use its `resource\GrinchTrainerSA\data\animations.toml` as a verified animation dataset and UI/reference behavior guide.
- Do not assume a folder with `.asi`, `.lib`, `.exp`, and resource files includes the real implementation source.

## 2026-03-22 Native AnimPanel Build Path Lesson

### Symptom
- Native `AnimPanel.asi` source changes compiled nowhere until the build was attempted directly.

### Root Cause
- `AnimPanel.vcxproj` used `$(SolutionDir)..\..\..\third_party\imgui\...` for include paths.
- In this solution layout, that resolves too far upward and breaks `imgui.h` includes.

### Fix
- Change native include paths to use `$(ProjectDir)..\..\..\third_party\imgui\...`.
- Build with:
- `vcvars32.bat`
- `MSBuild.exe D:\GTASAVIDEOCEKME\native-src\anim-panel\AnimPanel.sln /t:AnimPanel /p:Configuration="Release GTASA" /p:Platform=Win32`

### Prevention
- For this native panel project, trust the project directory as the stable anchor, not `$(SolutionDir)`, when resolving `third_party` dependencies.
- After native edits, rebuild the `.asi` and replace the root `D:\GTASAVIDEOCEKME\AnimPanel.asi` before calling the result tested.

## 2026-03-22 Duplicate AnimPanel Script Conflict

### Symptom
- Preview looked broken and `cleo.log` showed `Opcode [247B] not found! in script 'animpanel.cs'`.

### Root Cause
- A stale second script at `D:\GTASAVIDEOCEKME\cleo\animpanel.cs` was loading alongside `modloader\AnimPanel\cleo\AnimPanel.cs`.
- The old root-level script used unsupported opcodes and also reused the same `animpan` script name, which polluted diagnosis.

### Fix
- Disable the stale root script by renaming it to `animpanel.cs.disabled`.

### Prevention
- When runtime behavior makes no sense, check `cleo.log` for duplicate script loads by filename and script name before changing the target bridge logic.
- Do not keep old copies of the same feature script both in `cleo\` and `modloader\...\cleo\`.

## 2026-03-22 Panel Input Lock Regression

### Symptom
- While the panel was open, left mouse click could still punch and gameplay input could override preview attempts.

### Root Cause
- The bridge set `previewActive` and then called `ReleasePanelLock`, re-enabling player control before applying the animation.
- The main loop also skipped `ApplyPanelLock` while preview was active.

### Fix
- Keep `ApplyPanelLock` active for the entire time the panel is visible.
- On play, reinforce the lock instead of releasing it.

### Prevention
- If the panel is open, gameplay input must stay blocked with no preview exception.
- Do not re-enable control before testing whether the animation has actually taken over the actor task stack.

## 2026-03-21 Anim Panel Visual Expectation Lesson

### Symptom
- The user rejected the first "list panel" iteration even though it was functional, because it still looked like centered debug text instead of a trainer-style boxed HUD panel.

### Root Cause
- The implementation optimized for runtime safety first and kept the UI in low-priority text form too long.
- For this feature, "panel" means a visually framed left-side trainer menu with header/body/footer blocks, not just a stacked text list.

### Fix
- Rebuild the anim browser with explicit HUD boxes and positioned text so it reads like a real in-game trainer panel.
- Accept `NewOpcodes` as a justified dependency when richer positioned text styling is required and compile it explicitly with `{$USE NewOpcodes}`.

### Prevention
- For UI-heavy CLEO features in this workspace, match the requested visual archetype early. If the user references a trainer screenshot, implement the boxed layout first instead of shipping a text-only placeholder as if it were the intended design.
- When `NewOpcodes` is required for the requested visual quality, declare that dependency explicitly at the top of the script and keep the rest of the opcode set conservative.

## 2026-03-21 Runtime Extension Availability Lesson

### Symptom
- The boxed anim panel compiled cleanly, but pressing `F8` suspended the script at `0D66` with `CLEO extension plugin "NewOpcodes" is missing`.

### Root Cause
- Compile-time acceptance of an extension opcode did not mean the user actually had the matching runtime plugin installed in the game folder.

### Fix
- Remove the `NewOpcodes`-only text drawing path and rebuild the panel on standard SA text-draw opcodes plus CLEO FXT text entries.

### Prevention
- In this workspace, do not ship a UI path that depends on `NewOpcodes` unless `cleo.log` or the installed files prove that the runtime plugin is present on the target setup.
- A clean compile with `{$USE ...}` is not enough; runtime extension availability must be assumed absent until confirmed.

## 2026-03-21 Panel Flicker Lesson

### Symptom
- The anim panel visibly disappeared and reappeared every time the user pressed `Up` or `Down`.

### Root Cause
- Input debounce used blocking `wait 140` style pauses immediately after changing selection, which stopped the panel draw routine for that duration.

## 2026-03-21 Native Panel Bridge Command Lesson

### Symptom
- The native premium panel rendered and logged `STATUS: Previewing ...`, but pressing `NumPad 5` still did not play any animation on CJ.
- `cleo.log` showed no crash or suspension, so the failure stayed silent.

### Root Cause
- The native UI was writing preview commands correctly, but the CLEO bridge depended on ambiguous command-dedup compare forms during sequence checking.
- In this workspace/toolchain, raw compare expressions like `3@ == 1@` and `3@ <> 1@` are parser-fragile, and the negated-opcode form makes the intent too easy to get wrong in bridge logic.
- Because the bridge had no explicit command acknowledgment or stage logging, a blocked command path could fail with no visible proof.

### Fix
- Rebuild the bridge command gate as explicit control flow:
- `if 003B: 3@ == 1@ then jump @MAIN_AFTER_COMMAND`
- Only process play/stop when the incoming sequence is different from the last acknowledged sequence.
- After handling a command, write `ack_sequence` and reset `command.action` to `0`.
- Add lightweight runtime stage logging into `modloader\\AnimPanel\\cache\\bridge.ini` under `[debug]` so play failures can be mapped to the exact bridge stage.

### Prevention
- For command bridges in this workspace, do not use raw `==` / `<>` expression lines in low-level CLEO source.
- Do not rely on `803B`-style negated compare opcodes for critical bridge gating when an explicit equality-plus-skip label is clearer.
- Any native-to-CLEO command bridge must write an acknowledgment value and a stage code; do not leave command execution silent.

## 2026-03-21 PED Preview Load Chain Lesson

### Symptom
- Pressing `NumPad 5` in the premium native panel could freeze or crash the game right as a preview command was triggered.
- `bridge.ini` showed `stage=150`, which proved the bridge reached the final playback line before the crash.

### Root Cause
- The premium bridge had reintroduced `04ED` / `84EE` animation loading for built-in `PED` previews on CJ.
- Earlier stable testing had already shown that the live CJ preview path was safest when it called `0605` directly without a pre-load/wait chain for `PED`.
- Re-adding the load chain recreated the instability even though the command bridge itself was working.

### Fix
- Remove the `04ED` / `84EE` load-and-wait chain from the CJ preview path.
- Keep the bridge on the simpler proven flow:
- `0687: clear_actor $PLAYER_ACTOR task`
- `0605: actor $PLAYER_ACTOR perform_animation ... IFP "PED" ...`

### Prevention
- For classic GTA SA singleplayer live-preview on CJ, do not preload the stock `PED` IFP in the bridge path unless there is a newly verified reason.
- If an older stable build already proved a simpler playback path, do not reintroduce heavier loading logic during UI/native migrations without fresh runtime validation.

## 2026-03-21 Native Numpad Key Mapping Lesson

### Symptom
- The premium panel rendered and navigation could appear to work, but pressing numpad `5` often did nothing and `native-panel.log` showed no new `Previewing ...` entry at all.

### Root Cause
- The native UI listened only for `VK_NUMPAD5`.
- On some GTA SA / keyboard states, the numpad center key is surfaced as `VK_CLEAR` instead of `VK_NUMPAD5`. The same class of issue affects numpad arrows versus regular arrow virtual keys.
- That means the play/open hotkey could fail before the command ever reached the CLEO bridge.

### Fix
- Accept both numpad and translated virtual keys in the native panel:
- `8`: `VK_NUMPAD8` and `VK_UP`
- `2`: `VK_NUMPAD2` and `VK_DOWN`
- `4/6`: `VK_NUMPAD4`/`VK_LEFT`, `VK_NUMPAD6`/`VK_RIGHT`
- `5`: `VK_NUMPAD5`, `VK_CLEAR`, and `VK_RETURN`

### Prevention
- For GTA SA native menu controls, do not bind only to raw numpad virtual keys.
- When a key appears dead and the native log does not record the intended action, check the virtual-key mapping first before changing bridge or gameplay logic.

### Fix
- Replace blocking debounce waits with key-release loops that continue calling the panel draw and preview camera routines while the key is held.

### Prevention
- For any per-frame CLEO UI in this workspace, do not use blind post-input waits for debounce if the UI must stay visible.
- Input release handling must preserve redraw when a panel is open.

## 2026-03-21 Panel Input Lock Lesson

### Symptom
- On the first panel open, pressing `Up` or `Down` could still move CJ even though the panel was visible.

### Root Cause
- `set_player can_move 0` alone was not enough to guarantee a hard movement lock under the active mod/input stack.

### Fix
- When opening the panel, also lock the actor with `04D7` and disable mutual activities with `09F5`.
- Restore both flags when closing the panel.

### Prevention
- For in-game preview panels in this workspace, do not rely on `01B4` alone for input lock if the player must remain perfectly still.
- Combine player movement disable with actor lock for panel-open states.

## 2026-03-21 Native Panel Deployment Lesson

### Symptom
- The user still saw the old CLEO boxed panel even after native premium panel work had been implemented in source.

### Root Cause
- The native `AnimPanel.asi` binary had not yet been copied into the game root, so the game could only load the existing `AnimPanel.cs`.
- The CLEO script was still acting as the visible UI instead of being reduced to a bridge/runtime helper.

### Fix
- Build `native-src\\anim-panel\\output\\AnimPanel.asi` and deploy it to the game root as `AnimPanel.asi`.
- Replace the old CLEO fallback UI with a bridge-only script that reads native panel commands from `modloader\\AnimPanel\\cache\\bridge.ini`.

### Prevention
- For hybrid native+CLEO features in this workspace, do not call the native panel “active” until the built `.asi` is physically present in the game root.
- When migrating from a CLEO fallback UI to a native panel, remove or neutralize the old CLEO drawing path in the same pass so the user cannot keep seeing the obsolete interface.

## 2026-03-21 INI Extension Compile Lesson

### Symptom
- The new bridge-only CLEO script failed with `Opcode 0AF0 is not found in the standard opcodes and current script extensions.`

### Root Cause
- The script used `0AF0/0AF1/0AF4/0AF5` ini opcodes without declaring the `ini` extension explicitly.

### Fix
- Add `{$USE ini}` at the top of the script before using the ini opcode family.

### Prevention
- In this workspace, treat ini access as an extension feature, not as baseline CLEO syntax.
- If a bridge/helper script reads or writes `.ini` runtime files, declare `{$USE ini}` immediately instead of waiting for compile failure.

## 2026-03-21 Variable Assignment Parser Lesson

### Symptom
- The bridge script produced parse errors like `Unknown directive 1@ = 3@` and `Unknown directive 0@ = 2@`.

### Root Cause
- In the active low-level Sanny target, variable-to-variable assignment shorthand was parsed unreliably in this script shape even though literal assignment forms were accepted.

### Fix
- Replace variable copy shorthand with classic assignment opcode form, for example `0085: 1@ = 3@`.

### Prevention
- In this workspace, prefer classic assignment opcodes for variable-to-variable copies inside low-level CLEO scripts.
- If Sanny reports `Unknown directive` on a plain assignment line, convert it to the documented opcode form instead of assuming the surrounding `if` block is broken.

## 2026-03-21 Bridge Sequence Logic Lesson

### Symptom
- The native panel showed `Previewing ...` in the native log, but pressing `5` still did not make CJ play the selected animation.

### Root Cause
- The CLEO bridge compared command sequence IDs with the wrong polarity. It processed commands only when the incoming sequence matched the already handled one, instead of when it changed.

### Fix
- Use the negated integer comparison form for the bridge trigger:
- `if 803B: 3@ == 1@`
- Then copy the new sequence and process the action.

### Prevention
- For native-to-CLEO bridge command queues in this workspace, handle commands only on sequence change.
- When a UI log proves the command was emitted but gameplay does not react, inspect bridge de-duplication logic before changing the animation opcode path.

## 2026-03-21 SBL String Init Lesson

### Symptom
- The new coordinate HUD source failed to compile with `Unknown directive 4@= "N"`.

### Root Cause
- In the active `sa_sbl` compile target, inline declaration-time initialization for `shortString` (`shortString directionText = "N"`) was lowered into a form that this toolchain did not parse correctly.

### Fix
- Declare the string first, then initialize it on the next line with `string_format directionText = "N"`.

### Prevention
- In this workspace, avoid inline initializer syntax for `shortString` declarations in small CLEO utility scripts.
- If Sanny reports an `Unknown directive` on a string declaration, split declaration and assignment before changing unrelated logic.

## 2026-03-21 Clipboard Extension Lesson

### Symptom
- The coordinate HUD used `0B21`, but `compile.log` reported `Opcode 0B21 is not found in the standard opcodes and current script extensions.`

### Root Cause
- The clipboard opcode family is not baseline in this source mode; the script used it without explicitly declaring the `clipboard` extension.

### Fix
- Add `{$USE clipboard}` at the top of the source before calling `0B20/0B21`.

### Prevention
- In this workspace, treat clipboard access as an explicit extension feature, just like `ini`.
- If a utility mod writes to the Windows clipboard, declare `{$USE clipboard}` immediately instead of waiting for compile failure.

## 2026-03-21 Compile Helper False Positive Lesson

### Symptom
- `tools\\compile-cleo.ps1` printed `Compiled OK`, but the actual `compile.log` still contained a compile error for the same source.

### Root Cause
- The helper did not reliably surface the touched `compile.log` state in this run, so the console success message alone was misleading.

### Fix
- Re-read `SannyBuilder\\compile.log` manually after the helper run, catch the hidden error, then fix the source and rebuild until the log is empty.

### Prevention
- In this workspace, do not trust `Compiled OK` by itself after an extension-related source change.
- Always inspect the active `compile.log` directly before declaring the build clean.

## 2026-03-21 Clipboard Text Versus Raw Clipboard Lesson

### Symptom
- Pressing `F3` in the coordinate HUD did not behave like "copy this coordinate line as text"; the first implementation used the wrong clipboard path for the actual user goal.

### Root Cause
- The script used a raw clipboard-data opcode path and plugin-style guard logic, while the real requirement was plain text copy to the Windows clipboard.
- `cleo.log` also showed no loaded `ClipboardCommands.cleo`, so the first approach was not grounded in the active runtime.

### Fix
- Remove the raw clipboard/plugin path and switch the HUD to direct text clipboard writing with `Sf.ClipboardWriteText(copyText)`.

### Prevention
- When the user says "copy this text", prefer a text-specific clipboard opcode over raw byte clipboard commands.
- Before depending on a named CLEO plugin, confirm it is actually present in `cleo.log`.

## 2026-03-21 SBL Sf Clipboard Runtime Lesson

### Symptom
- The game raised `Opcode [0C8D] not found! CLEO extension plugin "SAMPFUNCS" is missing!` when `F3` was pressed in `coordhud.cs`.

### Root Cause
- In the active `sa_sbl` toolchain, `Sf.ClipboardWriteText` compiled cleanly but mapped to a SAMPFUNCS runtime opcode family that is not installed in the user's game.

### Fix
- Remove the `Sf.ClipboardWriteText` path entirely.
- Rebuild clipboard copy on baseline CLEO WinAPI calls via `kernel32.dll` and `user32.dll` (`GlobalAlloc`, `GlobalLock`, `OpenClipboard`, `SetClipboardData`, `CloseClipboard`).

### Prevention
- In this workspace, do not trust `sa_sbl` class names with `Sf.` for singleplayer runtime unless `cleo.log` proves SAMPFUNCS is actually loaded.
- A clean compile is not runtime proof for `Sf.*` commands.

## 2026-03-21 Dynamic Library Load Syntax Lesson

### Symptom
- Pressing `F3` raised `Invalid input argument #1 "fileName"` at `0AA2: LOAD_DYNAMIC_LIBRARY` in `coordhud.cs`.

### Root Cause
- In this `sa_sbl` script shape, the low-level `0AA2:` / `0AA4:` form with inline DLL/procedure strings compiled, but runtime treated the input string like a null pointer.

### Fix
- Replace the low-level form with the SBL function form used by the bundled example:
- `kernelLib = load_dynamic_library "kernel32.dll"`
- `procOpenClipboard = get_dynamic_library_procedure "OpenClipboard" {library} userLib`

### Prevention
- When using `load_dynamic_library` and `get_dynamic_library_procedure` in `sa_sbl` sources here, prefer the SBL assignment form over manual `0AA2:` / `0AA4:` opcode lines.
- If `cleo.log` reports an invalid string pointer on DLL loading, fix the string-calling form before changing the WinAPI logic itself.

## 2026-03-21 Clipboard Abandonment Lesson

### Symptom
- Repeated attempts to copy HUD text directly to the Windows clipboard caused runtime failures in the coordinate HUD:
- missing SAMPFUNCS for `0C8D`
- invalid DLL string input on `0AA2`
- wrong calling convention / `pop` mismatch on `0AA7`

### Root Cause
- Clipboard access in this singleplayer CLEO setup was not a stable baseline path. Different seemingly valid compile-time routes depended on missing plugins or fragile WinAPI calling details.

### Fix
- Remove clipboard and WinAPI copy logic entirely.
- On `F3`, write the current coordinate line into `modloader\\CoordHUD\\last-coordinate.txt` with plain CLEO file operations.

### Prevention
- For simple singleplayer data capture in this workspace, prefer writing a text file over trying to push text into the Windows clipboard from CLEO.
- Treat clipboard integration as optional/high-risk unless the exact runtime plugin stack has already been proven on the target machine.

## 2026-03-21 Coordinate File Truncation Lesson

### Symptom
- `modloader\\CoordHUD\\last-coordinate.txt` contained only a partial line like `X: 2495.73  Y: -`.

### Root Cause
- The HUD first built the full coordinate line into an intermediate string variable and then wrote it to file with `%s`. In this script shape that string path was not reliable for the whole formatted payload.

### Fix
- Remove the intermediate `%s` file write.
- Write the coordinate line directly to file with the final format string and the raw float/string arguments.

### Prevention
- For file output in this workspace, do not round-trip long formatted HUD text through an intermediate string unless there is a verified need.
- If a saved text line is mysteriously truncated, write the final values directly with `write_formatted_string_to_file`.

## 2026-03-21 String Concatenation Syntax Lesson

### Symptom
- The named coordinate HUD source hit `Unknown directive concatenate_strings inputName charText` during compile.

### Root Cause
- In this source mode, the plain-text SBL form of string concatenation was not accepted even though the underlying opcode exists.

### Fix
- Use the documented low-level opcode form:
- `0D4A: concatenate_strings inputName charText`

### Prevention
- For older string helper opcodes in this workspace, prefer the explicit opcode form if the more natural SBL phrase is rejected.
- When a compile failure says `Unknown directive` on a known string helper, first try the numeric opcode spelling before redesigning the feature.

## 2026-03-21 Named Input Crash Isolation Lesson

### Symptom
- Pressing `F3` to open the first named-coordinate input box threw the game out immediately, while `cleo.log` showed no fresh opcode suspension for `coordhud` after startup.

### Root Cause
- The first input-box build mixed a freshly opened modal state with an extra nested `repeat/wait 0` draw loop inside the same `F3` key path. That made the highest-risk part the modal transition/render pattern itself, not a single logged opcode.

### Fix
- Simplify the input flow:
- keep `F3` as a one-shot state toggle only
- move textbox rendering and keyboard handling back to the normal main loop
- reduce the text buffer from `longString` to `shortString`

### Prevention
- When a new CLEO UI crashes without a new `cleo.log` opcode error, remove nested modal loops first and let the main loop own rendering/input.
- Prefer the smallest viable string type in HUD/input tools unless a larger buffer is proven necessary.

## 2026-03-21 Coordinate HUD Live Text Input Lesson

### Symptom
- Pressing `F3` in the coordinate HUD still caused a hard game exit even after the nested loop had been removed, and `cleo.log` showed no fresh `coordhud` suspension line for the crash.

### Root Cause
- The remaining high-risk path was the live custom text-entry stack itself: per-frame string editing, backspace cutting, concatenation, and drawing the typed `%s` buffer inside the HUD.
- In this singleplayer CLEO setup, that custom textbox-style flow was not a stable baseline even though it compiled cleanly.

### Fix
- Remove live typed-name input entirely from the coordinate HUD.
- Rebuild `F3` as a direct save action that writes stable auto-tags like `ped001`, `ped002`, ... into `saved-coordinates.txt` and mirrors the last line into `last-coordinate.txt`.

### Prevention
- For small GTA SA CLEO utility mods in this workspace, do not build custom live text-entry UIs unless a proven native/game-supported input path already exists for the exact runtime.
- If `cleo.log` stays silent during an `F3`/HUD-triggered CTD, cut the dynamic string-editing path first and fall back to deterministic file output or auto-generated labels.

## 2026-03-21 Local Variable Name Collision Lesson

### Symptom
- `compile.log` reported `Duplicate constant name fileHandle` in the coordinate HUD even though the duplicated names were declared in different subroutine labels.

### Root Cause
- In this `sa_sbl` target, local declarations inside separate labels still share the same script-level naming space for generated symbols.

### Fix
- Rename per-subroutine locals to unique identifiers such as `namesFileHandle` and `lastHandle`.

### Prevention
- Do not reuse local variable names across different labels in the same CLEO source file here.
- Treat subroutine-local declarations as globally unique for compile safety in this workspace.

## 2026-03-21 HUD Hotkey Conflict Lesson

### Symptom
- Coordinate HUD name-selection hotkeys such as `F2`, `F4`, and `F5` conflicted with other in-game menus on the user's setup.

### Root Cause
- The first selectable-name build reused common function keys without checking the user's active mod stack and menu bindings.

### Fix
- Move non-critical HUD selection actions to `NumPad4` and `NumPad6`, while keeping `F3` only for save.

### Prevention
- In this workspace, avoid assigning helper HUD navigation to common menu keys like `F2/F4/F5` unless the user explicitly asks for them.
- Prefer numpad keys for lightweight in-game utility navigation when the feature must coexist with trainer/menu-heavy setups.

## 2026-03-21 HUD Selection Feedback Lesson

### Symptom
- Name switching on the coordinate HUD felt delayed, and the overlay showed a broken temporary text like `Secili i` instead of immediately showing the next selected name.

### Root Cause
- Selection changes were writing a temporary status message that overrode the main selected-name line.
- That temporary message lived in a `shortString`, so longer text like `Secili isim: ...` was visibly truncated.

### Fix
- Remove the temporary selection-status message entirely.
- Always draw the selected name on the main HUD line.
- Move save/error feedback to a separate lower line.
- Promote `statusText`, `selectedName`, and `lineText` to `longString`.

### Prevention
- Do not use `shortString` for user-visible labels that can exceed a few characters in this workspace.
- For HUD selectors, do not hide the selected value behind transient status messages; the current selection must update on the main line immediately.

## 2026-03-21 Name List Line Ending Lesson

### Symptom
- The coordinate HUD read only the first name from `name-list.txt`, and later selections fell back to `ped001`, `ped002`, ... even though the file contained multiple names.

### Root Cause
- The generated `name-list.txt` used Unix `LF` line endings.
- In this CLEO file-read path, `read_string_from_file` did not behave reliably for that list format and effectively treated the file as one broken chunk.

### Fix
- Rewrite `modloader\\CoordHUD\\name-list.txt` with Windows `CRLF` line endings.

### Prevention
- Runtime text lists that are consumed with `read_string_from_file` in this workspace should be saved with Windows line endings.
- When a text list reads only the first entry, inspect the file bytes before changing the selection logic.

## 2026-03-21 Name Selection Repeat Lesson

### Symptom
- Pressing or slightly holding `NumPad4` or `NumPad6` in the coordinate HUD could skip over 2-3 names instead of advancing exactly one slot.

### Root Cause
- The selector used `is_key_pressed` plus a small cooldown, so a held key could still retrigger after the cooldown expired.

### Fix
- Change the selector and save input to `is_key_just_pressed`.
- Keep only a tiny safety cooldown after the edge-triggered key event.

### Prevention
- For one-step menu or HUD selection in this workspace, prefer `is_key_just_pressed` over `is_key_pressed` whenever the action must happen exactly once per tap.
- Use cooldown only as secondary protection, not as the primary anti-repeat mechanism.

## 2026-03-21 Non-PED Anim Preview Load Lesson

### Symptom
- The native AnimPanel accepted `NumPad5` preview commands and wrote entries like `FOOD / EAT_BURGER` into `bridge.ini`, but CJ still showed no preview animation.

### Root Cause
- The CLEO bridge treated non-`PED` animation files like built-in player anims and went straight to playback logic without the required non-`PED` IFP load chain.
- `REQUEST_ANIMATION` / `HAS_ANIMATION_LOADED` was not the right bridge path here for arbitrary preview IFPs such as `FOOD`, `GANGS`, `BAR`, `SMOKING`, and similar panel entries.

### Fix
- Split preview playback into two runtime paths:
- `PED` entries stay on direct `0605`.
- Non-`PED` entries use `04ED` to load the selected IFP, wait on `04EE`, then play with `0605`, and release with `04EF` on stop/close/switch.
- Keep a lightweight cache of the last loaded preview selection so repeated preview on the same selected row does not reload immediately.

### Prevention
- In this anim panel bridge, never call live preview playback for non-`PED` entries until the IFP has positively reached the `04EE` loaded state.
- Treat `PED` as the special built-in case and all other `ifp_file` values as explicit load/release resources.
- When panel input and native logging prove the command was sent but CJ stays idle, inspect the IFP load stage before changing UI or key handling again.

## 2026-03-21 NewOpcodes String Compare Lesson

### Symptom
- The patched AnimPanel bridge failed to compile after introducing `0D49: compare_strings`.

### Root Cause
- `0D49` belongs to the `NewOpcodes` extension set, not the safe baseline opcode set for this bridge script.
- The compile helper still printed `Compiled OK`, so the real failure was only visible by reading `compile.log`.

### Fix
- Remove `0D49` from the bridge.
- Keep the cached loaded IFP name only for `04EF` release, and use integer state (`selected_result`) for cache-hit decisions instead of extension-only string comparison.

### Prevention
- Do not use `0D49` / `string_cmp` in this workspace unless `{$USE NewOpcodes}` is intentional and the runtime plugin is confirmed installed.
- For bridge/control logic here, prefer integer state or documented baseline opcodes over extension-only string helpers.
- After every compile helper success message, still read the active `compile.log` before declaring the build clean.

## 2026-03-22 Anim Panel Replay Spam Lesson

### Symptom
- The native panel logged many repeated `STATUS: Previewing food/EAT_BURGER` lines in the same short test window, but CJ still appeared not to preview animations.
- `cleo.log` stayed clean, so the failure looked silent.

### Root Cause
- The bridge replay path could restart the same selected preview again and again instead of treating it as an already-active preview.
- The bridge also diverged from the repo's older proven `AnimMenu.sc` playback flow, which uses `REQUEST_ANIMATION` / `HAS_ANIMATION_LOADED` and `TASK_PLAY_ANIM_NON_INTERRUPTABLE` for the same base-game entries.
- While preview was active, the bridge also kept the player in a stricter movement-disabled state than the older working menu path.

### Fix
- Ignore duplicate play commands for the same active `selected_result`.
- Re-align non-`PED` playback with the in-repo working menu flow:
- `REQUEST_ANIMATION`
- `WAIT 750`
- `HAS_ANIMATION_LOADED`
- `TASK_PLAY_ANIM_NON_INTERRUPTABLE`
- Relax the preview-hold movement lock so CJ can actually enter the preview animation task while the panel remains open.
- Add persistent `debug.play_stage` and `debug.play_result` keys so stop/close events do not erase the last playback result.

### Prevention
- When `native-panel.log` shows repeated preview commands for the same entry, add bridge-side dedup before changing key mapping again.
- If a repo already contains an older working script for the same gameplay feature, compare the runtime opcode path against that script before trusting an external theory.
- Do not let the final stop-stage overwrite the only useful playback-stage evidence; store last-play diagnostics separately.

## 2026-03-22 Bridge Debug Overwrite Lesson

### Symptom
- `bridge.ini` kept losing the `[debug]` section after live tests, so runtime stage evidence disappeared before inspection.

### Root Cause
- The native `AnimPanel.asi` rewrites `modloader\\AnimPanel\\cache\\bridge.ini` from scratch and only preserves `[panel]`, `[selection]`, and `[command]`.
- Any CLEO-side `[debug]` keys written into that same file are erased by the native sync path.

### Fix
- Mirror bridge playback diagnostics into a separate file such as `modloader\\AnimPanel\\logs\\bridge-debug.ini`.
- Treat `bridge.ini` as native-command transport only, not as a durable debug sink.

### Prevention
- For hybrid native+CLEO bridges in this workspace, never store durable debug state in the same `.ini` file that the native UI truncates and rewrites each frame or on selection changes.
- If a debug section "mysteriously disappears", inspect the native writer before assuming the CLEO script never reached that stage.

## 2026-03-22 Playback Verification Lesson

### Symptom
- Bridge debug could report "play path reached" while the user still saw no visible animation.

### Root Cause
- Reaching the playback opcode does not prove the actor actually entered the target animation state on this mod stack.
- Without a post-play check, bridge logs could produce false confidence (`stage` reached) even when another runtime factor prevented the animation from becoming active.

### Fix
- After each playback attempt, verify animation state with `0611: actor ... performing_animation ...`.
- If the first `TASK_PLAY_ANIM_NON_INTERRUPTABLE` call does not stick, try `0605` as a fallback.
- Emit distinct result codes (`940`/`941`) when both paths fail so the failure mode is explicit.
- Refresh `$PLAYER_ACTOR` via `01F5` before playback to avoid stale actor-handle assumptions.

### Prevention
- For animation preview bridges here, always validate "entered animation state" after requesting playback.
- Do not treat "opcode executed" as equivalent to "animation visibly active" on heavily modded stacks.

## 2026-03-22 Block Versus IfpFile Mapping Lesson

### Symptom
- Native panel preview commands and bridge stages progressed, but many selections still showed no visible animation on CJ.

### Root Cause
- Playback used the wrong selection field for the animation group parameter.
- In this catalog shape, `selection.block` is the runtime animation group to feed into play opcodes, while `selection.ifp_file` is the resource file name for load/release.
- Mixing those fields causes silent play failures on entries where they differ (for example `ifp_file=PED` with non-`ped` block groups).

### Fix
- Read `selection.block` explicitly in the bridge.
- Use `block` in play opcodes (`TASK_PLAY_ANIM_NON_INTERRUPTABLE` and fallback `0605`).
- Use `ifp_file` only for `04ED/04EE/04EF` resource lifecycle on non-PED paths.
- Keep `ped_flag` as the primary switch for whether load/release should run.

### Prevention
- For this AnimPanel bridge, treat field roles strictly:
- `anim_name` + `block` = playback call arguments
- `ifp_file` = load/release resource key
- `ped_flag` = load path selector
- Do not infer PED/non-PED behavior from string equality when `ped_flag` is already available.

## 2026-03-22 Active Anim Mod Conflict Lesson

### Symptom
- AnimPanel debugging kept stalling even after source-side fixes, and runtime behavior stayed ambiguous.

### Root Cause
- Another animation script, `modloader\\animmm\\cleo\\animmenu.cs`, was still loading in the same game session.
- Even when it was only meant as a reference example, leaving it active kept an extra animation/input script in the runtime and made isolation unreliable.

### Fix
- Disable unrelated live animation scripts before diagnosing AnimPanel.
- Keep example scripts as source references only, not as simultaneously loaded runtime mods.

### Prevention
- When the user says an older mod is "just an example", verify in `cleo.log` that it is not still loaded before continuing playback debugging.
- Do not debug one animation system while another animation browser script is active in the same session.

## 2026-03-22 Actor Lock Versus Preview Task Lesson

### Symptom
- The bridge received PED preview commands and reached the playback stages, but CJ still refused to enter the requested animation.

### Root Cause
- The bridge kept the player actor in `04D7: locked 1` state while attempting to start the preview task.
- The in-repo working menu path freezes control but does not hard-lock the actor before `TASK_PLAY_ANIM_NON_INTERRUPTABLE`.

### Fix
- During panel-visible and preview-start states, keep CJ frozen with movement/activity restrictions but leave the actor unlocked.
- Use `lockX 0 lockY 0` for the `0605` fallback path to match the simpler proven preview flow more closely.

### Prevention
- For live animation preview on CJ, do not combine `set_actor ... locked 1` with the same-frame animation task call unless that exact combination has been runtime-verified on the active mod stack.
- If a preview command reaches the bridge but no animation starts, inspect actor lock/control state before changing catalog data again.

## 2026-03-22 Zero-Lock Isolation Lesson

### Symptom
- Even after relaxing the actor hard lock, the user still wanted to rule out every CLEO-side control freeze as a cause of failed preview.

### Root Cause
- Partial control restrictions (`can_move 0`, `disable_player_mutal_activities 1`, `group_control_back 1`) can still interfere with diagnosis when the remaining playback failure is not yet isolated.

### Fix
- For the next isolation build, make `ApplyPanelLock` and `ApplyPreviewHold` effectively no-op release states:
- `can_move 1`
- `locked 0`
- `disable_player_mutal_activities 0`
- `group_control_back 0`

### Prevention
- When a user explicitly asks to eliminate player locking as a suspect, remove all bridge-side movement and activity locks, not just `set_actor ... locked 1`.

## 2026-03-22 Native Input Swallow Lesson

### Symptom
- Even after all CLEO-side locks were removed, the player still could not move with gameplay keys while the panel was open.

### Root Cause
- The native ImGui panel was still swallowing keyboard and mouse input through its own `wantsInputBlock` / `WindowProc` path.
- This can look exactly like a CJ movement lock even when the CLEO bridge has already released all player and actor locks.

### Fix
- Disable native input swallowing during isolation builds by leaving `wantsInputBlock` false in the UI render path.
- Show a visible warning in the panel for non-PED selections when those previews are intentionally disabled, so "5 does nothing" is not mistaken for another lock bug.

### Prevention
- When the user reports "character is still locked" after bridge locks are removed, inspect the native UI input-capture path before changing CLEO control opcodes again.
- If a build intentionally blocks a class of entries such as non-PED previews, surface that state on-screen, not only in logs.

## 2026-03-22 Known-Safe Smoke Test Lesson

### Symptom
- The bridge reached PED playback stages, but it was still unclear whether the real failure was the playback pipeline itself or the selected catalog entries.

### Root Cause
- A silent `940` failure alone does not distinguish between:
- a broken on-foot PED animation pipeline
- and a catalog full of context-dependent or non-preview-safe entries

### Fix
- After the selected PED entry fails, run one or more hardcoded known-safe PED smoke tests such as `HANDSUP` and `ENDCHAT_03`.
- Log the selected entry fields and the smoke-test anim name into `modloader\\AnimPanel\\logs\\bridge-debug.ini`.
- Use distinct result codes for smoke-test success so the diagnosis is binary instead of speculative.

### Prevention
- When a user wants an exact runtime diagnosis, add a known-safe animation smoke test before changing catalog generation again.
- Treat "selected entry failed" and "no PED anim can start at all" as different bugs with different fixes.

## 2026-03-22 Player-Target Fallback Lesson

### Symptom
- The bridge could hit multiple actor-target playback attempts and still fail even on known-safe PED smoke-test anims.

### Root Cause
- The actor-target preview path in the bridge (`$PLAYER_ACTOR` + `0611`) was not equivalent to the older working menu path, which targets `player` and validates with `IS_CHAR_PLAYING_ANIM player`.

### Fix
- Add a player-target fallback path that mirrors the working menu semantics more closely:
- `TASK_PLAY_ANIM_NON_INTERRUPTABLE player ...`
- `IF IS_CHAR_PLAYING_ANIM player ...`
- Keep distinct debug stages and result codes around those fallbacks.

### Prevention
- When a working reference script uses `player`-target animation commands, do not assume `$PLAYER_ACTOR` + actor-state checks are equivalent on the active mod stack.
- If actor-target smoke tests fail, add the player-target version before abandoning the playback route.

## 2026-03-22 Close-Before-Play Lesson

### Symptom
- Even hardcoded safe PED smoke-test animations failed while the native panel stayed open.

### Root Cause
- The working reference menu does not preview while the menu remains active; it closes the menu first, then transfers the player into the animation state.
- Keeping the native panel session alive during playback was the last major runtime difference after catalog, locking, and target syntax had already been ruled out.

### Fix
- Close the native panel immediately on `Play` before writing the bridge command, so the bridge executes playback with the panel inactive.

### Prevention
- If an in-game reference menu closes itself before animation playback, treat that behavior as part of the runtime contract, not just UI preference.
- When safe smoke tests still fail, compare "menu open during play" versus "menu closed before play" before assuming a deeper animation-data problem.

## 2026-03-22 Exact Control-Flow Parity Lesson

### Symptom
- Even after matching lock state, target syntax, smoke tests, and close-before-play, previews still failed.

### Root Cause
- The bridge still differed from the working menu in the exact control/task prep path.
- The reference menu explicitly does:
- `SET_PLAYER_CONTROL 0 1`
- `SET_PLAYER_ENTER_CAR_BUTTON 0 0`
- and uses immediate task clearing on the player

### Fix
- Add those exact prep/cleanup calls into the bridge around playback and stop.

### Prevention
- When a local working reference exists, copy its control/task preparation flow exactly before assuming a deeper engine limitation.
- "Almost the same" is not enough for player animation preview paths on a modded SA stack.

## 2026-03-22 Overlapping String Slots Lesson

### Symptom
- Native logs said the panel was previewing one PED anim, but bridge logs showed a different anim name such as:
- `ENDCHAT_03 -> ENDCPED`
- `IDLE_stance -> IDLEPED`
- `swat_run -> swatPED`

### Root Cause
- The bridge stored multiple `@s` string variables in consecutive local slots (`10@s`, `11@s`, `12@s`).
- Those text-label variables overlap in CLEO storage, so later reads overwrote earlier strings and corrupted the playback arguments.

### Fix
- Move each bridge string to a non-overlapping slot range:
- `10@s` = `anim_name`
- `14@s` = `ifp_file`
- `18@s` = `block`

### Prevention
- In CLEO, do not place multiple `@s` text-label variables in consecutive local slots.
- If logs show the bridge reading a different string than the native UI wrote, suspect overlapping string storage before changing animation opcodes again.

## 2026-03-22 Root-Cause Prioritization Lesson

### Symptom
- Many downstream fixes changed runtime behavior, but the real "anim never starts" bug remained for hours.

### Root Cause
- The bridge was sending corrupted animation names because overlapping CLEO string slots mangled the payload.
- That made every later runtime observation misleading:
- wrong anim name reaching playback
- safe smoke tests appearing broken
- panel-open versus panel-closed behavior looking suspicious

### Fix
- Fix payload integrity first by separating string storage, then remove temporary workarounds that were only added during diagnosis.

### Prevention
- When native logs and bridge logs disagree on string payloads, stop runtime experimentation and fix transport integrity first.
- Do not spend hours tuning opcodes, camera, locks, or UI state while the command payload itself is already proven corrupt.

## 2026-03-22 Single-Layer Native Panel Rule

### Symptom
- The hybrid native UI + CLEO bridge design for `AnimPanel` created extra failure surfaces and turned one bug into hours of misleading symptoms.

### Root Cause
- A continuously open native panel was managing UI, input, and camera in one runtime layer while animation playback lived in another.
- That split forced a transport layer (`bridge.ini` + CLEO string locals), and the transport itself became the real bug source.

### Fix
- For native UI panels of this class, keep UI, input, camera, state, and playback in the same native `.asi` layer.
- Do not introduce a CLEO bridge unless the user explicitly asks for a hybrid design after tradeoffs are explained.

### Prevention
- If a feature is already native because it needs persistent UI/input/camera management, default to single-layer native execution.
- Do not split one interactive panel feature across native + CLEO just because gameplay opcodes are convenient in CLEO.

## 2026-03-22 Plugin-SDK Native Build Integration Lesson

### Symptom
- Converting `AnimPanel` to a single native `.asi` initially failed with cascading compile and link errors in `plugin-sdk`, even though the playback code itself was valid.

### Root Cause
- The project file did not match the current `plugin-sdk` requirements:
- it was using an older language standard while the SDK now uses C++ concepts and `std::expected`
- RenderWare support was not enabled (`RW` define and `rw` include path were missing)
- the project included plugin-sdk headers without linking the small set of shared/game_sa source files that provide `FindPlayerPed`, `CPools`, `CRunningScript`, `ScriptCommands`, and path helpers

### Fix
- Move the native project to `stdcpplatest`.
- Add `RW` plus the `plugin_sa\\game_sa\\rw` include path.
- Add the minimal plugin-sdk sources needed by AnimPanel:
- `shared\\DynAddress.cpp`
- `shared\\GameVersion.cpp`
- `shared\\Patch.cpp`
- `shared\\extensions\\Paths.cpp`
- `shared\\extensions\\ScriptCommands.cpp`
- `plugin_sa\\game_sa\\common.cpp`
- `plugin_sa\\game_sa\\CPools.cpp`
- `plugin_sa\\game_sa\\CRunningScript.cpp`
- Do not add unrelated plugin-sdk sources just because they exist.

### Prevention
- When adopting `plugin-sdk` in a new native project, validate the exact language standard and RenderWare defines before debugging gameplay code.
- Start from the smallest source set that satisfies unresolved symbols, and only add more plugin-sdk translation units when the linker proves they are required.

## 2026-03-22 Open.mp Scene-Dependent Catalog Filter Lesson

### Symptom
- The larger open.mp-based catalog could still admit animation families that are technically real but unsafe or unnatural for solo CJ preview, such as driver-seat steering, shop/tattoo/haircut service scenes, vending interactions, arrests, CPR, partner actions, and door/window/trunk context poses.

### Root Cause
- Filtering only obvious vehicle libraries was not enough.
- Many unsafe entries live in generic libraries or `PED`, and only their official description text reveals that they require a second actor, a prop, a vehicle seat, or a staged world interaction.

### Fix
- Expand the generator's playable filter to use official description/notes text as a first-class exclusion signal.
- Batch-exclude scene-heavy libraries like `BAR`, `CAMERA`, `CARRY`, `CLOTHES`, `HAIRCUTS`, `INT_HOUSE`, `INT_OFFICE`, `INT_SHOP`, `MEDIC`, `SHOP`, `TATTOOS`, and `VENDING`.

### Prevention
- For the AnimPanel catalog, treat official description text as the primary safety signal, not just library names.
- If one entry from a family crashes because it is scene-dependent, remove the whole family by description/library heuristics instead of waiting for each variant to fail one by one.

## 2026-03-22 PowerShell 5 Generator Compatibility Lesson

### Symptom
- The open.mp catalog generator failed in the local shell with `ConvertFrom-Json -AsHashtable`, even though the script logic itself was valid.

### Root Cause
- The workspace shell is Windows PowerShell 5, where `-AsHashtable` is not available.

### Fix
- Replace `ConvertFrom-Json -AsHashtable` with a small recursive converter that turns `PSCustomObject` and arrays into plain hashtables/arrays.

### Prevention
- For local tooling in this workspace, assume Windows PowerShell 5 unless a script explicitly launches `pwsh`.
- Do not use PowerShell 7-only flags in generators or build helpers unless the script also enforces a PowerShell 7 runtime.

## 2026-03-22 Catalog Count Mismatch Lesson

### Symptom
- A new filtering pass was claimed, but the catalog count stayed unchanged at `1311`.

### Root Cause
- The generator was not actually completing.
- Windows PowerShell 5 compatibility bugs in the override loader (`-AsHashtable`, `ContainsKey`, and direct `.Count` assumptions on dynamic objects) stopped regeneration before the new exclusions were applied.
- That made the output file look "updated enough" while still keeping old scene-dependent families.

### Fix
- Make the generator fully PowerShell 5-safe.
- Rerun the generator and verify both:
- the new total count
- and zero string matches for the banned library/family names

### Prevention
- After any catalog filter change, never trust the intended logic alone.
- Require a post-generation proof step: check the final count and grep the output JSON for the exact banned families that were supposed to disappear.

## 2026-03-22 Native Catalog JSON Escape Lesson

### Symptom
- `F8` toggled camera/input behavior, but the panel itself would not render.
- Native log spammed `Catalog load failed: Unsupported escape sequence.`

### Root Cause
- The generator wrote JSON through PowerShell's `ConvertTo-Json`, which escaped characters like `&` and `'` as `\u0026` and `\u0027`.
- The native flat JSON parser used by `AnimPanel` does not support `\uXXXX` unicode escapes.

### Fix
- Post-process generated JSON and expand all `\uXXXX` sequences into real UTF-8 characters before writing the catalog file.

### Prevention
- After changing the catalog generator, grep the final JSON for `\\u` escapes before shipping it to the native panel.
- If `F8` affects camera/input but the panel does not draw, inspect `native-panel.log` first for catalog parse errors before touching UI code.

## 2026-03-22 Pose-Dependent Transition Animation Lesson

### Symptom
- Many animations in the same family could preview correctly, but one specific entry still hard-crashed the game.
- Example: `SUNBATHE/SBATHE_F_LIEB2SIT`.

### Root Cause
- Some entries are not free-standing idles or loops; they are pose-dependent transition clips.
- `SBATHE_F_LIEB2SIT` is explicitly a "lying to sitting position" transition and assumes a matching prior lying pose/state. Forcing it directly onto standing CJ is unsafe.

### Fix
- Exclude transition clips whose official description/name shows they require a specific prior pose, starting with `lying to sitting position` / `LieB2Sit`.

### Prevention
- Do not treat "same library" or "similar surrounding animations worked" as proof that every sibling is preview-safe.
- When a family mostly works but one entry crashes immediately on its own play attempt, classify it as a pose-dependent transition and remove that pattern from the catalog.
