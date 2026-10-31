#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BulletteDamageNumbersWidget.generated.h"

/**
 * Full-screen, click-through layer that paints the damage numbers queued in UBulletteFeedbackSubsystem.
 * Needs no layout asset: it draws text directly in NativePaint, so one widget covers every number.
 * Numbers pop in, rise and fade over Lifetime (game time, so they freeze while paused).
 */
UCLASS()
class BULLETTE_API UBulletteDamageNumbersWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UBulletteDamageNumbersWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	UPROPERTY(EditDefaultsOnly, Category = "Damage Numbers", meta = (ClampMin = "0.1", Units = "s"))
	float Lifetime = 0.7f;

	/** How far a number rises over its lifetime, in screen units. */
	UPROPERTY(EditDefaultsOnly, Category = "Damage Numbers")
	float RiseDistance = 50.0f;

	/** Numbers start this much bigger and shrink to normal size over PopDuration. */
	UPROPERTY(EditDefaultsOnly, Category = "Damage Numbers", meta = (ClampMin = "1"))
	float PopScale = 1.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage Numbers", meta = (ClampMin = "0.01", Units = "s"))
	float PopDuration = 0.08f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage Numbers")
	FSlateFontInfo EnemyDamageFont;

	UPROPERTY(EditDefaultsOnly, Category = "Damage Numbers")
	FLinearColor EnemyDamageColor = FLinearColor(1.0f, 0.95f, 0.75f);

	UPROPERTY(EditDefaultsOnly, Category = "Damage Numbers")
	FSlateFontInfo PlayerDamageFont;

	UPROPERTY(EditDefaultsOnly, Category = "Damage Numbers")
	FLinearColor PlayerDamageColor = FLinearColor(1.0f, 0.2f, 0.15f);
};
