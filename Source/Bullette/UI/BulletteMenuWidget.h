#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BulletteMenuWidget.generated.h"

class UBulletteOptionsWidget;
class UButton;
class USoundBase;
class UWorld;

/**
 * Base for button menus (main menu, pause menu, options). Highlights whichever button or slider has focus and moves
 * focus to the one under the mouse, so the menu is usable with a gamepad, and provides the shared Open Level, Quit
 * and Options actions. Every UButton and USlider in the layout is picked up automatically; widgets created later
 * (rows of a list) are added with AddFocusTarget.
 */
UCLASS(Abstract)
class BULLETTE_API UBulletteMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** The widget that should have focus when the menu opens. */
	virtual UWidget* GetDefaultFocusWidget() const PURE_VIRTUAL(UBulletteMenuWidget::GetDefaultFocusWidget, return nullptr;);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void OpenLevel(const TSoftObjectPtr<UWorld>& Level) const;
	void QuitGame() const;

	/** Shows OptionsWidgetClass on top, hiding this menu until it closes. */
	void OpenOptions();

	/** Adds a button or slider to the focus highlighting (and buttons to the click sound). */
	void AddFocusTarget(UWidget* Widget);

	/** Button background, or slider handle, while it has focus. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Style")
	FLinearColor FocusedColor = FLinearColor(0.95f, 0.65f, 0.15f);

	/** Played when focus moves from one widget to another (not when the menu opens). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundBase> FocusSound;

	/** Played when any button is clicked. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundBase> ClickSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Options")
	TSubclassOf<UBulletteOptionsWidget> OptionsWidgetClass;

	/** Opens the options screen when clicked, if the layout has one. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> OptionsButton;

private:
	UFUNCTION()
	void HandleAnyButtonClicked();

	UFUNCTION()
	void HandleOptionsClicked();

	void HandleOptionsClosed();

	struct FFocusTarget
	{
		TWeakObjectPtr<UWidget> Widget;
		FLinearColor NormalColor;
		bool bShowingFocus = false;
	};
	TArray<FFocusTarget> FocusTargets;

	/** The target that had focus last tick, or null if none did. */
	TWeakObjectPtr<UWidget> LastFocused;

	UPROPERTY()
	TObjectPtr<UBulletteOptionsWidget> OptionsWidget;

	ESlateVisibility VisibilityBeforeOptions = ESlateVisibility::Visible;
};
