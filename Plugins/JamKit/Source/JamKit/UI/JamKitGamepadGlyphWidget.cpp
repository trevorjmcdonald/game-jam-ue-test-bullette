#include "UI/JamKitGamepadGlyphWidget.h"

#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Settings/JamKitDeckSettingsSubsystem.h"

#define LOCTEXT_NAMESPACE "JamKitGlyph"

namespace
{
	struct FGlyph
	{
		FText Label;
		/** Letter colour (Xbox face buttons); white otherwise. */
		FLinearColor Ink = FLinearColor(0.92f, 0.92f, 0.92f);
		/** Single letters sit in a circle; longer labels in a pill. */
		bool bCircle = false;
	};

	bool FindGlyph(const FKey& Key, bool bDeck, FGlyph& Out)
	{
		// Xbox face button colours; the Deck's buttons are plain.
		const FLinearColor Green(0.42f, 0.78f, 0.25f), Red(0.93f, 0.27f, 0.22f), Blue(0.25f, 0.56f, 0.95f), Yellow(0.98f, 0.78f, 0.1f);
		auto Face = [&](const TCHAR* Letter, const FLinearColor& Colour)
		{
			Out.Label = FText::FromString(Letter);
			Out.bCircle = true;
			if (!bDeck)
			{
				Out.Ink = Colour;
			}
		};
		auto Pill = [&](const FText& Xbox, const FText& Deck)
		{
			Out.Label = bDeck ? Deck : Xbox;
		};

		if (Key == EKeys::Gamepad_FaceButton_Bottom) { Face(TEXT("A"), Green); }
		else if (Key == EKeys::Gamepad_FaceButton_Right) { Face(TEXT("B"), Red); }
		else if (Key == EKeys::Gamepad_FaceButton_Left) { Face(TEXT("X"), Blue); }
		else if (Key == EKeys::Gamepad_FaceButton_Top) { Face(TEXT("Y"), Yellow); }
		else if (Key == EKeys::Gamepad_LeftShoulder) { Pill(LOCTEXT("LB", "LB"), LOCTEXT("L1", "L1")); }
		else if (Key == EKeys::Gamepad_RightShoulder) { Pill(LOCTEXT("RB", "RB"), LOCTEXT("R1", "R1")); }
		else if (Key == EKeys::Gamepad_LeftTrigger || Key == EKeys::Gamepad_LeftTriggerAxis) { Pill(LOCTEXT("LT", "LT"), LOCTEXT("L2", "L2")); }
		else if (Key == EKeys::Gamepad_RightTrigger || Key == EKeys::Gamepad_RightTriggerAxis) { Pill(LOCTEXT("RT", "RT"), LOCTEXT("R2", "R2")); }
		else if (Key == EKeys::Gamepad_LeftThumbstick) { Pill(LOCTEXT("LSPress", "L3"), LOCTEXT("L3", "L3")); }
		else if (Key == EKeys::Gamepad_RightThumbstick) { Pill(LOCTEXT("RSPress", "R3"), LOCTEXT("R3", "R3")); }
		else if (Key == EKeys::Gamepad_Left2D || Key == EKeys::Gamepad_LeftX || Key == EKeys::Gamepad_LeftY) { Pill(LOCTEXT("LStick", "L Stick"), LOCTEXT("LStickDeck", "L Stick")); }
		else if (Key == EKeys::Gamepad_Right2D || Key == EKeys::Gamepad_RightX || Key == EKeys::Gamepad_RightY) { Pill(LOCTEXT("RStick", "R Stick"), LOCTEXT("RStickDeck", "R Stick")); }
		else if (Key == EKeys::Gamepad_Special_Right) { Pill(LOCTEXT("Menu", "Menu"), LOCTEXT("MenuDeck", "Menu")); }
		else if (Key == EKeys::Gamepad_Special_Left) { Pill(LOCTEXT("View", "View"), LOCTEXT("ViewDeck", "View")); }
		else if (Key == EKeys::Gamepad_DPad_Up) { Pill(LOCTEXT("DUp", "D-Pad Up"), LOCTEXT("DUpDeck", "D-Pad Up")); }
		else if (Key == EKeys::Gamepad_DPad_Down) { Pill(LOCTEXT("DDown", "D-Pad Down"), LOCTEXT("DDownDeck", "D-Pad Down")); }
		else if (Key == EKeys::Gamepad_DPad_Left) { Pill(LOCTEXT("DLeft", "D-Pad Left"), LOCTEXT("DLeftDeck", "D-Pad Left")); }
		else if (Key == EKeys::Gamepad_DPad_Right) { Pill(LOCTEXT("DRight", "D-Pad Right"), LOCTEXT("DRightDeck", "D-Pad Right")); }
		else { return false; }
		return true;
	}
}

bool UJamKitGamepadGlyphWidget::HasGlyph(const FKey& Key)
{
	FGlyph Unused;
	return FindGlyph(Key, false, Unused);
}

bool UJamKitGamepadGlyphWidget::SetKey(const FKey& Key)
{
	FGlyph Glyph;
	if (!FindGlyph(Key, UJamKitDeckSettingsSubsystem::IsRunningOnSteamDeck(), Glyph))
	{
		return false;
	}
	const float Radius = Size * 0.5f;
	const FLinearColor Fill(0.06f, 0.06f, 0.07f, 0.95f);
	const FLinearColor Rim(0.55f, 0.55f, 0.58f, 1.0f);
	Shape->SetBrush(FSlateRoundedBoxBrush(Fill, Radius, Rim, 1.5f, FVector2f(Size, Size)));
	Shape->SetPadding(Glyph.bCircle ? FMargin(0.0f) : FMargin(Size * 0.35f, 0.0f));
	Shape->SetHorizontalAlignment(HAlign_Center);
	Shape->SetVerticalAlignment(VAlign_Center);
	Label->SetText(Glyph.Label);
	Label->SetColorAndOpacity(FSlateColor(Glyph.Ink));
	return true;
}

#undef LOCTEXT_NAMESPACE
