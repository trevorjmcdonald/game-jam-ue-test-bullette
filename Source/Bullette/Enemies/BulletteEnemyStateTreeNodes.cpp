#include "Enemies/BulletteEnemyStateTreeNodes.h"

#include "Enemies/BulletteCasterEnemy.h"
#include "StateTreeExecutionContext.h"

namespace
{
	ABulletteEnemy* GetLiveEnemy(const FStateTreeExecutionContext& Context)
	{
		ABulletteEnemy* Enemy = Cast<ABulletteEnemy>(Context.GetOwner());
		return Enemy && !Enemy->IsDead() ? Enemy : nullptr;
	}
}

EStateTreeRunStatus FBulletteMoveTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	Context.GetInstanceData(*this).Elapsed = 0.0f;
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FBulletteMoveTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	ABulletteEnemy* Enemy = GetLiveEnemy(Context);
	if (!Enemy)
	{
		return EStateTreeRunStatus::Failed;
	}

	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.Elapsed += DeltaTime;
	Enemy->MoveRelativeToTarget(Data.bAway, DeltaTime);

	const double Distance = Enemy->GetDistanceToTarget();
	const bool bArrived = Data.bAway ? Distance >= Data.StopDistance : Distance <= Data.StopDistance;
	return bArrived || Data.Elapsed >= Data.MaxDuration ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FBulletteCastTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	ABulletteCasterEnemy* Caster = Cast<ABulletteCasterEnemy>(GetLiveEnemy(Context));
	if (!Caster)
	{
		return EStateTreeRunStatus::Failed;
	}

	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.Elapsed = 0.0f;
	Data.bFired = false;
	Caster->BeginCast(Data.WindUp);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FBulletteCastTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	ABulletteCasterEnemy* Caster = Cast<ABulletteCasterEnemy>(GetLiveEnemy(Context));
	if (!Caster)
	{
		return EStateTreeRunStatus::Failed;
	}

	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.Elapsed += DeltaTime;
	if (!Data.bFired && Data.Elapsed >= Data.WindUp)
	{
		Caster->FireAtTarget();
		Data.bFired = true;
	}
	return Data.Elapsed >= Data.WindUp + Data.Recovery ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

bool FBulletteDistanceToPlayerCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const ABulletteEnemy* Enemy = GetLiveEnemy(Context);
	if (!Enemy)
	{
		return false;
	}

	const FInstanceDataType& Data = Context.GetInstanceData(*this);
	const double Distance = Enemy->GetDistanceToTarget();
	return Distance >= Data.MinDistance && Distance <= Data.MaxDistance;
}
