#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BulletteMenuWidget.generated.h"

class UButton;
class UWorld;

/**
 * Base for simple button menus (main menu, pause menu). Highlights whichever button has focus and moves focus to the
 * button under the mouse, so the menu is usable with a gamepad, and provides the shared Open Level and Quit actions.
 */
UCLASS(Abstract)
class BULLETTE_API UBulletteMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** The button that should have focus when the menu opens. */
	virtual UButton* GetDefaultFocusButton() const PURE_VIRTUAL(UBulletteMenuWidget::GetDefaultFocusButton, return nullptr;);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void OpenLevel(const TSoftObjectPtr<UWorld>& Level) const;
	void QuitGame() const;

	/** Button background while the button has focus. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Style")
	FLinearColor FocusedColor = FLinearColor(0.95f, 0.65f, 0.15f);

private:
	struct FMenuButton
	{
		TWeakObjectPtr<UButton> Button;
		FLinearColor NormalColor;
		bool bShowingFocus = false;
	};
	TArray<FMenuButton> Buttons;
};
