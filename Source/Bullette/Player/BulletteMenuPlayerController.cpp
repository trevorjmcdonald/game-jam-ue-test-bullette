#include "Player/BulletteMenuPlayerController.h"

#include "Audio/BulletteAudio.h"
#include "Components/AudioComponent.h"
#include "Game/BulletteSettingsSubsystem.h"
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
	if (UWidget* Focus = MenuWidget->GetDefaultFocusWidget())
	{
		InputMode.SetWidgetToFocus(Focus->TakeWidget());
	}
	SetInputMode(InputMode);
	SetShowMouseCursor(true);

	if (const UBulletteSettingsSubsystem* Settings = UBulletteSettingsSubsystem::Get(this))
	{
		Settings->ApplyVolumes();
	}
	MusicComponent = BulletteAudio::PlayMusic(this, MenuMusic);
}
