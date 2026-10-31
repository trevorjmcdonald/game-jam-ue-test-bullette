#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "BulletteCombatSet.generated.h"

/**
 * Player stats that upgrades modify. Starting values come from the player character;
 * upgrades are Infinite gameplay effects that modify these attributes.
 * Per-weapon numbers live on the weapon abilities; these are the player-wide multipliers and movement stats.
 */
UCLASS()
class BULLETTE_API UBulletteCombatSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UBulletteCombatSet();

	/** Multiplier on every weapon's damage. Starts at 1. */
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	FGameplayAttributeData DamageScale;
	ATTRIBUTE_ACCESSORS_BASIC(UBulletteCombatSet, DamageScale)

	/** Multiplier on every weapon's attack rate (divides intervals, speeds up orbits). Starts at 1. */
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	FGameplayAttributeData AttackSpeed;
	ATTRIBUTE_ACCESSORS_BASIC(UBulletteCombatSet, AttackSpeed)

	/** Walk speed in cm/s. */
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS_BASIC(UBulletteCombatSet, MoveSpeed)

	/** XP gems inside this radius (cm) fly to the player. */
	UPROPERTY(BlueprintReadOnly, Category = "Pickup")
	FGameplayAttributeData PickupRadius;
	ATTRIBUTE_ACCESSORS_BASIC(UBulletteCombatSet, PickupRadius)

protected:
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
};
