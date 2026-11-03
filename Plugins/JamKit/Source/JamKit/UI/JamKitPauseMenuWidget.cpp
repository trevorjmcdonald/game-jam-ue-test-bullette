#include "UI/JamKitPauseMenuWidget.h"

#include "Components/Button.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "Settings/JamKitSettings.h"

UWidget* UJamKitPauseMenuWidget::GetDefaultFocusWidget() const
{
	return ResumeButton;
}

void UJamKitPauseMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	ResumeButton->OnClicked.AddDynamic(this, &UJamKitPauseMenuWidget::HandleResumeClicked);
	MainMenuButton->OnClicked.AddDynamic(this, &UJamKitPauseMenuWidget::HandleMainMenuClicked);
	QuitButton->OnClicked.AddDynamic(this, &UJamKitPauseMenuWidget::HandleQuitClicked);
}

bool UJamKitPauseMenuWidget::IsPauseKey(const FKey& Key) const
{
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	return PauseAction && Subsystem && Subsystem->QueryKeysMappedToAction(PauseAction).Contains(Key);
}

FReply UJamKitPauseMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (ResumeKeys.Contains(InKeyEvent.GetKey()) || IsPauseKey(InKeyEvent.GetKey()))
	{
		OnResume.ExecuteIfBound();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UJamKitPauseMenuWidget::HandleResumeClicked()
{
	OnResume.ExecuteIfBound();
}

void UJamKitPauseMenuWidget::HandleMainMenuClicked()
{
	OpenLevel(GetDefault<UJamKitSettings>()->MainMenuLevel);
}

void UJamKitPauseMenuWidget::HandleQuitClicked()
{
	QuitGame();
}
