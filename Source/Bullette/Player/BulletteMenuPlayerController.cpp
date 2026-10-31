#include "Player/BulletteMenuPlayerController.h"

#include "Components/Button.h"
#include "UI/BulletteMenuWidget.h"

void ABulletteMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController() || !ensureMsgf(MenuWidgetClass, TEXT("%s has no MenuWidgetClass assigned"), *GetName()))
	{
		return;
	}

	MenuWidget = CreateWidget<UBulletteMenuWidget>(this, MenuWidgetClass);
	MenuWidget->AddToViewport();

	FInputModeUIOnly InputMode;
	if (UButton* FocusButton = MenuWidget->GetDefaultFocusButton())
	{
		InputMode.SetWidgetToFocus(FocusButton->TakeWidget());
	}
	SetInputMode(InputMode);
	SetShowMouseCursor(true);
}
