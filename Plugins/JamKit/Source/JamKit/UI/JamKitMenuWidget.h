#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"
#include "JamKitMenuWidget.generated.h"

class UButton;
class USoundBase;
class UWorld;

/**
 * Base for every kit menu screen (main, pause, options, credits).
 *
 * Gamepad and keyboard friendly: highlights whichever button or slider has focus (UMG buttons show no focus by
 * themselves) and moves focus to the one under the mouse. Every UButton and USlider in the layout is picked up
 * automatically; widgets created later (rows of a list) are added with AddFocusTarget.
 *
 * Screens open sub-screens on top of themselves (OptionsButton, CreditsButton): the opener collapses until the
 * sub-screen closes, so navigation can't wander into it. A screen opened that way closes on Back keys (Escape, B)
 * through Close(); override Close to save first.
 */
UCLASS(Abstract)
class JAMKIT_API UJamKitMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** The widget that should have focus when the screen opens. */
	virtual UWidget* GetDefaultFocusWidget() const PURE_VIRTUAL(UJamKitMenuWidget::GetDefaultFocusWidget, return nullptr;);

	/** Called each time the screen is shown as a sub-screen. Refresh displayed values here. */
	virtual void OnOpened() {}

	/** Set by the screen that opened this one; Close() calls it. */
	FSimpleDelegate OnClosed;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** Fades out and opens Level (see UJamKitFadeWidget). */
	void OpenLevel(const TSoftObjectPtr<UWorld>& Level) const;
	void QuitGame() const;

	/** Closes this sub-screen (returns to the screen that opened it). */
	virtual void Close();

	/** Shows a sub-screen on top, hiding this one until it closes. Instances are kept and reused. */
	void OpenSubScreen(TSubclassOf<UJamKitMenuWidget> ScreenClass, UButton* ReturnFocusTo);

	/** Adds a button or slider to the focus highlighting (and buttons to the click sound). */
	void AddFocusTarget(UWidget* Widget);

	/** Button background, or slider handle, while it has focus. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Style")
	FLinearColor FocusedColor = FLinearColor(0.95f, 0.65f, 0.15f);

	/** Played when focus moves from one widget to another (not when the screen opens). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundBase> FocusSound;

	/** Played when any button is clicked. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundBase> ClickSound;

	/** Keys that close this screen when it was opened as a sub-screen. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu")
	TArray<FKey> BackKeys = { EKeys::Escape, EKeys::Gamepad_FaceButton_Right };

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu")
	TSubclassOf<UJamKitMenuWidget> OptionsScreenClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu")
	TSubclassOf<UJamKitMenuWidget> CreditsScreenClass;

	/** Opens OptionsScreenClass, if the layout has the button. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> OptionsButton;

	/** Opens CreditsScreenClass, if the layout has the button. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CreditsButton;

private:
	UFUNCTION()
	void HandleAnyButtonClicked();

	UFUNCTION()
	void HandleOptionsClicked();

	UFUNCTION()
	void HandleCreditsClicked();

	void HandleSubScreenClosed();

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
	TMap<TSubclassOf<UJamKitMenuWidget>, TObjectPtr<UJamKitMenuWidget>> SubScreens;

	UPROPERTY()
	TObjectPtr<UJamKitMenuWidget> OpenScreen;

	TWeakObjectPtr<UButton> ReturnFocus;
	ESlateVisibility VisibilityBeforeSubScreen = ESlateVisibility::Visible;
};
