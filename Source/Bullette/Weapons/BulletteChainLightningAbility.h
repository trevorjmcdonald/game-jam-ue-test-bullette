#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Weapons/BulletteWeaponAbility.h"
#include "BulletteChainLightningAbility.generated.h"

class UMaterialInterface;

/** Strikes the nearest enemy, then jumps Count more times, each to the nearest enemy not yet hit by this strike. */
UCLASS(Abstract)
class BULLETTE_API UBulletteChainLightningAbility : public UBulletteWeaponAbility
{
	GENERATED_BODY()

protected:
	virtual void Fire() override;

	/** How far one jump can reach from the last enemy hit. */
	UPROPERTY(EditDefaultsOnly, Category = "Chain Lightning", meta = (ClampMin = "0", Units = "cm"))
	float JumpRange = 450.0f;

	/** Gameplay cue executed for each jump (EffectContext origin = start, Location = end). If unset, BeamMaterial cylinders are drawn instead. */
	UPROPERTY(EditDefaultsOnly, Category = "Chain Lightning|Visual", meta = (Categories = "GameplayCue"))
	FGameplayTag BeamCue;

	/** Placeholder visual, used only without BeamCue. */
	UPROPERTY(EditDefaultsOnly, Category = "Chain Lightning|Visual")
	TObjectPtr<UMaterialInterface> BeamMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Chain Lightning|Visual", meta = (ClampMin = "1", Units = "cm"))
	float BeamThickness = 12.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Chain Lightning|Visual", meta = (ClampMin = "0.01", Units = "s"))
	float BeamDuration = 0.15f;

private:
	void ShowBeam(const FVector& From, const FVector& To) const;
};
