#pragma once

#include "CoreMinimal.h"
#include "InputModifiers.h"
#include "JamKitInputModifierInvertLook.generated.h"

/**
 * Flips the Y of a look mapping when the player has turned on "Invert look up/down" in the options screen
 * (UJamKitSettingsSubsystem, saved with the other settings). Add it to the mouse look mapping with bGamepad off and
 * to the stick look mapping with bGamepad on, so the two can be inverted separately.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Jam Kit Invert Look"))
class JAMKIT_API UJamKitInputModifierInvertLook : public UInputModifier
{
	GENERATED_BODY()

public:
	/** Which setting this mapping follows: the controller's (true) or the mouse's (false). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	bool bGamepad = false;

protected:
	virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* PlayerInput, FInputActionValue CurrentValue, float DeltaTime) override;
};
