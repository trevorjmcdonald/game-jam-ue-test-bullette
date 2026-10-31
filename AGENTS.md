# AGENTS.md

This file provides guidance to Codex (claude.ai/code) when working with code in this repository.

## Project state

Bullette is a small bullet heaven game. It's a practice run of the Codex + Unreal MCP workflow ahead of the Epic AI Megajam. Assets will come from KayKit, Quaternius, and Meshy. The project uses Unreal's built-in gameplay systems (GAS, Gameplay Tags, StateTree, Niagara, UMG/MVVM, Data Registry) on purpose, partly to find out which are worth using in a jam and which are faster to rebuild. Keep scope small.

It's an Unreal Engine **5.8** C++ project created from the Blank template (`TP_Blank`, see the redirects in `Config/DefaultEngine.ini`). There's one primary game module, `Source/Bullette`. So far it has a game mode, a player controller (adds the input mapping context) and a top-down player character with placeholder meshes. It also has a lightweight chaser enemy (`ABulletteEnemy`: a pawn with no CharacterMovement, no controller and its own Tick) and a wave spawner placed in `L_Arena` that logs `EnemyStats` lines to `LogBullette`. There are no weapons, damage or real art yet. The starting map is `/Game/Maps/L_Arena`, a copy of the engine's Basic template (`Template_Default`, no World Partition). It's set as both `GameDefaultMap` and `EditorStartupMap`. `GlobalDefaultGameMode` is `BP_BulletteGameMode`. Content layout: `/Game/Maps`, `/Game/Input` (`IA_*`, `IMC_*`), `/Game/Blueprints` (`BP_*`). The directory is not a git repository.

## Engine location

The engine is a launcher install at `C:\Games\Epic Games\Games\UE_5.8` (the `.sln`/`.slnx` files reference it by relative path). The commands below use `$UE` for that path.

## Commands

Build the editor target (the one you need for editor work and tests):

```
"$UE\Engine\Build\BatchFiles\Build.bat" BulletteEditor Win64 Development -Project="C:\dev\Codex-labs\unreal-engine\Bullette\Bullette.uproject" -WaitMutex
```

- Build the standalone game target: replace `BulletteEditor` with `Bullette`.
- If the editor is open with Live Coding enabled, `Build.bat` refuses to build. Close the editor first, or recompile from inside it (Ctrl+Alt+F11).
- Regenerate `Bullette.sln`/`.slnx` after you add or remove source files or modules:
  ```
  "$UE\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" -projectfiles -project="C:\dev\Codex-labs\unreal-engine\Bullette\Bullette.uproject" -game -rocket -progress
  ```

Run automation tests headlessly (none exist yet; the filter matches on test-name prefix, so pass a full test path to run a single test):

```
"$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\dev\Codex-labs\unreal-engine\Bullette\Bullette.uproject" -ExecCmds="Automation RunTests Bullette; Quit" -unattended -nullrhi -nosplash -log
```

Logs go to `Saved/Logs/Bullette.log`.

## Build configuration

- Targets (`Source/*.Target.cs`) use `BuildSettingsVersion.V7` and `EngineIncludeOrderVersion.Unreal5_8`. Keep these in sync if you add more targets.
- `Bullette.Build.cs` has public dependencies on `Core`, `CoreUObject`, `Engine`, `InputCore`, and `EnhancedInput`. Add new engine modules (Slate, UMG, etc.) here.
- Input uses Enhanced Input (`DefaultPlayerInputClass`/`DefaultInputComponentClass` in `Config/DefaultInput.ini`). Use Input Actions and Mapping Contexts, not legacy axis/action mappings.
- Rendering (`Config/DefaultEngine.ini`): Lumen GI and reflections, Virtual Shadow Maps, hardware ray tracing, mesh distance fields, **Substrate** materials, DX12/SM6, and no static lighting. Materials should be authored for Substrate.
- Enabled plugins: `ModelingToolsEditorMode`, `VisualStudioTools`, `ModelContextProtocol` (Unreal MCP), and the MCP toolset plugins listed below. When gameplay code starts using GAS, StateTree, etc., add the matching modules to `Bullette.Build.cs` and make sure the runtime plugins (`GameplayAbilities`, `StateTree`/`GameplayStateTree`, `DataRegistry`, ...) are explicitly enabled in `Bullette.uproject`, rather than relying on toolset dependencies.
- Edit `Bullette.uproject` only while the editor is closed; the running editor rewrites it.
- Source lives in feature subfolders of `Source/Bullette/` (`Game/`, `Player/`, `Enemies/`, ...). Build settings V7 doesn't put the module root on the include path, so `Bullette.Build.cs` adds it with `PublicIncludePaths.Add(ModuleDirectory)`. Use module-relative includes (`"Enemies/BulletteEnemy.h"`, `"Bullette.h"`). Log through `LogBullette`, declared in `Bullette.h`.
- Gameplay classes are `UCLASS(Abstract)` C++ bases. Their `BP_` Blueprint subclasses assign assets (input actions, mapping contexts, meshes), and the game uses the BPs.
- **Live Coding (`LiveCodingToolset.CompileLiveCoding`) works with the editor open, including new `UCLASS`es and properties,** because re-instancing is enabled in the editor's Live Coding settings. Blueprints can be created from new classes straight away. It writes `UnrealEditor-Bullette.patch_N.dll` and leaves the main DLL alone, so the next editor launch asks to rebuild the module; accept, or run `Build.bat` while the editor is closed. Use a full build for new modules, plugin changes, or `Build.cs` dependency changes.
- Launcher-engine gotcha: engine plugins only compile on the fly from `Engine/Plugins/Marketplace/`. `VisualStudioTools` was installed directly in `Engine/Plugins/` and broke every build (`Expecting to find a type ... named 'VisualStudioTools' in 'UE5Rules'`), so it was moved to `Engine/Plugins/Marketplace/VisualStudioTools`. If a Visual Studio update reinstalls it to the old spot, move it again. Never delete or rename `Engine/Intermediate/Build/BuildRules/UE5Rules.dll`; installed engines can't rebuild it.

