#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"
#include "JamKitKeyBindingRowWidget.generated.h"

class UButton;
class UJamKitGamepadGlyphWidget;
class UTextBlock;
class UJamKitKeyBindingRowWidget;

UENUM(BlueprintType)
enum class EJamKitOptionRow : uint8
{
	/** A rebindable action. */
	Binding,
	/** "Invert look up/down": the keyboard column toggles the mouse, the gamepad column the controller. */
	InvertLook,
};

/** One rebindable action in the options screen. Mapping names are the player-mappable key settings names on the
 * rebind context's mappings (UJamKitSettings::RebindContext). */
USTRUCT(BlueprintType)
struct FJamKitBindingRowSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Binding")
	EJamKitOptionRow Kind = EJamKitOptionRow::Binding;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Binding")
	FText DisplayName;

	/** None: no keyboard binding in this row. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Binding")
	FName KeyboardMapping;

	/** None: no gamepad binding in this row. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Binding")
	FName GamepadMapping;

	/** If set, the gamepad button cycles through these keys instead of waiting for a press (for sticks, which
	 * can't be "pressed" to capture them). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Binding")
	TArray<FKey> GamepadChoices;
};

DECLARE_DELEGATE_TwoParams(FJamKitRebindRequested, UJamKitKeyBindingRowWidget* /*Row*/, bool /*bGamepad*/);

/** A row of the controls list: action name, keyboard key, gamepad key. Layout lives in /JamKit/UI/WBP_KeyBindingRow. */
UCLASS(Abstract)
class JAMKIT_API UJamKitKeyBindingRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(const FJamKitBindingRowSpec& InSpec);
	const FJamKitBindingRowSpec& GetSpec() const { return Spec; }

	/** Empty text hides that column's button. A valid GamepadKeyValue with a glyph is drawn as the glyph instead
	 * of GamepadKey's text (when the layout has a GamepadGlyph). */
	void ShowKeys(const FText& KeyboardKey, const FText& GamepadKey, const FKey& GamepadKeyValue = FKey());
	void ShowWaiting(bool bGamepad);

	UButton* GetKeyboardButton() const { return KeyboardButton; }
	UButton* GetGamepadButton() const { return GamepadButton; }

	FJamKitRebindRequested OnRebindRequested;

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ActionText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> KeyboardButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> KeyboardText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> GamepadButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> GamepadText;

	/** Optional: the gamepad key drawn as a controller glyph (next to GamepadText, which it replaces). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UJamKitGamepadGlyphWidget> GamepadGlyph;

private:
	UFUNCTION()
	void HandleKeyboardClicked();

	UFUNCTION()
	void HandleGamepadClicked();

	FJamKitBindingRowSpec Spec;
};
