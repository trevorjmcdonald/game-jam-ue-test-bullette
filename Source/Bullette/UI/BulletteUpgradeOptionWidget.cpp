#include "UI/BulletteUpgradeOptionWidget.h"

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
	}
}

void UBulletteUpgradeOptionWidget::HandleHovered()
{
	SelectButton->SetKeyboardFocus();
}

void UBulletteUpgradeOptionWidget::SetUpgrade(const UBulletteUpgradeDefinition* InUpgrade, int32 TimesPicked)
{
	Upgrade = InUpgrade;

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
