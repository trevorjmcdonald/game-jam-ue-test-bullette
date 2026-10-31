#include "AbilitySystem/BulletteDamage.h"

#include "AbilitySystem/BulletteGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"

bool BulletteDamage::ApplyDamage(UAbilitySystemComponent* SourceASC, AActor* Target, TSubclassOf<UGameplayEffect> EffectClass, float Amount)
{
	if (!SourceASC || !EffectClass || Amount <= 0.0f)
	{
		return false;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
	if (!TargetASC)
	{
		return false;
	}

	const FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(EffectClass, 1.0f, SourceASC->MakeEffectContext());
	if (!Spec.IsValid())
	{
		return false;
	}

	Spec.Data->SetSetByCallerMagnitude(BulletteTags::SetByCaller_Damage, Amount);
	SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, TargetASC);
	return true;
}
