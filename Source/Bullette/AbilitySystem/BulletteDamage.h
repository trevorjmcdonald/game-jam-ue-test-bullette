#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"

class AActor;
class UAbilitySystemComponent;
class UGameplayEffect;

namespace BulletteDamage
{
	/**
	 * Applies EffectClass from SourceASC to Target, passing Amount as the SetByCaller.Damage magnitude.
	 * Returns false if Target has no ability system component.
	 */
	BULLETTE_API bool ApplyDamage(UAbilitySystemComponent* SourceASC, AActor* Target, TSubclassOf<UGameplayEffect> EffectClass, float Amount);
}
