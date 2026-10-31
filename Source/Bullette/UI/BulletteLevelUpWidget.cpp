#include "UI/BulletteLevelUpWidget.h"

#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBoxSlot.h"

void UBulletteLevelUpWidget::ShowChoices(int32 NewLevel, TConstArrayView<FBulletteUpgradeChoice> Choices)
{
	TitleText->SetText(FText::Format(NSLOCTEXT("Bullette", "LevelUpTitle", "Level {0}! Choose an upgrade"), FText::AsNumber(NewLevel)));

	OptionList->ClearChildren();
	if (!ensureMsgf(OptionWidgetClass, TEXT("%s has no OptionWidgetClass assigned"), *GetName()))
	{
		return;
	}

	for (const FBulletteUpgradeChoice& Choice : Choices)
	{
		UBulletteUpgradeOptionWidget* Option = CreateWidget<UBulletteUpgradeOptionWidget>(this, OptionWidgetClass);
		Option->SetUpgrade(Choice.Upgrade, Choice.TimesPicked);
		Option->OnChosen.BindUObject(this, &UBulletteLevelUpWidget::HandleOptionChosen);
		if (UVerticalBoxSlot* OptionSlot = Cast<UVerticalBoxSlot>(OptionList->AddChild(Option)))
		{
			OptionSlot->SetPadding(FMargin(0.0f, 6.0f));
		}
	}
}

UWidget* UBulletteLevelUpWidget::GetFirstOptionButton() const
{
	const UBulletteUpgradeOptionWidget* First = OptionList->GetChildrenCount() > 0 ? Cast<UBulletteUpgradeOptionWidget>(OptionList->GetChildAt(0)) : nullptr;
	return First ? First->GetSelectButton() : nullptr;
}

void UBulletteLevelUpWidget::HandleOptionChosen(const UBulletteUpgradeDefinition* Upgrade)
{
	OnUpgradeChosen.ExecuteIfBound(Upgrade);
}
