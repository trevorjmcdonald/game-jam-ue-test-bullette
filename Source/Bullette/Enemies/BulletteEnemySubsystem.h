#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "BulletteEnemySubsystem.generated.h"

class ABulletteEnemy;

/** Tracks every live enemy in the world so weapons can find targets without scanning all actors. */
UCLASS()
class BULLETTE_API UBulletteEnemySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void RegisterEnemy(ABulletteEnemy* Enemy);
	void UnregisterEnemy(ABulletteEnemy* Enemy);

	int32 GetNumEnemies() const { return Enemies.Num(); }

	/** Nearest enemy on the XY plane within MaxRange of Location, skipping Exclude, or null. */
	ABulletteEnemy* FindNearestEnemy(const FVector& Location, float MaxRange, TConstArrayView<const AActor*> Exclude = {}) const;

	/** Up to Count enemies within MaxRange (XY), nearest first. */
	void FindNearestEnemies(const FVector& Location, float MaxRange, int32 Count, TArray<ABulletteEnemy*>& OutEnemies) const;

	/**
	 * Enemies whose collision sphere overlaps a circle of Radius around Location (XY).
	 * Returns a copy, so the caller can damage (and kill) them while iterating.
	 */
	void FindEnemiesInRadius(const FVector& Location, float Radius, TArray<ABulletteEnemy*>& OutEnemies) const;

private:
	UPROPERTY()
	TArray<TObjectPtr<ABulletteEnemy>> Enemies;
};
