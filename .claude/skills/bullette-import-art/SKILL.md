---
name: bullette-import-art
description: Import art into Bullette (KayKit, Quaternius, Meshy .glb/.gltf/.fbx from SourceArt/) and hook it up to the player/enemy Blueprints. Covers the ImportAssets commandlet, the PL_GLTFCharacter pipeline, KayKit rig facts and the current character art setup.
---

# Importing art into Bullette

Source files live in `SourceArt/<Pack>/`, outside `Content/`, together with the pack's licence. `$UE` is the UE 5.8 install directory (see Engine location in CLAUDE.md).

**No MCP tool imports glTF.** `SkeletalMeshTools.import_file` exists but only handles FBX/OBJ (it goes through `FbxFactory`). Import glTF with the **ImportAssets commandlet**, which works while the editor is open; the running editor picks up the new assets on its own:

```powershell
& "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "<abs>\Bullette.uproject" -run=ImportAssets `
  -source="<abs>\SourceArt\KayKit\Skeleton_Minion.glb" -dest="/Game/Art/KayKit/SkeletonMinion" `
  -nosourcecontrol -unattended -nopause
```

The import exits cleanly in about 7 s. Don't add an MCP `-ini` override (see CLAUDE.md); commandlets skip MCP auto-start on their own.

- **glTF imports go through `/Game/Art/Pipelines/PL_GLTFCharacter`,** a copy of the engine's `DefaultGLTFAssetsPipeline` with `CombineSkeletalMeshesBehavior=BySkeleton` and `bCreatePhysicsAsset=false`. The engine default splits KayKit characters into 9 skeletal meshes with 9 physics assets.
  - The override lives in `InterchangeProjectSettings.ContentImportSettings.pipelineStacks.assets.perTranslatorPipelines`, under the glTF translator.
  - `ConfigSettingsToolset` saved it to `Saved/Config/WindowsEditor/Engine.ini`, which is per-machine and not in the repo, not `DefaultEngine.ini`. On a fresh clone, recreate the override (or move it into `DefaultEngine.ini`).
- **Folder auto-import doesn't work for `.glb` or `.gltf`.** The editor's monitor only watches extensions that a legacy `UFactory` registers, and none registers glTF (FBX would work). An auto-reimport mapping from `SourceArt/` to `/Game/Art/` (editor preferences, per-machine) does nothing for `.glb`.
- **A commandlet can't save assets the editor has loaded.** Importing the Knight couldn't re-save the shared skeleton for that reason; the import still worked because the Knight reuses the existing skeleton unchanged.

## KayKit facts

- KayKit characters (CC0, Kay Lousberg) share one 41-bone rig and embed all their animations; Skeleton_Minion has 95, the Knight 76. Import result: 1 skeletal mesh, 1 skeleton, materials, a texture and one AnimSequence per clip, named `<Mesh><Clip>`. The `.glb` files are the ones from the KayKit Adventurers and Skeletons packs (kaylousberg.com / itch.io).
- The Knight import landed on the existing `Skeleton_Minion_Skeleton`, but the Barbarian (imported with the editor closed) got its own `Barbarian_ArmLeft_Skeleton`. Check which skeleton a new character got before assuming it can share animations; retargeting or assigning the shared skeleton may be needed.
- **Prop meshes (weapons, shields) keep the offset of the hand that held them,** about (-90, 50, 105) from their pivot. Get the real centre with `StaticMeshTools.get_bounds` and cancel it in whatever transform places the mesh (location = -(rotation × scale × centre)). `1H_Sword` lies flat along +Y (tip at +Y); `1H_Axe` lies flat with its handle along +Y.
- **They face +Y after import.** Unreal actors face +X, so give the mesh component a yaw of -90.

## Arena props

- `SourceArt/KayKit/HalloweenBits` and `SourceArt/KayKit/DungeonRemastered` (CC0, licences alongside) were imported as static meshes to `/Game/Art/KayKit/{HalloweenBits,DungeonRemastered}/<name>/StaticMeshes`. They're in metres like the characters; about 0.7 scale suits the 0.55-scale skeletons, 0.8-1.1 for trees.
- Every mesh uses the first import's material (`HalloweenBits/arch/Materials/HalloweenBits`, `DungeonRemastered/column/Materials/texture`); the per-mesh duplicates were deleted. Re-importing a prop brings its duplicate back, so repoint it.
- Collision was removed from all of them, so the player walks through props. Nanite is on (the import default).

## Current weapon art

- Orbiting blades: Knight `1H_Sword`, scale 1, yaw 180, centred and 30 cm below the orbit point (`GA_Weapon_Orbit.BladeTransform`).
- Boomerang: Barbarian `1H_Axe` (`/Game/Art/KayKit/Barbarian`), scale 0.7, centred on the spin pivot (`GA_Weapon_Boomerang.AxeTransform`).
- Source: `SourceArt/KayKit/Barbarian.glb` (KayKit Adventurers, CC0).

## Current character art

- **Enemy (`BP_BulletteEnemy`):** a Blueprint-added `Body` SkeletalMeshComponent showing Skeleton_Minion, at `z = -40`, yaw -90, scale 0.55. It loops `Running_A` in `AnimationSingleNode` mode, with no Anim Blueprint, `OnlyTickPoseWhenRendered` and update-rate optimisations on. The C++ placeholder cube is `bHiddenInGame`. C++ finds the mesh in `BeginPlay` (`FindComponentByClass`) and uses it for the hit pop (`HitPopScale`, `HitPopDuration`) and `DeathAnimation` (`Skeleton_MinionDeath_A`, corpse kept for `DeathDuration` 1.2 s). On the PC, 300 of them ran at 116–119 fps, about the same as the cubes.
- **Player (`BP_BullettePlayerCharacter`):** `CharacterMesh0` shows `Knight_ArmLeft` (the combined Knight mesh; the name is just the first part the pipeline saw), at `z = -88`, yaw -90, scale 0.75. The placeholder body and nose are hidden. `IdleAnimation`/`RunAnimation`/`DeathAnimation` are `KnightIdle`/`KnightRunning_A`/`KnightDeath_A`, played directly on the mesh from the character's Tick (switches at `RunSpeedThreshold`).
- **Warrior (`BP_BulletteEnemy_Warrior`):** a duplicate of the minion Blueprint. `Body` shows `Skeleton_Warrior_ArmLeft` (`/Game/Art/KayKit/SkeletonWarrior`) at scale 0.75, looping `Skeleton_WarriorWalking_D_Skeletons` (a shamble that fits a slow tank); death `Skeleton_WarriorDeath_A`.
- **Mage (`BP_BulletteEnemy_Mage`):** `Body` added with `ActorTools.add_component`, `Skeleton_Mage_ArmLeft` (`/Game/Art/KayKit/SkeletonMage`) at the minion's transform (z -40, yaw -90, scale 0.55). C++ switches `Skeleton_MageRunning_A` / `Skeleton_MageIdle_Combat`, plays `Skeleton_MageSpellcast_Shoot` when casting and `Skeleton_MageDeath_A` on death.
- The Warrior and Mage were imported with the editor closed and each got its own skeleton (`*_ArmLeft_Skeleton`), like the Barbarian. Their clips come from their own `.glb`, so that's fine.
