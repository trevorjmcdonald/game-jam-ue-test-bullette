#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BulletteHUDWidget.generated.h"

class UBulletteHUDViewModel;

/**
 * In-run HUD. Owns a UBulletteHUDViewModel, gives it to the widget's MVVM view, and refreshes it every frame
 * from the player's attributes, leveling component and the game mode. The layout and bindings live in WBP_HUD.
 */
UCLASS(Abstract)
class BULLETTE_API UBulletteHUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY()
	TObjectPtr<UBulletteHUDViewModel> ViewModel;
};
