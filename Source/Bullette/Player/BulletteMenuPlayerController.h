#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BulletteMenuPlayerController.generated.h"

class UBulletteMenuWidget;

/** Controller for the main menu map: shows MenuWidgetClass with UI-only input and the mouse cursor. */
UCLASS(Abstract)
class BULLETTE_API ABulletteMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu")
	TSubclassOf<UBulletteMenuWidget> MenuWidgetClass;

private:
	UPROPERTY()
	TObjectPtr<UBulletteMenuWidget> MenuWidget;
};
