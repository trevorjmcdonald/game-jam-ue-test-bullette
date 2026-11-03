#include "Player/JamKitMenuPlayerController.h"

#include "Audio/JamKitAudio.h"
#include "Components/AudioComponent.h"
#include "JamKit.h"
#include "Settings/JamKitSettingsSubsystem.h"
#include "UI/JamKitFadeWidget.h"
#include "UI/JamKitMenuWidget.h"

void AJamKitMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	if (const UJamKitSettingsSubsystem* Settings = UJamKitSettingsSubsystem::Get(this))
	{
		Settings->ApplyVolumes();
	}
	MusicComponent = JamKitAudio::PlayMusic(this, MenuMusic);

	if (!MenuWidgetClass)
	{
		UE_LOG(LogJamKit, Warning, TEXT("%s has no MenuWidgetClass"), *GetName());
		return;
	}
	MenuWidget = CreateWidget<UJamKitMenuWidget>(this, MenuWidgetClass);
	MenuWidget->AddToViewport();

	FInputModeUIOnly InputMode;
	if (UWidget* Focus = MenuWidget->GetDefaultFocusWidget())
	{
		InputMode.SetWidgetToFocus(Focus->TakeWidget());
	}
	SetInputMode(InputMode);
	SetShowMouseCursor(true);

	UJamKitFadeWidget::FadeIn(this);
}
