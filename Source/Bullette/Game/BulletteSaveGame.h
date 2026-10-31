#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "BulletteSaveGame.generated.h"

/**
 * Everything the game keeps between sessions except key bindings, which Enhanced Input saves in its own slot.
 * Owned by UBulletteSettingsSubsystem. Add new fields with defaults; old saves load them as the default.
 */
UCLASS()
class UBulletteSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	float MasterVolume = 1.0f;

	UPROPERTY()
	float MusicVolume = 1.0f;

	UPROPERTY()
	float EffectsVolume = 1.0f;

	/** Longest run so far, and its kills. 0 means no run has been finished yet. */
	UPROPERTY()
	float BestSurvivedSeconds = 0.0f;

	UPROPERTY()
	int32 BestKills = 0;
};
