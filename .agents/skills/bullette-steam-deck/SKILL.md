---
name: bullette-steam-deck
description: Package Bullette and deploy, run or measure it on the user's Steam Deck (tools/deploy-deck.ps1, Win64 under Proton, VC++ runtime, Deck graphics profile, Deck performance baseline and logs, packaging/cook gotchas).
---

# Bullette on the Steam Deck

The Deck runs the **Win64 build under Proton**. Native Linux would need the launcher's Linux target support and Epic's `v26_clang-20.1.8` toolchain, neither of which is installed.

```powershell
powershell -File tools/deploy-deck.ps1 -Package       # package (Win64 Development into Packaged/) and copy to ~/Games/bullette
powershell -File tools/deploy-deck.ps1                # copy the existing package (refuses if it's older than Source/Content/Config)
powershell -File tools/deploy-deck.ps1 -AddToSteam    # first deploy only: adds the library entry (Steam must be running)
```

- **`-Package` needs the editor closed** (the script refuses otherwise). It builds both the game and editor targets before cooking: the cook runs the editor DLL, and properties that only exist in a Live Coding patch are silently dropped from cooked Blueprints. Save everything, close the editor (see the restart procedure in AGENTS.md), package, then relaunch it.
- **SSH:** goes through WSL's `steamdeck` alias, shared with `../../srpg`: `wsl ssh steamdeck`. "Deck not found" means it's asleep, off or on another network. Tell the user; don't scan for it.
- **VC++ runtime:** Proton's per-game prefix has no VC++ redistributable. Packaging therefore ships the runtime DLLs next to the exes, using `-applocaldirectory=$(EngineDir)/Binaries/ThirdParty/AppLocalDependencies` (the deploy script passes it) and `IncludeAppLocalPrerequisites` in the project's Packaging settings. Without them the launcher stub stops with "missing Microsoft Visual C++ 2015-2022 Redistributable".
- **On the Deck, once:** in the game's Properties > Compatibility, force a Proton version. Launch it from the Steam library; launching over SSH never shows up in gamescope.
- **Graphics:** `UBulletteDeckSettingsSubsystem` applies Low scalability (Medium textures) when `SteamDeck=1` is set. Pass `-SteamDeck` to preview it on a PC. It logs `DeckProfile applied`.
  - **Detection doesn't work yet.** On the first Deck run the line never appeared, although the code is in the exe, so Steam apparently doesn't set `SteamDeck=1` for a non-Steam Proton shortcut. Unconfirmed: check `/proc/<pid>/environ` while the game runs.
  - A sturdier check would be to read the DMI product name through Wine's `Z:` drive (`Z:\sys\class\dmi\id\product_name`, which is `Jupiter` on an LCD Deck and `Galileo` on an OLED).
- **Deck baseline (2026-10-30):** the user's Deck is an **OLED (90 Hz)**. With full PC settings (no Deck profile) and placeholder cubes, 300 enemies held a **steady 90 fps** (11.1 ms) under Proton with D3D12. Big wave spawns dipped to about 70 fps. The first run after a deploy hitches (PSO cache misses, frames up to 360 ms), and the second run is smooth.
- **Performance on the Deck:** read `~/Games/bullette/Bullette/Saved/Logs/Bullette.log` over SSH. It's a Development build, so the `EnemyStats` and `RunOver` lines are there. Deploys keep that `Saved/` folder.
- **Packaging gotchas:**
  - The first cook took about 17 minutes, almost all of it shader compilation for ray tracing, Lumen, VSM and Substrate permutations. Shaders are cached in the DDC afterwards.
  - **The cook process (`UnrealEditor-Cmd -run=Cook`) hangs at exit at 0% CPU after printing "Success"** — every `-Package` run so far (47 minutes the first time, 2 hours the second). MCP isn't the cause: commandlets never start the MCP server. The hung process's only child is `zenserver.exe` (the DDC server), so the shutdown wait on Zen is the next suspect.
  - **Workaround:** when the log shows `Execution of commandlet took` and then stops after "Exiting ShaderCompilingThread", check the process is idle (CPU time not increasing), kill `UnrealEditor-Cmd` (not `zenserver`), then stage from the complete `Saved/Cooked` output and deploy:
    ```powershell
    & "$UE\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="<abs>\Bullette.uproject" -noP4 -platform=Win64 -clientconfig=Development `
      -skipbuild -skipcook -stage -pak -archive -archivedirectory="<abs>\Packaged" `
      -applocaldirectory='$(EngineDir)/Binaries/ThirdParty/AppLocalDependencies' -utf8output
    powershell -File tools/deploy-deck.ps1
    ```
    The staging run takes about a minute. The whole cook (with cached shaders) took about 15 minutes.
