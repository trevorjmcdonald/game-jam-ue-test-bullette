#include "UI/JamKitKeyBindingRowWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "UI/JamKitGamepadGlyphWidget.h"

void UJamKitKeyBindingRowWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	KeyboardButton->OnClicked.AddDynamic(this, &UJamKitKeyBindingRowWidget::HandleKeyboardClicked);
	GamepadButton->OnClicked.AddDynamic(this, &UJamKitKeyBindingRowWidget::HandleGamepadClicked);
}

void UJamKitKeyBindingRowWidget::Setup(const FJamKitBindingRowSpec& InSpec)
{
	Spec = InSpec;
	ActionText->SetText(Spec.DisplayName);
}

void UJamKitKeyBindingRowWidget::ShowKeys(const FText& KeyboardKey, const FText& GamepadKey, const FKey& GamepadKeyValue)
{
	// Hidden (not collapsed) so the columns stay lined up across rows.
	KeyboardText->SetText(KeyboardKey);
	KeyboardButton->SetVisibility(KeyboardKey.IsEmpty() ? ESlateVisibility::Hidden : ESlateVisibility::Visible);
	GamepadText->SetText(GamepadKey);
	GamepadButton->SetVisibility(GamepadKey.IsEmpty() ? ESlateVisibility::Hidden : ESlateVisibility::Visible);

	const bool bGlyph = GamepadGlyph && GamepadKeyValue.IsValid() && GamepadGlyph->SetKey(GamepadKeyValue);
	if (GamepadGlyph)
	{
		GamepadGlyph->SetVisibility(bGlyph ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	GamepadText->SetVisibility(bGlyph ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

void UJamKitKeyBindingRowWidget::ShowWaiting(bool bGamepad)
{
	UTextBlock* Text = bGamepad ? GamepadText : KeyboardText;
	Text->SetText(NSLOCTEXT("JamKit", "PressKey", "Press..."));
	Text->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (bGamepad && GamepadGlyph)
	{
		GamepadGlyph->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UJamKitKeyBindingRowWidget::HandleKeyboardClicked()
{
	OnRebindRequested.ExecuteIfBound(this, false);
}

void UJamKitKeyBindingRowWidget::HandleGamepadClicked()
{
	OnRebindRequested.ExecuteIfBound(this, true);
}
