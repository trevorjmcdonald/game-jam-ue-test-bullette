#pragma once

#include "CoreMinimal.h"
#include "Settings/JamKitSettingsSubsystem.h"
#include "UI/JamKitMenuWidget.h"
#include "UI/JamKitKeyBindingRowWidget.h"
#include "JamKitOptionsWidget.generated.h"

class UEnhancedInputUserSettings;
class UInputMappingContext;
class USlider;
class UTextBlock;
class UVerticalBox;

/**
 * Volume sliders and key rebinding, opened from the main menu and the pause menu (UJamKitMenuWidget::OptionsButton).
 * Layout lives in /JamKit/UI/WBP_Options; these widget names must match. Binding rows are RowClass instances, one per
 * entry of Bindings; leave Bindings empty to build them from UJamKitSettings::RebindContext: every player-mappable
 * mapping, grouped into rows by display name, keyboard keys in one column and gamepad keys in the other (a 2D stick
 * mapping cycles between the two sticks).
 *
 * Rebinding goes through Enhanced Input's player-mappable key settings (bEnableUserSettings in DefaultInput.ini),
 * which save themselves to their own slot. Volumes live in UJamKitSettingsSubsystem and are saved on close.
 * Clicking a key starts a capture: the next key of the right kind (keyboard or gamepad) is bound; Escape or a few
 * seconds without input cancel it. A key already used by another row of the same kind is swapped with it.
 */
UCLASS(Abstract)
class JAMKIT_API UJamKitOptionsWidget : public UJamKitMenuWidget
{
	GENERATED_BODY()

public:
	UJamKitOptionsWidget();

	virtual UWidget* GetDefaultFocusWidget() const override;

	/** Refreshes values from the current settings. */
	virtual void OnOpened() override;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** Saves the volumes before returning. */
	virtual void Close() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Controls")
	TSubclassOf<UJamKitKeyBindingRowWidget> RowClass;

	/** Explicit rows. Empty: built from the rebind context. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Controls")
	TArray<FJamKitBindingRowSpec> Bindings;

	/** Adds an "Invert look up/down" row (mouse and controller toggles) after the first row whose name starts with LookRowName ("Look", "Look (Stick)"), or first. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Controls")
	bool bInvertLookRow = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Controls")
	FText LookRowName = NSLOCTEXT("JamKitOptions", "LookRow", "Look");

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

	void HandleRebindRequested(UJamKitKeyBindingRowWidget* Row, bool bGamepad);

	/** One row per display name of the context's player-mappable mappings, in mapping order. */
	static TArray<FJamKitBindingRowSpec> BuildRows(const UInputMappingContext* Context);

	void SetVolume(EJamKitVolume Volume, float Value, UTextBlock* ValueText, bool bPreview);
	UEnhancedInputUserSettings* GetInputSettings() const;
	FKey GetBoundKey(FName MappingName) const;
	/** Binds Key to MappingName, swapping it with any other row (of the same kind) that already uses it. */
	void BindKey(FName MappingName, const FKey& Key, bool bGamepad);
	void RefreshRows();
	void EndCapture();

	UPROPERTY()
	TArray<TObjectPtr<UJamKitKeyBindingRowWidget>> Rows;

	TWeakObjectPtr<UJamKitKeyBindingRowWidget> CaptureRow;
	bool bCaptureGamepad = false;
	double CaptureStartTime = 0.0;
	double LastPreviewTime = 0.0;
};
