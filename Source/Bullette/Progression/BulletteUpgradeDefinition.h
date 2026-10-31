#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Templates/SubclassOf.h"
#include "Weapons/BulletteWeaponAbility.h"
#include "BulletteUpgradeDefinition.generated.h"

class UGameplayEffect;

/**
 * One level-up choice. A passive applies Effect to the player. A weapon grants WeaponAbility on the first pick
 * and raises its ability level on each later pick, so MaxPicks is the weapon's max level.
 */
UCLASS(BlueprintType)
class BULLETTE_API UBulletteUpgradeDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrade")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrade", meta = (MultiLine = true))
	FText Description;

	/** Shown instead of Description once the player already has this upgrade. Leave empty to always show Description. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrade", meta = (MultiLine = true))
	FText LevelUpDescription;

	/** Passives: Infinite effects for permanent stat boosts; Instant effects for one-offs like healing. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrade")
	TSubclassOf<UGameplayEffect> Effect;

	/** Weapons: the ability granted on the first pick. Leave Effect empty. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrade")
	TSubclassOf<UBulletteWeaponAbility> WeaponAbility;

	/** How many times this can be picked in one run. 0 means no limit. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrade", meta = (ClampMin = "0"))
	int32 MaxPicks = 5;

	bool IsWeapon() const { return WeaponAbility != nullptr; }
};