## Unreal MCP (editor automation)

The `ModelContextProtocol` plugin (experimental) runs an MCP server inside the editor at `http://127.0.0.1:8765/mcp`. `.mcp.json` points there, and the editor regenerates that file with the `ModelContextProtocol.GenerateClientConfig Codex` console command. Port 8000 (the default) is taken by an unrelated `Manager.exe` on this machine. If the server returns `ECONNRESET`, check that the port in Editor Preferences → Plugins → Unreal MCP matches `.mcp.json`. The editor must be running for any of these tools to work.

Tool search is on, so the server exposes only `list_toolsets`, `describe_toolset` and `call_tool`. Run `describe_toolset` before using a toolset. With `call_tool`, pass the **short** tool name (e.g. `get_current_level`) plus `toolset_name`. Passing the fully qualified name fails with "Unknown tool".

Each toolset is a separate engine plugin enabled in `Bullette.uproject`: EditorToolset, LiveCoding, AutomationTest, ConfigSettings, Plugin, GAS, GameplayTags, StateTree, Niagara, UMG, MVVM, Physics, SemanticSearch, PCG, AnimationAssistant, DataRegistry. Known gaps:
- **GAS, StateTree, and Data Registry toolsets inspect but don't author.** Write abilities, effects, attribute sets and StateTree tasks in C++, with Blueprint subclasses through `editor_toolset...BlueprintTools`. Use the GAS inspector to check runtime state in PIE.
- **The Physics toolset only covers Physics Assets (ragdolls).** Collision channels and profiles live in `Config/DefaultEngine.ini`; edit them through `ConfigSettingsToolset`.
- **AnimationAssistant covers Control Rig and Sequencer, not retargeting.** Making KayKit, Quaternius and Meshy skeletons share animations may need manual editor work.
- **UMG:** call `ObjectTools.list_properties` before `set_properties` on any widget or slot. Property names can't be guessed, and wrong ones fail silently.
- **`editor_toolset...ProgrammaticToolset`** batches many tool calls in one sandboxed Python script. Use it for bulk work such as placing many actors.
- **`ObjectTools.set_properties` on a Blueprint asset path edits its class defaults.** Class references use the `_C` generated class path. Instanced sub-objects such as input modifiers are created by passing their class path (e.g. `{"refPath": "/Script/EnhancedInput.InputModifierNegate"}`). Read the values back with `get_properties` to confirm.
- **New assets:** `DataAssetTools.create` works for any `UDataAsset` subclass (input actions, mapping contexts). `BlueprintTools.create` takes a C++ parent class (`/Script/Bullette.<Class>`). Call `AssetTools.save_assets` afterwards, because nothing is saved automatically. There's no tool for new levels; duplicate a template map instead.
- **`SceneTools.load_level` refuses if the current level has unsaved changes,** and no tool can discard them. Ask the user to switch levels themselves.
- **`EditorAppToolset.StartPIE` always reports "PIE ended before warmup completed",** but PIE keeps running. Check with `IsPIERunning`.
- **Inherited C++ components on a Blueprint** (e.g. a mesh's `OverrideMaterials`) can be set through `ObjectTools` on the class default object's component path, `/Game/.../BP_X.Default__BP_X_C:ComponentName`. The value survives a recompile. Find the paths with `BlueprintTools.get_default_object` and `ActorTools.get_components`.
- **Restarting the editor (the user has approved this when a restart is needed):**
  1. `AssetTools.save_assets([])` saves everything unsaved; check with `is_dirty`.
  2. In PowerShell, run `(Get-Process UnrealEditor).CloseMainWindow()` and `WaitForExit`. If it doesn't exit, a dialog is probably open; tell the user rather than force-killing.
  3. Run `Build.bat`, then UBT `-projectfiles` if source files were added.
  4. `Start-Process` `UnrealEditor.exe` with the `.uproject`.

  The MCP port comes up within seconds, and the tools reconnect on their own; no `/mcp` needed.
- **Enemy baseline (2026-10-30, this PC, PIE, enemies clumped on a still player):**
  - 300, 1000 and 2000 actor enemies all stay around 85–120 fps.
  - Spawning 400+ enemies in one frame causes 30–60 ms spikes.
  - So plain actors are fine for now and Mass isn't needed. Spread big waves over several frames, or pool enemies, before raising counts.
- **Measuring performance in PIE:** the editor drops to 3 fps (333 ms frames) whenever it isn't the focused window ("Use Less CPU when in Background"), and otherwise caps at 120 fps. Ignore 333 ms samples. A steady 8.33 ms only means "under the cap", not the real cost.
- **`BlueprintTools` returns a schema too large to read inline.** Its output is saved to a file; parse that file instead.
- **`LiveCodingToolset`** compiles C++ inside the running editor. Prefer it over `Build.bat`, which refuses to run while the editor is open with Live Coding enabled.

## Skills

`.Codex/skills/` holds the `ue-*` skills from github.com/quodsoler/unreal-engine-skills (MIT license, Markdown only, targets UE 5.8). Use the matching skill before writing UE C++ in its area (e.g. `ue-gameplay-abilities`, `ue-module-build-system`, `ue-input-system`). To update them, copy the repo's `skills/` folder over this one again.

## Generated directories

`Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`, and `.vs/` are build and editor output. Don't edit them by hand.
