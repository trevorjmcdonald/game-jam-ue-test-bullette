#include "AbilitySystem/BulletteAttributeSet.h"

#include "GameplayEffectExtension.h"

void UBulletteAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
}

void UBulletteAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		const float IncomingDamage = GetDamage();
		SetDamage(0.0f);
		if (IncomingDamage > 0.0f)
		{
			SetHealth(FMath::Clamp(GetHealth() - IncomingDamage, 0.0f, GetMaxHealth()));
		}
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}

	if (!bOutOfHealth && GetHealth() <= 0.0f)
	{
		bOutOfHealth = true;
		OnOutOfHealth.Broadcast(Data.EffectSpec.GetEffectContext().GetOriginalInstigator());
	}
}
