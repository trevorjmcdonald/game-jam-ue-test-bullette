#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "BulletteBeamCueNotify.generated.h"

class UNiagaraSystem;

/**
 * One-shot beam cue: spawns BeamSystem at the effect context's origin and sets its User.BeamEnd parameter to the
 * cue's Location. Used by chain lightning (GC_Weapon_ChainLightning in /Game/Abilities, the cue scan path).
 */
UCLASS(Abstract)
class BULLETTE_API UBulletteBeamCueNotify : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;

protected:
	/** Needs a world-space Vector user parameter named BeamEnd. */
	UPROPERTY(EditDefaultsOnly, Category = "Beam")
	TObjectPtr<UNiagaraSystem> BeamSystem;
};
