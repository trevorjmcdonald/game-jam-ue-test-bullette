#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BulletteDeckSettingsSubsystem.generated.h"

/**
 * Applies a low-spec graphics profile when running on a Steam Deck.
 * Detection: Steam sets SteamDeck=1 for games it launches on the Deck (including Windows builds under Proton).
 * Pass -SteamDeck on the command line to preview the profile on a PC. Never applies in the editor.
 */
UCLASS()
class BULLETTE_API UBulletteDeckSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	static bool IsRunningOnSteamDeck();
};
