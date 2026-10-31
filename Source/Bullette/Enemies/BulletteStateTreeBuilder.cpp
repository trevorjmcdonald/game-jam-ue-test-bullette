// Editor-only: builds and saves the enemy StateTree assets from code, because the MCP StateTree toolset can inspect
// trees but not author them. Run "Bullette.BuildStateTrees" in the editor console (or headless with
// -ExecCmds="Bullette.BuildStateTrees; Quit"). Existing assets are left alone unless the command gets "force",
// so hand edits in the StateTree editor survive.

#if WITH_EDITOR

#include "AssetRegistry/AssetRegistryModule.h"
#include "Bullette.h"
#include "Components/StateTreeComponentSchema.h"
#include "Enemies/BulletteEnemyStateTreeNodes.h"
#include "HAL/IConsoleManager.h"
#include "Misc/PackageName.h"
#include "StateTree.h"
#include "StateTreeCompilerLog.h"
#include "StateTreeEditingSubsystem.h"
#include "StateTreeEditorData.h"
#include "StateTreeState.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace BulletteStateTreeBuilder
{
	/** Root picks the first child whose enter condition passes: back off if crowded, close in if far, else attack. */
	void BuildMageTree(UStateTreeEditorData& EditorData)
	{
		UStateTreeState& Root = EditorData.AddSubTree(FName(TEXT("Root")));
		Root.SelectionBehavior = EStateTreeStateSelectionBehavior::TrySelectChildrenInOrder;

		UStateTreeState& Retreat = Root.AddChildState(FName(TEXT("Retreat")));
		Retreat.AddEnterCondition<FBulletteDistanceToPlayerCondition>().GetInstanceData().MaxDistance = 350.0f;
		{
			FBulletteMoveTaskInstanceData& Move = Retreat.AddTask<FBulletteMoveTask>().GetInstanceData();
			Move.bAway = true;
			Move.StopDistance = 550.0f;
			Move.MaxDuration = 1.0f;
		}
		Retreat.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::GotoState, &Root);

		UStateTreeState& Approach = Root.AddChildState(FName(TEXT("Approach")));
		Approach.AddEnterCondition<FBulletteDistanceToPlayerCondition>().GetInstanceData().MinDistance = 800.0f;
		{
			FBulletteMoveTaskInstanceData& Move = Approach.AddTask<FBulletteMoveTask>().GetInstanceData();
			Move.bAway = false;
			Move.StopDistance = 650.0f;
			Move.MaxDuration = 3.0f;
		}
		Approach.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::GotoState, &Root);

		UStateTreeState& Attack = Root.AddChildState(FName(TEXT("Attack")));
		{
			FBulletteCastTaskInstanceData& Cast = Attack.AddTask<FBulletteCastTask>().GetInstanceData();
			Cast.WindUp = 0.5f;
			Cast.Recovery = 1.3f;
		}
		Attack.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::GotoState, &Root);
	}

	bool BuildAsset(const FString& PackageName, void (*BuildTree)(UStateTreeEditorData&), bool bForce)
	{
		if (!bForce && FPackageName::DoesPackageExist(PackageName))
		{
			UE_LOG(LogBullette, Display, TEXT("BuildStateTrees: %s exists, skipping (pass 'force' to overwrite)"), *PackageName);
			return true;
		}

		UPackage* Package = CreatePackage(*PackageName);
		const FName AssetName(*FPackageName::GetShortName(PackageName));
		UStateTree* StateTree = NewObject<UStateTree>(Package, AssetName, RF_Public | RF_Standalone | RF_Transactional);
		UStateTreeEditorData* EditorData = NewObject<UStateTreeEditorData>(StateTree, NAME_None, RF_Transactional);
		EditorData->Schema = NewObject<UStateTreeComponentSchema>(EditorData);
		StateTree->EditorData = EditorData;

		BuildTree(*EditorData);

		FStateTreeCompilerLog Log;
		const bool bCompiled = UStateTreeEditingSubsystem::CompileStateTree(StateTree, Log);
		Log.DumpToLog(LogBullette);
		if (!bCompiled)
		{
			UE_LOG(LogBullette, Error, TEXT("BuildStateTrees: %s failed to compile"), *PackageName);
			return false;
		}

		FAssetRegistryModule::AssetCreated(StateTree);
		Package->MarkPackageDirty();

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		const FString FileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
		const bool bSaved = UPackage::SavePackage(Package, StateTree, *FileName, SaveArgs);
		UE_LOG(LogBullette, Display, TEXT("BuildStateTrees: %s compiled, saved=%d"), *PackageName, bSaved);
		return bSaved;
	}

	void Run(const TArray<FString>& Args)
	{
		const bool bForce = Args.Contains(TEXT("force"));
		BuildAsset(TEXT("/Game/Enemies/ST_SkeletonMage"), &BuildMageTree, bForce);
	}

	static FAutoConsoleCommand BuildStateTreesCommand(
		TEXT("Bullette.BuildStateTrees"),
		TEXT("Builds the enemy StateTree assets (/Game/Enemies/ST_*) from code. Pass 'force' to overwrite existing ones."),
		FConsoleCommandWithArgsDelegate::CreateStatic(&Run));
}

#endif // WITH_EDITOR
