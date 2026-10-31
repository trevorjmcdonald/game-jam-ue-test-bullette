#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BulletteOrbitBlades.generated.h"

class UBulletteOrbitAbility;
class UStaticMeshComponent;

/**
 * The blades of UBulletteOrbitAbility. Follows the player each tick (after movement) and spins blade meshes
 * around it. Each blade hits like a spoke from the player to the blade (distance checks against the enemy subsystem),
 * with a per-enemy cooldown.
 * Radius, speed, damage and hit interval are read from the ability every tick, so upgrades apply immediately.
 */
UCLASS(NotBlueprintable)
class BULLETTE_API ABulletteOrbitBlades : public AActor
{
	GENERATED_BODY()

public:
	ABulletteOrbitBlades();

	void Init(UBulletteOrbitAbility* InAbility, AActor* InCenter);
	void SetBladeCount(int32 NewCount);

	virtual void Tick(float DeltaSeconds) override;

private:
	TWeakObjectPtr<UBulletteOrbitAbility> Ability;
	TWeakObjectPtr<AActor> Center;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> BladeMeshes;

	/** Last time each enemy was hit, for the per-enemy cooldown. */
	TMap<TWeakObjectPtr<AActor>, double> LastHitTimes;

	double Angle = 0.0;
};
