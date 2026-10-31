#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "StateTreeTaskBase.h"
#include "BulletteEnemyStateTreeNodes.generated.h"

// StateTree nodes for enemies run by a UStateTreeComponent (StateTree Component schema).
// They reach the enemy through the execution context's owner, so the tree needs no property bindings.

USTRUCT()
struct FBulletteMoveTaskInstanceData
{
	GENERATED_BODY()

	/** Move away from the player instead of toward it. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bAway = false;

	/** Succeeds once the player is this close (toward) or this far (away). */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0", Units = "cm"))
	float StopDistance = 600.0f;

	/** Succeeds after this long even if StopDistance wasn't reached. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.1", Units = "s"))
	float MaxDuration = 2.0f;

	UPROPERTY()
	float Elapsed = 0.0f;
};

/** Moves the enemy toward or away from the player at its MoveSpeed. */
USTRUCT(meta = (DisplayName = "Bullette Move Relative To Player", Category = "Bullette"))
struct BULLETTE_API FBulletteMoveTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FBulletteMoveTaskInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

USTRUCT()
struct FBulletteCastTaskInstanceData
{
	GENERATED_BODY()

	/** Time from starting the cast to the projectile leaving. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0", Units = "s"))
	float WindUp = 0.5f;

	/** Time after firing before the task succeeds (the attack cooldown). */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0", Units = "s"))
	float Recovery = 1.2f;

	UPROPERTY()
	float Elapsed = 0.0f;

	UPROPERTY()
	bool bFired = false;
};

/** ABulletteCasterEnemy only: wind up, fire one projectile at the player, then wait out Recovery. */
USTRUCT(meta = (DisplayName = "Bullette Cast At Player", Category = "Bullette"))
struct BULLETTE_API FBulletteCastTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FBulletteCastTaskInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

USTRUCT()
struct FBulletteDistanceToPlayerConditionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0", Units = "cm"))
	float MinDistance = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0", Units = "cm"))
	float MaxDistance = 100000.0f;
};

/** True while the player is between MinDistance and MaxDistance (XY) from the enemy. */
USTRUCT(meta = (DisplayName = "Bullette Distance To Player", Category = "Bullette"))
struct BULLETTE_API FBulletteDistanceToPlayerCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FBulletteDistanceToPlayerConditionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
