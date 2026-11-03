// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Bullette : ModuleRules
{
	public Bullette(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Allow module-relative includes ("Bullette.h", "Enemies/BulletteEnemy.h") from any subfolder.
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "GameplayAbilities", "GameplayTags", "GameplayTasks" });

		// The Jam Kit plugin: seeded randomness, the trace, offered choices and the unattended runs (probe, scripts).
		PublicDependencyModuleNames.Add("JamKit");

		// UMG and MVVM are public because UI headers derive from UUserWidget and UMVVMViewModelBase.
		PublicDependencyModuleNames.AddRange(new string[] { "UMG", "ModelViewViewModel", "FieldNotification" });
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// UBulletteAudioSettings is a UDeveloperSettings (Project Settings > Game > Bullette Audio).
		PrivateDependencyModuleNames.Add("DeveloperSettings");

		// StateTree runs enemy behaviour (UStateTreeComponent, custom tasks and conditions).
		PublicDependencyModuleNames.AddRange(new string[] { "StateTreeModule", "GameplayStateTreeModule" });

		// Niagara: weapon VFX spawned by gameplay cue notifies.
		PrivateDependencyModuleNames.Add("Niagara");

		// Editor-only: BulletteStateTreeBuilder.cpp builds and compiles StateTree assets from code.
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "StateTreeEditorModule", "PropertyBindingUtils", "AssetRegistry" });
		}

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
