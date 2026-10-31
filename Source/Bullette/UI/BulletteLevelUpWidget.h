#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/BulletteUpgradeOptionWidget.h"
#include "BulletteLevelUpWidget.generated.h"

class UPanelWidget;
class UTextBlock;

struct FBulletteUpgradeChoice
{
	const UBulletteUpgradeDefinition* Upgrade = nullptr;
	int32 TimesPicked = 0;
};

/** The "pick one" screen shown on level-up. Layout lives in WBP_LevelUp; these widget names must match. */
UCLASS(Abstract)
class BULLETTE_API UBulletteLevelUpWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowChoices(int32 NewLevel, TConstArrayView<FBulletteUpgradeChoice> Choices);

	/** The first option's button, for keyboard and gamepad focus. */
	UWidget* GetFirstOptionButton() const;

	FBulletteUpgradeChosenDelegate OnUpgradeChosen;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TitleText;

	/** Option cards are created into this panel. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> OptionList;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Level Up")
	TSubclassOf<UBulletteUpgradeOptionWidget> OptionWidgetClass;

private:
	void HandleOptionChosen(const UBulletteUpgradeDefinition* Upgrade);
};
