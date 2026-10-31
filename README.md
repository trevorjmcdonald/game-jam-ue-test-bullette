# Bullette

A small bullet heaven game made in Unreal Engine 5.8. It's a practice run for the Epic AI Megajam: the game was built mostly by Claude Code driving the editor through Unreal's Model Context Protocol (MCP) plugin, to find out how well that workflow holds up and which of Unreal's built-in gameplay systems are worth using in a game jam.

Survive as long as you can. Your weapons fire on their own; you move, collect XP gems, and choose an upgrade each time you level up.

## Status

A complete but unbalanced loop:

- **Four weapons:** bolt, orbiting blades, chain lightning and boomerang axe, each upgradable to level 5.
- **Passive upgrades:** damage, attack speed, max health, move speed, pickup radius and heal.
- **Three enemy types:** skeleton minion, a slow tanky warrior, and a ranged mage.
- **Screens:** a HUD, level-up choices, a game over screen with restart, a main menu and a pause menu.
- **Runs on the Steam Deck** under Proton.

Not yet: audio, settings, balancing and polish.

## Controls

| Action | Keyboard | Gamepad |
|---|---|---|
| Move | WASD | Left stick |
| Pause | P or Esc | Start |
| Menus | Arrow keys + Enter, or the mouse | D-pad/stick + A |

## Unreal systems used

The project leans on Unreal's built-in systems on purpose, to learn them:

- **Gameplay Ability System:** weapons are abilities; damage, health and upgrades are gameplay effects and attributes.
- **StateTree:** runs the mage's behavior. The tree asset is generated from C++.
- **Niagara:** the chain lightning effect, triggered through a gameplay cue.
- **UMG + MVVM:** the HUD is driven by view model bindings.
- **Curve tables:** per-level weapon stats come from `SourceData/WeaponStats.csv`.
- **Enhanced Input and Gameplay Tags.**

Enemies are plain actors rather than Mass entities; about 2,000 at once still ran at 85–120 fps on the dev PC.

## Building

You need:

- Unreal Engine 5.8.
- Visual Studio 2022 with the "Game development with C++" workload (`.vsconfig` lists the components).
- [Git LFS](https://git-lfs.com/). All `.uasset`/`.umap` files and source art are stored in LFS, so install it before cloning.

Rendering uses Lumen, hardware ray tracing, Virtual Shadow Maps and Substrate (DX12, SM6), so it needs a recent GPU.

```
git lfs install
git clone https://github.com/trevorjmcdonald/game-jam-ue-test-bulette.git
```

Open `Bullette.uproject` and let Unreal build the module, or right-click it and choose Generate Visual Studio project files, then build `BulletteEditor` (Development Editor, Win64).

## Working on it with Claude Code

`CLAUDE.md` is the project guide Claude Code reads. It covers the architecture, the MCP setup (the editor serves MCP on `http://127.0.0.1:8765/mcp`, see `.mcp.json`), and many workflow gotchas found along the way. `.claude/skills/` holds the Unreal C++ skills and the project's own skills for art import and Steam Deck deployment. `tools/` has PowerShell helpers to screenshot the editor window, send key presses to it, and package and deploy to a Steam Deck.

## Credits

- Characters and weapons: [KayKit](https://kaylousberg.com) Adventurers and Skeletons packs by Kay Lousberg (CC0).
- Unreal Engine skills in `.claude/skills/ue-*` and `.agents/skills/ue-*`: [quodsoler/unreal-engine-skills](https://github.com/quodsoler/unreal-engine-skills) (MIT).

## License

The project's own code, assets and docs are under the [MIT No Attribution](LICENSE) license: use them however you like, no credit needed, no warranty. Third-party content keeps its own license (see Credits, `SourceArt/KayKit/LICENSE-*` and `.claude/skills/LICENSE-unreal-engine-skills`).
