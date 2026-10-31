#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BulletteUpgradeOptionWidget.generated.h"

class UBulletteUpgradeDefinition;
class UButton;
class USoundBase;
class UTextBlock;

DECLARE_DELEGATE_OneParam(FBulletteUpgradeChosenDelegate, const UBulletteUpgradeDefinition* /*Upgrade*/);

/**
 * One clickable upgrade card. Layout lives in WBP_UpgradeOption; these widget names must match.
 * The card with focus is highlighted (UMG buttons show no focus by default, so gamepad players couldn't see which
 * card is selected), and hovering with the mouse moves focus to the card so mouse and gamepad agree.
 */
UCLASS(Abstract)
class BULLETTE_API UBulletteUpgradeOptionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** TimesPicked is how many times the player already has this upgrade. */
	void SetUpgrade(const UBulletteUpgradeDefinition* InUpgrade, int32 TimesPicked);

	UButton* GetSelectButton() const { return SelectButton; }

	FBulletteUpgradeChosenDelegate OnChosen;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Button background while this card has focus. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Style")
	FLinearColor FocusedColor = FLinearColor(0.95f, 0.65f, 0.15f);

	/** Played when this card gains focus, except for the focus it's given as the screen opens. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundBase> FocusSound;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SelectButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DescriptionText;

private:
	UFUNCTION()
	void HandleClicked();

	UFUNCTION()
	void HandleHovered();

	FLinearColor NormalColor = FLinearColor::White;
	bool bShowingFocus = false;

	/** Real time (the game is paused) when SetUpgrade last filled the card. */
	double ShownRealTime = 0.0;

	UPROPERTY()
	TObjectPtr<const UBulletteUpgradeDefinition> Upgrade;
};
