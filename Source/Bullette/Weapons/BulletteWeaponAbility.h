#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ScalableFloat.h"
#include "BulletteWeaponAbility.generated.h"

class UGameplayEffect;
class USoundBase;

/**
 * Base for auto-firing weapons. A weapon is a granted ability that activates itself as soon as it has an avatar
 * and runs until the player dies (CancelAllAbilities). Weapon upgrades raise the ability level; the per-level stats
 * are FScalableFloats pointing at rows of CT_WeaponStats.
 * Periodic weapons override Fire(), which runs every Interval. Persistent weapons override StartWeapon() instead.
 */
UCLASS(Abstract)
class BULLETTE_API UBulletteWeaponAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UBulletteWeaponAbility();

	/** Called by the player controller after it raises this weapon's level. */
	virtual void OnWeaponLevelChanged() {}

	/** Damage per hit at the current level, scaled by the owner's DamageScale. */
	float GetDamage() const;

	/** The owner's AttackSpeed multiplier. */
	float GetAttackSpeed() const;

	/** The Count stat at the current level, rounded, at least 1. */
	int32 GetCount() const;

	/** Applies DamageEffect from the owner to Target. */
	void DealDamage(AActor* Target, float Amount) const;

protected:
	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** Default: call Fire() every Interval / AttackSpeed, re-read before each wait so upgrades apply straight away. */
	virtual void StartWeapon();
	virtual void Fire() {}

	float GetStat(const FScalableFloat& Stat) const;

	/** Plays FireSound. Fire() implementations call it when they actually attack (not when there's no target). */
	void PlayFireSound() const;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Audio")
	TObjectPtr<USoundBase> FireSound;

	/** Instant effect that adds SetByCaller.Damage to the target's Damage attribute. */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<UGameplayEffect> DamageEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FScalableFloat Damage;

	/** Seconds between attacks, before AttackSpeed. Unused by persistent weapons. */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FScalableFloat Interval;

	/** Bolts, blades, jumps or axes, depending on the weapon. */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	FScalableFloat Count;

	/** Enemies farther than this from the player are not targeted. */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (ClampMin = "0", Units = "cm"))
	float Range = 1500.0f;

private:
	void WaitForNextAttack();

	UFUNCTION()
	void HandleAttackTimer();
};
