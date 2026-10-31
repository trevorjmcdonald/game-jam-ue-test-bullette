#include "Weapons/BulletteBoltAbility.h"

#include "AbilitySystemComponent.h"
#include "Enemies/BulletteEnemy.h"
#include "Enemies/BulletteEnemySubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Weapons/BulletteProjectile.h"

void UBulletteBoltAbility::Fire()
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	const UBulletteEnemySubsystem* Enemies = GetWorld()->GetSubsystem<UBulletteEnemySubsystem>();
	if (!Avatar || !Enemies || !ensureMsgf(ProjectileClass, TEXT("%s has no ProjectileClass"), *GetName()))
	{
		return;
	}

	TArray<ABulletteEnemy*> Targets;
	Enemies->FindNearestEnemies(Avatar->GetActorLocation(), Range, GetCount(), Targets);

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const float HitDamage = GetDamage();
	for (const ABulletteEnemy* Target : Targets)
	{
		// Fire level with the target so the sweep lines up with enemy collision.
		FVector SpawnLocation = Avatar->GetActorLocation();
		SpawnLocation.Z = Target->GetActorLocation().Z;
		FVector Direction = Target->GetActorLocation() - SpawnLocation;
		Direction.Z = 0.0;
		const FTransform SpawnTransform(Direction.Rotation(), SpawnLocation);

		ABulletteProjectile* Projectile = GetWorld()->SpawnActorDeferred<ABulletteProjectile>(
			ProjectileClass, SpawnTransform, Avatar, Cast<APawn>(Avatar), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (Projectile)
		{
			Projectile->InitDamage(ASC, DamageEffect, HitDamage);
			Projectile->FinishSpawning(SpawnTransform);
		}
	}
}
