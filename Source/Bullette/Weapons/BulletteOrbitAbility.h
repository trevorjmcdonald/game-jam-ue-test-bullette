#pragma once

#include "CoreMinimal.h"
#include "Weapons/BulletteWeaponAbility.h"
#include "BulletteOrbitAbility.generated.h"

class ABulletteOrbitBlades;
class UStaticMesh;

/** Count blades circle the player and damage enemies they touch. Persistent: spawns the blades once and keeps them. */
UCLASS(Abstract)
class BULLETTE_API UBulletteOrbitAbility : public UBulletteWeaponAbility
{
	GENERATED_BODY()

public:
	virtual void OnWeaponLevelChanged() override;

	float GetOrbitRadius() const { return GetStat(OrbitRadius); }

	/** Degrees per second, scaled by AttackSpeed. */
	float GetOrbitSpeed() const { return GetStat(OrbitSpeed) * GetAttackSpeed(); }

	/** Seconds before the same enemy can be hit again, shortened by AttackSpeed. */
	float GetHitInterval() const { return HitInterval / GetAttackSpeed(); }

	float GetHitRadius() const { return HitRadius; }

	UStaticMesh* GetBladeMesh() const { return BladeMesh; }
	const FTransform& GetBladeTransform() const { return BladeTransform; }

protected:
	virtual void StartWeapon() override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** Distance from the player to each blade. */
	UPROPERTY(EditDefaultsOnly, Category = "Orbit")
	FScalableFloat OrbitRadius;

	/** Degrees per second. */
	UPROPERTY(EditDefaultsOnly, Category = "Orbit")
	FScalableFloat OrbitSpeed;

	/** Enemies within this distance of the line from the player to a blade are hit. */
	UPROPERTY(EditDefaultsOnly, Category = "Orbit", meta = (ClampMin = "1", Units = "cm"))
	float HitRadius = 60.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Orbit", meta = (ClampMin = "0.05", Units = "s"))
	float HitInterval = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Orbit|Visual")
	TObjectPtr<UStaticMesh> BladeMesh;

	/** Blade mesh offset, rotation and scale relative to its orbit point (X points along the direction of travel). */
	UPROPERTY(EditDefaultsOnly, Category = "Orbit|Visual")
	FTransform BladeTransform;

private:
	UPROPERTY()
	TObjectPtr<ABulletteOrbitBlades> Blades;
};
