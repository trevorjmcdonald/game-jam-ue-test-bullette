#include "UI/BulletteGameOverWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

void UBulletteGameOverWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	RestartButton->OnClicked.AddDynamic(this, &UBulletteGameOverWidget::HandleRestartClicked);
}

void UBulletteGameOverWidget::ShowResults(const FText& Stats, const FText& Weapons)
{
	StatsText->SetText(Stats);
	WeaponsText->SetText(Weapons);
}

void UBulletteGameOverWidget::HandleRestartClicked()
{
	OnRestart.ExecuteIfBound();
}
