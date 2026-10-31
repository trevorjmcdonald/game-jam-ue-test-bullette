#include "UI/BulletteMainMenuWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Game/BulletteSettingsSubsystem.h"

UWidget* UBulletteMainMenuWidget::GetDefaultFocusWidget() const
{
	return PlayButton;
}

void UBulletteMainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	PlayButton->OnClicked.AddDynamic(this, &UBulletteMainMenuWidget::HandlePlayClicked);
	QuitButton->OnClicked.AddDynamic(this, &UBulletteMainMenuWidget::HandleQuitClicked);
}

void UBulletteMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!BestRunText)
	{
		return;
	}
	const UBulletteSettingsSubsystem* Settings = UBulletteSettingsSubsystem::Get(this);
	if (!Settings || !Settings->HasBestRun())
	{
		BestRunText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const int32 Seconds = FMath::FloorToInt(Settings->GetBestSurvivedSeconds());
	FNumberFormattingOptions TwoDigits;
	TwoDigits.MinimumIntegralDigits = 2;
	BestRunText->SetText(FText::Format(NSLOCTEXT("Bullette", "BestRun", "Best run {0}:{1} - {2} kills"),
		FText::AsNumber(Seconds / 60), FText::AsNumber(Seconds % 60, &TwoDigits), FText::AsNumber(Settings->GetBestKills())));
	BestRunText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UBulletteMainMenuWidget::HandlePlayClicked()
{
	OpenLevel(GameLevel);
}

void UBulletteMainMenuWidget::HandleQuitClicked()
{
	QuitGame();
}
