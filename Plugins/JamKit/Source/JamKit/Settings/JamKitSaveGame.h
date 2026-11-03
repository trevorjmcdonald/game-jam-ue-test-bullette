#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "JamKitSaveGame.generated.h"

/**
 * The player's settings, kept between sessions (key bindings excepted: Enhanced Input saves those in its own slot).
 * Owned by UJamKitSettingsSubsystem. Add new fields with defaults; old saves load them as the default.
 * Game progress (best runs, unlocks) belongs in the game's own save, not here.
 */
UCLASS()
class JAMKIT_API UJamKitSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	float MasterVolume = 1.0f;

	UPROPERTY()
	float MusicVolume = 1.0f;

	UPROPERTY()
	float EffectsVolume = 1.0f;

	/** "Invert look up/down", separately for the mouse and the controller (UJamKitInputModifierInvertLook). */
	UPROPERTY()
	bool bInvertLookMouse = false;

	UPROPERTY()
	bool bInvertLookGamepad = false;
};
