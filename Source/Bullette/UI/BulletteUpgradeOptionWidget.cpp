#include "UI/BulletteUpgradeOptionWidget.h"

#include "Audio/BulletteAudio.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Progression/BulletteUpgradeDefinition.h"

void UBulletteUpgradeOptionWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	SelectButton->OnClicked.AddDynamic(this, &UBulletteUpgradeOptionWidget::HandleClicked);
	SelectButton->OnHovered.AddDynamic(this, &UBulletteUpgradeOptionWidget::HandleHovered);
	NormalColor = SelectButton->GetBackgroundColor();
}

void UBulletteUpgradeOptionWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const bool bFocused = SelectButton->HasAnyUserFocus();
	if (bFocused != bShowingFocus)
	{
		bShowingFocus = bFocused;
		SelectButton->SetBackgroundColor(bFocused ? FocusedColor : NormalColor);

		// The first card is focused as the screen opens, alongside the level-up sound; stay quiet for that one.
		if (bFocused && GetWorld()->GetRealTimeSeconds() - ShownRealTime > 0.15)
		{
			BulletteAudio::PlaySound(this, FocusSound, true, 0.0f);
		}
	}
}

void UBulletteUpgradeOptionWidget::HandleHovered()
{
	SelectButton->SetKeyboardFocus();
}

void UBulletteUpgradeOptionWidget::SetUpgrade(const UBulletteUpgradeDefinition* InUpgrade, int32 TimesPicked)
{
	Upgrade = InUpgrade;
	ShownRealTime = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0;

	const FText Name = TimesPicked > 0
		? FText::Format(NSLOCTEXT("Bullette", "UpgradeNameWithRank", "{0} ({1})"), Upgrade->DisplayName, FText::AsNumber(TimesPicked + 1))
		: Upgrade->DisplayName;
	NameText->SetText(Name);
	DescriptionText->SetText(TimesPicked > 0 && !Upgrade->LevelUpDescription.IsEmpty() ? Upgrade->LevelUpDescription : Upgrade->Description);
}

void UBulletteUpgradeOptionWidget::HandleClicked()
{
	OnChosen.ExecuteIfBound(Upgrade);
}
