#include "Progression/BulletteLevelingComponent.h"

#include "Bullette.h"

UBulletteLevelingComponent::UBulletteLevelingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

int32 UBulletteLevelingComponent::GetXPForNextLevel() const
{
	return FirstLevelXP + (Level - 1) * XPGrowthPerLevel;
}

void UBulletteLevelingComponent::AddXP(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	XP += Amount;
	while (XP >= GetXPForNextLevel())
	{
		XP -= GetXPForNextLevel();
		++Level;
		UE_LOG(LogBullette, Display, TEXT("LevelUp level=%d"), Level);
		OnLevelUp.Broadcast(Level);
	}
}
