#include "UI/BulletteKeyBindingRowWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

void UBulletteKeyBindingRowWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	KeyboardButton->OnClicked.AddDynamic(this, &UBulletteKeyBindingRowWidget::HandleKeyboardClicked);
	GamepadButton->OnClicked.AddDynamic(this, &UBulletteKeyBindingRowWidget::HandleGamepadClicked);
}

void UBulletteKeyBindingRowWidget::Setup(const FBulletteBindingRowSpec& InSpec)
{
	Spec = InSpec;
	ActionText->SetText(Spec.DisplayName);
}

void UBulletteKeyBindingRowWidget::ShowKeys(const FText& KeyboardKey, const FText& GamepadKey)
{
	// Hidden (not collapsed) so the columns stay lined up across rows.
	KeyboardText->SetText(KeyboardKey);
	KeyboardButton->SetVisibility(KeyboardKey.IsEmpty() ? ESlateVisibility::Hidden : ESlateVisibility::Visible);
	GamepadText->SetText(GamepadKey);
	GamepadButton->SetVisibility(GamepadKey.IsEmpty() ? ESlateVisibility::Hidden : ESlateVisibility::Visible);
}

void UBulletteKeyBindingRowWidget::ShowWaiting(bool bGamepad)
{
	(bGamepad ? GamepadText : KeyboardText)->SetText(NSLOCTEXT("Bullette", "PressKey", "Press..."));
}

void UBulletteKeyBindingRowWidget::HandleKeyboardClicked()
{
	OnRebindRequested.ExecuteIfBound(this, false);
}

void UBulletteKeyBindingRowWidget::HandleGamepadClicked()
{
	OnRebindRequested.ExecuteIfBound(this, true);
}
