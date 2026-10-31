#include "AbilitySystem/BulletteCombatSet.h"

UBulletteCombatSet::UBulletteCombatSet()
{
	// Multipliers must never read 0, even before the owner initialises the set.
	InitDamageScale(1.0f);
	InitAttackSpeed(1.0f);
}

void UBulletteCombatSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// Attack speed divides into weapon intervals, so keep it well above zero.
	if (Attribute == GetAttackSpeedAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.1f);
	}
	else
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}
