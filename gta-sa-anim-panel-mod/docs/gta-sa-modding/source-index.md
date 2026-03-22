# Source Index

## Priority 1: script-authoring truth
- Sanny Builder docs: syntax, directives, compiler behavior, CLI, troubleshooting.
  - https://docs.sannybuilder.com/
  - https://docs.sannybuilder.com/editor/cli
  - https://docs.sannybuilder.com/troubleshooting/errors/0083
- Sanny Builder Library: opcode signatures and enums for SA.
  - https://github.com/sannybuilder/library/tree/master/sa
- CLEO docs and bundled examples: CLEO runtime behavior, SDK notes, examples, debug opcodes.
  - https://cleo.li/sdk.html
  - local examples under `cleo_readme/examples`
  - CLEO 5 release notes at https://github.com/cleolibrary/CLEO5/releases/tag/v5.4.0

Use these for:
- `.cs` syntax
- directives such as `{$CLEO .cs}` and `{$USE CLEO+}`
- opcode argument shapes
- compile/debug behavior
- version and plugin guards

Do not replace these with:
- MTA Lua docs
- `plugin-sdk` C++ declarations

## Priority 2: verified CLEO examples
- Working CLEO script examples:
  - https://github.com/yugecin/scmcleoscripts/tree/master/cleo
  - local installed scripts under `cleo`
  - local CLEO examples under `cleo_readme/examples`
- GTA script generator references:
  - https://github.com/wmysterio/gta-script-generator/tree/v_7.5/GTA.SA

Use these for:
- proven script structure
- mission trigger patterns
- save/load custom script patterns
- practical debug style

## Priority 3: GTA SA data references
- MTA Wiki vehicle IDs:
  - https://wiki.multitheftauto.com/wiki/Vehicle_IDs
- MTA Wiki skin IDs:
  - https://wiki.multitheftauto.com/wiki/All_Skins_Page
- MTA object list:
  - https://github.com/multitheftauto/mtasa-resources/blob/master/%5Beditor%5D/editor_gui/client/browser/objects.xml
- MTA element docs:
  - https://wiki.multitheftauto.com/wiki/Element
  - https://wiki.multitheftauto.com/wiki/Element/Player

Use these only for:
- IDs
- names
- category tables
- list lookups

Do not use these for:
- CLEO script syntax
- opcode spelling
- direct function translation into `.cs`

## Priority 4: engine research references
- `plugin-sdk`:
  - https://github.com/DK22Pac/plugin-sdk/tree/master/plugin_sa/game_sa
- `gta-reversed`:
  - https://github.com/gta-reversed/gta-reversed
- GTAMods wiki:
  - https://gtamods.com/wiki/List_of_opcodes
  - https://gtamods.com/wiki/Category:GTA_SA
  - https://gtamods.com/wiki/Create_a_mission
  - https://gtamods.com/wiki/Paths_(GTA_SA)
  - https://gtamods.com/wiki/Replays_(GTA_SA)
  - https://gtamods.com/wiki/SCM_language
  - https://gtamods.com/wiki/Vehicle_Formats#San_Andreas

Use these for:
- understanding engine behavior
- opcode history and older references
- mission and SCM concepts
- memory research when a script truly needs it

Do not use these as the first source for:
- final CLEO `.cs` syntax
- exact opcode parameter forms when Sanny Library already defines them
