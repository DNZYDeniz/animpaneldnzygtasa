# Workflow

## Authoring baseline
- Write source as text at `modding-src/<mod-name>/<mod-name>.txt`.
- Start with `{$CLEO .cs}` and keep the script complete from top to bottom.
- Prefer standard opcodes and clean syntax first. Add CLEO 5, CLEO+, or NewOpcodes only if the feature actually needs them.
- In infinite loops always include `wait 0` or a deliberate throttle.
- Use short debug strings during development and remove noisy traces unless they are still useful.

## Compile loop
- Compile with:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\compile-cleo.ps1 `
  -InputPath .\modding-src\<mod-name>\<mod-name>.txt `
  -OutputPath .\modloader\<ModName>\cleo\<ModName>.cs
```

- The helper calls `D:\GTASAVIDEOCEKME\SannyBuilder\sanny.exe --compile`.
- The helper defaults to CLI mode `sa`, which is the reliable local compile mode in this installation. If a specific source is known to require SBL-only syntax and the local CLI mode is confirmed to support it, override with `-Mode sa_sbl`.
- If compilation fails, read the emitted error summary and inspect the freshest `compile.log`.
- Fix the source and rerun until the helper exits cleanly.

## Runtime debug flow
- Primary runtime evidence:
  - `cleo.log`
  - optional `cleo_script.log` when enabled in CLEO debug tooling
  - `debug_on`
  - `trace`
  - `log_to_file`
  - short `show_formatted_text_lowpriority` messages when needed
- Do not treat the Sanny native debugger as the main debugger for `.cs` CLEO scripts. It is for `main.scm` and SA v1.0, not the normal CLEO debug path.

## Safety checks
- Before using a non-baseline plugin feature, confirm the plugin is actually installed or guard the path.
- For headerless/header-light scripts and object/vehicle/ped models, prefer numeric IDs if model-name resolution is unreliable.
- If the script allocates resources, make cleanup explicit.
- If the mod is designed to stop itself, terminate with the correct custom-script opcode rather than falling through undefined control flow.
