#pragma once

#include "CoreMinimal.h"
#include "Game/BulletteSettingsSubsystem.h"
#include "UI/BulletteMenuWidget.h"
#include "UI/BulletteKeyBindingRowWidget.h"
#include "BulletteOptionsWidget.generated.h"

class UEnhancedInputUserSettings;
class UInputMappingContext;
class USlider;
class UTextBlock;
class UVerticalBox;

/**
 * Volume sliders and key rebinding, opened from the main menu and the pause menu (UBulletteMenuWidget::OptionsButton).
 * Layout lives in WBP_Options; these widget names must match. Binding rows are WBP_KeyBindingRow instances made from
 * Bindings when the screen is created.
 *
 * Rebinding goes through Enhanced Input's player-mappable key settings (bEnableUserSettings in DefaultInput.ini),
 * which save themselves to their own slot. Volumes live in UBulletteSettingsSubsystem and are saved on Back.
 * Clicking a key starts a capture: the next key of the right kind (keyboard or gamepad) is bound; Escape or a few
 * seconds without input cancel it. A key already used by another row of the same kind is swapped with it.
 */
UCLASS(Abstract)
class BULLETTE_API UBulletteOptionsWidget : public UBulletteMenuWidget
{
	GENERATED_BODY()

public:
	UBulletteOptionsWidget();

	virtual UWidget* GetDefaultFocusWidget() const override;

	/** Refreshes values from the current settings. Call each time the screen is shown. */
	void RefreshFromSettings();

	/** Back was chosen; the opener removes the screen. */
	FSimpleDelegate OnClosed;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** Registered with the player's input settings so its mappings can be listed even where it isn't applied
	 * (the main menu). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Controls")
	TObjectPtr<UInputMappingContext> BindingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Controls")
	TSubclassOf<UBulletteKeyBindingRowWidget> RowClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Controls")
	TArray<FBulletteBindingRowSpec> Bindings;

	/** Keys that close the screen (when not waiting for a key to bind). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Controls")
	TArray<FKey> BackKeys = { EKeys::Escape, EKeys::Gamepad_FaceButton_Right };

	/** A rebind capture gives up after this long without a usable key. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Controls", meta = (ClampMin = "1", Units = "s"))
	float CaptureTimeout = 6.0f;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> MasterSlider;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MasterValueText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> MusicSlider;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MusicValueText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> EffectsSlider;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> EffectsValueText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> BindingList;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> HintText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ResetButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BackButton;

private:
	UFUNCTION()
	void HandleMasterChanged(float Value);

	UFUNCTION()
	void HandleMusicChanged(float Value);

	UFUNCTION()
	void HandleEffectsChanged(float Value);

	UFUNCTION()
	void HandleResetClicked();

	UFUNCTION()
	void HandleBackClicked();

	void HandleRebindRequested(UBulletteKeyBindingRowWidget* Row, bool bGamepad);

	void SetVolume(EBulletteVolume Volume, float Value, UTextBlock* ValueText, bool bPreview);
	UEnhancedInputUserSettings* GetInputSettings() const;
	FKey GetBoundKey(FName MappingName) const;
	/** Binds Key to MappingName, swapping it with any other row (of the same kind) that already uses it. */
	void BindKey(FName MappingName, const FKey& Key, bool bGamepad);
	void RefreshRows();
	void EndCapture();

	UPROPERTY()
	TArray<TObjectPtr<UBulletteKeyBindingRowWidget>> Rows;

	TWeakObjectPtr<UBulletteKeyBindingRowWidget> CaptureRow;
	bool bCaptureGamepad = false;
	double CaptureStartTime = 0.0;
	double LastPreviewTime = 0.0;
};
