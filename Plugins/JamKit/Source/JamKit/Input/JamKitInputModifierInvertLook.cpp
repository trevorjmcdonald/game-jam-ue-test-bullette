#include "Input/JamKitInputModifierInvertLook.h"

#include "EnhancedPlayerInput.h"
#include "GameFramework/PlayerController.h"
#include "Settings/JamKitSettingsSubsystem.h"

FInputActionValue UJamKitInputModifierInvertLook::ModifyRaw_Implementation(const UEnhancedPlayerInput* PlayerInput, FInputActionValue CurrentValue, float DeltaTime)
{
	const APlayerController* PlayerController = PlayerInput ? PlayerInput->GetOuterAPlayerController() : nullptr;
	const UJamKitSettingsSubsystem* Settings = UJamKitSettingsSubsystem::Get(PlayerController);
	if (!Settings || !Settings->GetInvertLook(bGamepad))
	{
		return CurrentValue;
	}
	FVector Value = CurrentValue.Get<FVector>();
	Value.Y = -Value.Y;
	return FInputActionValue(CurrentValue.GetValueType(), Value);
}
