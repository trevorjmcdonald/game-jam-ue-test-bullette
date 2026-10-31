#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BulletteProjectile.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UStaticMeshComponent;

/**
 * Straight-line projectile that sweeps the Projectile trace channel itself each tick.
 * Doing its own sweep (instead of ProjectileMovementComponent) means it also hits enemies it spawns inside,
 * which happens constantly when enemies crowd the player.
 */
UCLASS(Abstract)
class BULLETTE_API ABulletteProjectile : public AActor
{
	GENERATED_BODY()

public:
	ABulletteProjectile();

	virtual void Tick(float DeltaSeconds) override;

	/** Call between SpawnActorDeferred and FinishSpawning. */
	void InitDamage(UAbilitySystemComponent* InSourceASC, TSubclassOf<UGameplayEffect> InDamageEffect, float InDamage);

protected:
	/** Stand-in visual until real projectile art or VFX exists. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Placeholder")
	TObjectPtr<UStaticMeshComponent> PlaceholderMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "0", Units = "cm/s"))
	float Speed = 2000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "1", Units = "cm"))
	float CollisionRadius = 15.0f;

private:
	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;
	TSubclassOf<UGameplayEffect> DamageEffect;
	float Damage = 0.0f;
};
