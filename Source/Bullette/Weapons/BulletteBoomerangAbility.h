#pragma once

#include "CoreMinimal.h"
#include "Weapons/BulletteWeaponAbility.h"
#include "BulletteBoomerangAbility.generated.h"

class UStaticMesh;

/** Throws Count axes, fanned out around the nearest enemy, that fly out to Distance and come back to the player. */
UCLASS(Abstract)
class BULLETTE_API UBulletteBoomerangAbility : public UBulletteWeaponAbility
{
	GENERATED_BODY()

protected:
	virtual void Fire() override;

	/** How far each axe flies before turning back. */
	UPROPERTY(EditDefaultsOnly, Category = "Boomerang")
	FScalableFloat Distance;

	UPROPERTY(EditDefaultsOnly, Category = "Boomerang", meta = (ClampMin = "1", Units = "cm/s"))
	float Speed = 1400.0f;

	/** Enemies within this distance of an axe are hit (once per enemy on the way out, once on the way back). */
	UPROPERTY(EditDefaultsOnly, Category = "Boomerang", meta = (ClampMin = "1", Units = "cm"))
	float HitRadius = 70.0f;

	/** Angle between neighbouring axes when several are thrown at once. */
	UPROPERTY(EditDefaultsOnly, Category = "Boomerang", meta = (ClampMin = "0", Units = "deg"))
	float SpreadAngle = 25.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boomerang|Visual")
	TObjectPtr<UStaticMesh> AxeMesh;

	/** Axe mesh offset, rotation and scale relative to the spinning pivot. */
	UPROPERTY(EditDefaultsOnly, Category = "Boomerang|Visual")
	FTransform AxeTransform;

	UPROPERTY(EditDefaultsOnly, Category = "Boomerang|Visual", meta = (Units = "deg"))
	float SpinSpeed = 900.0f;
};
