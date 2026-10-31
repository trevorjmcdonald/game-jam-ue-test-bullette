#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"
#include "BulletteKeyBindingRowWidget.generated.h"

class UButton;
class UTextBlock;
class UBulletteKeyBindingRowWidget;

/** One rebindable action in the options screen. Mapping names are the player-mappable names set on IMC_Default. */
USTRUCT(BlueprintType)
struct FBulletteBindingRowSpec
{
	GENERATED_BODY()

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

DECLARE_DELEGATE_TwoParams(FBulletteRebindRequested, UBulletteKeyBindingRowWidget* /*Row*/, bool /*bGamepad*/);

/** A row of the controls list: action name, keyboard key, gamepad key. Layout lives in WBP_KeyBindingRow. */
UCLASS(Abstract)
class BULLETTE_API UBulletteKeyBindingRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(const FBulletteBindingRowSpec& InSpec);
	const FBulletteBindingRowSpec& GetSpec() const { return Spec; }

	/** Empty text hides that column's button. */
	void ShowKeys(const FText& KeyboardKey, const FText& GamepadKey);
	void ShowWaiting(bool bGamepad);

	UButton* GetKeyboardButton() const { return KeyboardButton; }
	UButton* GetGamepadButton() const { return GamepadButton; }

	FBulletteRebindRequested OnRebindRequested;

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

private:
	UFUNCTION()
	void HandleKeyboardClicked();

	UFUNCTION()
	void HandleGamepadClicked();

	FBulletteBindingRowSpec Spec;
};
