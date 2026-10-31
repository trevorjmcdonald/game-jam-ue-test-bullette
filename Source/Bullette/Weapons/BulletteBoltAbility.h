#pragma once

#include "CoreMinimal.h"
#include "Weapons/BulletteWeaponAbility.h"
#include "BulletteBoltAbility.generated.h"

class ABulletteProjectile;

/** The starting weapon: fires Count projectiles per attack, each at a different one of the nearest enemies. */
UCLASS(Abstract)
class BULLETTE_API UBulletteBoltAbility : public UBulletteWeaponAbility
{
	GENERATED_BODY()

protected:
	virtual void Fire() override;

	UPROPERTY(EditDefaultsOnly, Category = "Bolt")
	TSubclassOf<ABulletteProjectile> ProjectileClass;
};
