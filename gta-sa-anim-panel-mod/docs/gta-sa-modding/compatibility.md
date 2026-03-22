# Compatibility Policy

## Safe baseline
- Default compatibility target is `Classic GTA San Andreas 1.0 US + CLEO 5 + CLEO+ + Sanny Builder 4`.
- This is the baseline to optimize for unless the user asks for broader support.

## What "compatible" means here
- The script compiles cleanly with the local Sanny Builder CLI.
- The script avoids unsupported syntax and bogus opcode signatures.
- Runtime-sensitive behavior is guarded when it depends on CLEO 5, CLEO+, NewOpcodes, or installation-specific plugins.

## Default assumptions
- CLEO 5 is available.
- CLEO+ is available in this workspace.
- NewOpcodes is not assumed unless the specific mod truly needs it.
- `plugin-sdk` and `gta-reversed` are research sources only, not direct script-authoring sources.

## When to add guards
- Add a version or plugin guard before using:
  - CLEO 5-only opcodes such as debug helpers
  - CLEO+ opcodes and types
  - NewOpcodes-specific behavior
  - memory patches or offsets that may vary by game build
- If a guard fails, show a short user-facing message and terminate safely.

## What not to promise
- Do not claim "all GTA SA versions" by default.
- Do not assume mobile, Steam, 1.01, downgrader variants, or modded executables behave like SA 1.0 US.
- Do not ship unguarded memory-address logic and call it universal.

## Headerless model rule
- Sanny error `0083: Unknown model ID` is common in headerless or `{$EXTERNAL}` cases.
- In those cases use numeric in-game model IDs instead of model names when resolution is unreliable.
- Model/name lookup is a data task; use trusted ID lists, then feed the final numeric value into CLEO script syntax.
