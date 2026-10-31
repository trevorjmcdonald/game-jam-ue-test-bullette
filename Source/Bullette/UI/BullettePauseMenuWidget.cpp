#include "UI/BullettePauseMenuWidget.h"

#include "Components/Button.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"

UWidget* UBullettePauseMenuWidget::GetDefaultFocusWidget() const
{
	return ResumeButton;
}

void UBullettePauseMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	ResumeButton->OnClicked.AddDynamic(this, &UBullettePauseMenuWidget::HandleResumeClicked);
	MainMenuButton->OnClicked.AddDynamic(this, &UBullettePauseMenuWidget::HandleMainMenuClicked);
	QuitButton->OnClicked.AddDynamic(this, &UBullettePauseMenuWidget::HandleQuitClicked);
}

bool UBullettePauseMenuWidget::IsPauseKey(const FKey& Key) const
{
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	return PauseAction && Subsystem && Subsystem->QueryKeysMappedToAction(PauseAction).Contains(Key);
}

FReply UBullettePauseMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (ResumeKeys.Contains(InKeyEvent.GetKey()) || IsPauseKey(InKeyEvent.GetKey()))
	{
		OnResume.ExecuteIfBound();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UBullettePauseMenuWidget::HandleResumeClicked()
{
	OnResume.ExecuteIfBound();
}

void UBullettePauseMenuWidget::HandleMainMenuClicked()
{
	OpenLevel(MainMenuLevel);
}

void UBullettePauseMenuWidget::HandleQuitClicked()
{
	QuitGame();
}
