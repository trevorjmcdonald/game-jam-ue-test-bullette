#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BulletteMenuPlayerController.generated.h"

class UAudioComponent;
class UBulletteMenuWidget;
class USoundBase;

/** Controller for the main menu map: shows MenuWidgetClass with UI-only input and the mouse cursor. */
UCLASS(Abstract)
class BULLETTE_API ABulletteMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu")
	TSubclassOf<UBulletteMenuWidget> MenuWidgetClass;

	/** Set it to loop on the asset. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundBase> MenuMusic;

private:
	UPROPERTY()
	TObjectPtr<UBulletteMenuWidget> MenuWidget;

	UPROPERTY()
	TObjectPtr<UAudioComponent> MusicComponent;
};
