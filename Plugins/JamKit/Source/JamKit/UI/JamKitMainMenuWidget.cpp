#include "UI/JamKitMainMenuWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Settings/JamKitSettings.h"

UWidget* UJamKitMainMenuWidget::GetDefaultFocusWidget() const
{
	return PlayButton;
}

void UJamKitMainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	PlayButton->OnClicked.AddDynamic(this, &UJamKitMainMenuWidget::HandlePlayClicked);
	QuitButton->OnClicked.AddDynamic(this, &UJamKitMainMenuWidget::HandleQuitClicked);

	const FText& Title = GetDefault<UJamKitSettings>()->GameTitle;
	if (TitleText && !Title.IsEmpty())
	{
		TitleText->SetText(Title);
	}
}

void UJamKitMainMenuWidget::HandlePlayClicked()
{
	OpenLevel(GetDefault<UJamKitSettings>()->GameLevel);
}

void UJamKitMainMenuWidget::HandleQuitClicked()
{
	QuitGame();
}
