#include "Enemies/BulletteCasterEnemy.h"

#include "AbilitySystemComponent.h"
#include "Components/StateTreeComponent.h"
#include "Enemies/BulletteEnemyProjectile.h"
#include "Engine/World.h"
#include "StateTree.h"

ABulletteCasterEnemy::ABulletteCasterEnemy()
{
	Brain = CreateDefaultSubobject<UStateTreeComponent>(TEXT("Brain"));
	// Started in BeginPlay once Behavior is assigned to it.
	Brain->SetStartLogicAutomatically(false);
}

void ABulletteCasterEnemy::BeginPlay()
{
	Super::BeginPlay();

	if (ensureMsgf(Behavior, TEXT("%s has no Behavior StateTree"), *GetName()))
	{
		Brain->SetStateTree(Behavior);
		Brain->StartLogic();
	}
}

void ABulletteCasterEnemy::OnDied()
{
	Brain->StopLogic(TEXT("Died"));
}

void ABulletteCasterEnemy::BeginCast(float WindUpSeconds)
{
	FaceTarget();
	PlayBodyAnimation(CastAnimation, false, WindUpSeconds + 0.3f);
}

void ABulletteCasterEnemy::FireAtTarget()
{
	const AActor* TargetActor = GetTarget();
	if (!TargetActor || IsDead() || !ensureMsgf(ProjectileClass, TEXT("%s has no ProjectileClass"), *GetName()))
	{
		return;
	}

	FaceTarget();
	const FVector Start = GetActorLocation() + FVector(0.0, 0.0, ProjectileHeight);
	FVector Direction = TargetActor->GetActorLocation() - Start;
	Direction.Z = 0.0;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ABulletteEnemyProjectile* Projectile = GetWorld()->SpawnActor<ABulletteEnemyProjectile>(ProjectileClass, Start, Direction.Rotation(), SpawnParams))
	{
		Projectile->Init(AbilitySystemComponent, ContactDamageEffect, ProjectileDamage, GetTarget());
	}
}
