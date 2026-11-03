#include "UI/JamKitDamageNumbersWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/World.h"
#include "Feedback/JamKitFeedbackSubsystem.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

UJamKitDamageNumbersWidget::UJamKitDamageNumbersWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	EnemyDamageFont = FCoreStyle::GetDefaultFontStyle("Bold", 22);
	EnemyDamageFont.OutlineSettings.OutlineSize = 2;
	EnemyDamageFont.OutlineSettings.OutlineColor = FLinearColor::Black;

	PlayerDamageFont = FCoreStyle::GetDefaultFontStyle("Bold", 26);
	PlayerDamageFont.OutlineSettings.OutlineSize = 2;
	PlayerDamageFont.OutlineSettings.OutlineColor = FLinearColor::Black;

	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UJamKitDamageNumbersWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	// Repaints every frame anyway; this keeps it correct if Slate invalidation is ever turned on.
	ForceVolatile(true);
}

void UJamKitDamageNumbersWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (UJamKitFeedbackSubsystem* Feedback = UJamKitFeedbackSubsystem::Get(this))
	{
		Feedback->RemoveDamageNumbersBefore(GetWorld()->GetTimeSeconds() - Lifetime);
	}
}

int32 UJamKitDamageNumbersWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	LayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	const UJamKitFeedbackSubsystem* Feedback = UJamKitFeedbackSubsystem::Get(this);
	APlayerController* PC = GetOwningPlayer();
	if (!Feedback || !PC || Feedback->GetDamageNumbers().IsEmpty())
	{
		return LayerId;
	}

	const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const double Now = GetWorld()->GetTimeSeconds();
	const int32 TextLayer = LayerId + 1;

	for (const FJamKitDamageNumber& Number : Feedback->GetDamageNumbers())
	{
		const float Age = static_cast<float>(Now - Number.SpawnTime);
		if (Age < 0.0f || Age > Lifetime)
		{
			continue;
		}

		FVector2D ScreenPosition;
		if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, Number.Location, ScreenPosition, false))
		{
			continue;
		}

		const float T = Age / Lifetime;
		// Rise fast, then slow down; hold full opacity for the first 60%, then fade.
		const float Rise = RiseDistance * (1.0f - FMath::Square(1.0f - T));
		const float Opacity = T < 0.6f ? 1.0f : 1.0f - (T - 0.6f) / 0.4f;
		const float Scale = Age < PopDuration ? FMath::Lerp(PopScale, 1.0f, Age / PopDuration) : 1.0f;

		FSlateFontInfo Font = Number.bPlayerDamage ? PlayerDamageFont : EnemyDamageFont;
		Font.Size *= Scale;
		Font.OutlineSettings.OutlineColor.A *= Opacity;
		FLinearColor Color = Number.bPlayerDamage ? PlayerDamageColor : EnemyDamageColor;
		Color.A *= Opacity;

		const FString Text = FString::FromInt(FMath::Max(1, FMath::RoundToInt(Number.Amount)));
		const FVector2f Size(FontMeasure->Measure(Text, Font));
		const FVector2f Center = FVector2f(ScreenPosition) + Number.Jitter - FVector2f(0.0f, Rise);

		FSlateDrawElement::MakeText(OutDrawElements, TextLayer,
			AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(Center - Size * 0.5f)),
			Text, Font, ESlateDrawEffect::None, Color);
	}

	return TextLayer;
}
