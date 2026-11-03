#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"
#include "JamKitGamepadGlyphWidget.generated.h"

class UBorder;
class UTextBlock;

/**
 * A controller button drawn as a glyph instead of its name: A/B/X/Y in coloured circles (Xbox style), LB/RB/LT/RT,
 * L3/R3, the sticks, Menu/View and the d-pad as labelled pills. On a Steam Deck (UJamKitDeckSettingsSubsystem) it uses
 * the Deck's own labels (L1/R1/L2/R2) in its monochrome style. No textures: Shape is a Border given a rounded brush
 * at runtime, Label its text. Layout lives in /JamKit/UI/WBP_GamepadGlyph.
 */
UCLASS(Abstract)
class JAMKIT_API UJamKitGamepadGlyphWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** False if Key isn't a gamepad key this can draw (the caller shows its name instead). */
	bool SetKey(const FKey& Key);

	/** Whether Key has a glyph. */
	static bool HasGlyph(const FKey& Key);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> Shape;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Label;

	/** Glyph height in slate units; circles are this wide, pills grow with their label. */
	UPROPERTY(EditAnywhere, Category = "Glyph")
	float Size = 34.0f;
};
