#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "JamKitMenuPlayerController.generated.h"

class UAudioComponent;
class UJamKitMenuWidget;
class USoundBase;

/**
 * Controller for the main menu map: fades in, shows MenuWidgetClass with UI-only input and the mouse cursor, applies
 * the saved volumes and plays MenuMusic. Use it from a game mode with no default pawn.
 */
UCLASS()
class JAMKIT_API AJamKitMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu")
	TSubclassOf<UJamKitMenuWidget> MenuWidgetClass;

	/** Set it to loop on the asset. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundBase> MenuMusic;

private:
	UPROPERTY()
	TObjectPtr<UJamKitMenuWidget> MenuWidget;

	UPROPERTY()
	TObjectPtr<UAudioComponent> MusicComponent;
};
