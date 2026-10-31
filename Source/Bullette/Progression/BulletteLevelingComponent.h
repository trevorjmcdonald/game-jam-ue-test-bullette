#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BulletteLevelingComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FBulletteLevelUpDelegate, int32 /*NewLevel*/);

/** Tracks the player's XP and level. Broadcasts once per level gained. */
UCLASS(ClassGroup = (Bullette), meta = (BlueprintSpawnableComponent))
class BULLETTE_API UBulletteLevelingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBulletteLevelingComponent();

	void AddXP(int32 Amount);

	int32 GetLevel() const { return Level; }
	int32 GetXP() const { return XP; }
	int32 GetXPForNextLevel() const;

	FBulletteLevelUpDelegate OnLevelUp;

protected:
	/** XP needed to go from level 1 to level 2. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Leveling", meta = (ClampMin = "1"))
	int32 FirstLevelXP = 5;

	/** Extra XP needed for each level after that. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Leveling", meta = (ClampMin = "0"))
	int32 XPGrowthPerLevel = 5;

private:
	int32 Level = 1;
	int32 XP = 0;
};
