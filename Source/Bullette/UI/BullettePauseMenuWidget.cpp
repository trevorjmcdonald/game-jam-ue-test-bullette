#include "UI/BullettePauseMenuWidget.h"

#include "Components/Button.h"

void UBullettePauseMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	ResumeButton->OnClicked.AddDynamic(this, &UBullettePauseMenuWidget::HandleResumeClicked);
	MainMenuButton->OnClicked.AddDynamic(this, &UBullettePauseMenuWidget::HandleMainMenuClicked);
	QuitButton->OnClicked.AddDynamic(this, &UBullettePauseMenuWidget::HandleQuitClicked);
}

FReply UBullettePauseMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (ResumeKeys.Contains(InKeyEvent.GetKey()))
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
