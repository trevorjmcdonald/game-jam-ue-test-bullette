#include "UI/JamKitFadeWidget.h"

#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "JamKit.h"
#include "Kismet/GameplayStatics.h"
#include "PipelineStateCache.h"
#include "Rendering/DrawElements.h"
#include "Settings/JamKitSettings.h"
#include "Styling/CoreStyle.h"

namespace
{
	/** Draws above every other widget. */
	constexpr int32 FadeZOrder = 10000;
	/** The shader wait ends once no precache request has run for this long (requests arrive in bursts as components register). */
	constexpr double ShaderIdleSeconds = 0.25;
	/** The progress bar only appears if the wait lasts longer than this, so quick loads just fade in. */
	constexpr double ShowBarAfterSeconds = 0.3;
	TWeakObjectPtr<UJamKitFadeWidget> ActiveFadeOut;
}

UJamKitFadeWidget* UJamKitFadeWidget::Start(APlayerController* PlayerController, float From, float To)
{
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return nullptr;
	}
	UJamKitFadeWidget* Fade = CreateWidget<UJamKitFadeWidget>(PlayerController, UJamKitFadeWidget::StaticClass());
	Fade->FromAlpha = From;
	Fade->ToAlpha = To;
	Fade->Duration = GetDefault<UJamKitSettings>()->FadeSeconds;
	Fade->StartTime = FPlatformTime::Seconds();
	Fade->SetVisibility(ESlateVisibility::HitTestInvisible);
	Fade->AddToViewport(FadeZOrder);
	return Fade;
}

void UJamKitFadeWidget::OpenLevelWithFade(APlayerController* PlayerController, const TSoftObjectPtr<UWorld>& Level, const FString& Options)
{
	if (Level.IsNull())
	{
		UE_LOG(LogJamKit, Warning, TEXT("OpenLevelWithFade: no level set (Project Settings > Jam Kit)"));
		return;
	}
	if (ActiveFadeOut.IsValid())
	{
		return;
	}

	UWorld* World = PlayerController ? PlayerController->GetWorld() : nullptr;
	const float Seconds = GetDefault<UJamKitSettings>()->FadeSeconds;
	UJamKitFadeWidget* Fade = Seconds > 0.0f ? Start(PlayerController, 0.0f, 1.0f) : nullptr;
	if (!Fade)
	{
		UGameplayStatics::OpenLevelBySoftObjectPtr(World, Level, true, Options);
		return;
	}

	ActiveFadeOut = Fade;
	TWeakObjectPtr<UWorld> WeakWorld = World;
	Fade->OnFinished = [WeakWorld, Level, Options]()
	{
		if (UWorld* LiveWorld = WeakWorld.Get())
		{
			UGameplayStatics::OpenLevelBySoftObjectPtr(LiveWorld, Level, true, Options);
		}
	};
}

void UJamKitFadeWidget::FadeIn(APlayerController* PlayerController, bool bWaitForShaders)
{
	const UJamKitSettings* Settings = GetDefault<UJamKitSettings>();
	if (Settings->FadeSeconds <= 0.0f)
	{
		return;
	}
	UJamKitFadeWidget* Fade = Start(PlayerController, 1.0f, 0.0f);
	if (Fade && bWaitForShaders && Settings->MaxShaderWaitSeconds > 0.0f)
	{
		Fade->bWaitingForShaders = true;
		Fade->MaxWaitSeconds = Settings->MaxShaderWaitSeconds;
		Fade->WaitStartTime = Fade->IdleSince = FPlatformTime::Seconds();
		PlayerController->SetIgnoreMoveInput(true);
		PlayerController->SetIgnoreLookInput(true);
	}
}

bool UJamKitFadeWidget::TickShaderWait()
{
	const double Now = FPlatformTime::Seconds();
	const uint32 Active = PipelineStateCache::NumActivePrecacheRequests();
	PeakRequests = FMath::Max(PeakRequests, Active);
	if (Active > 0)
	{
		IdleSince = Now;
	}
	// More requests can arrive after the bar has moved, so it never goes backwards.
	if (PeakRequests > 0)
	{
		Progress = FMath::Max(Progress, 1.0f - static_cast<float>(Active) / static_cast<float>(PeakRequests));
	}
	const bool bIdle = Now - IdleSince >= ShaderIdleSeconds;
	const bool bTimedOut = Now - WaitStartTime >= MaxWaitSeconds;
	if (bTimedOut && !bIdle)
	{
		UE_LOG(LogJamKit, Warning, TEXT("FadeIn: gave up waiting for shaders after %.1f s (%u still compiling)"), Now - WaitStartTime, Active);
	}
	return !bIdle && !bTimedOut;
}

void UJamKitFadeWidget::EndShaderWait()
{
	bWaitingForShaders = false;
	StartTime = FPlatformTime::Seconds();
	UE_LOG(LogJamKit, Log, TEXT("FadeIn: waited %.2f s for shaders (peak %u compiling)"), StartTime - WaitStartTime, PeakRequests);
	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		PlayerController->SetIgnoreMoveInput(false);
		PlayerController->SetIgnoreLookInput(false);
	}
}

float UJamKitFadeWidget::GetAlpha() const
{
	if (bWaitingForShaders)
	{
		return FromAlpha;
	}
	const float T = Duration > 0.0f ? FMath::Clamp(static_cast<float>((FPlatformTime::Seconds() - StartTime) / Duration), 0.0f, 1.0f) : 1.0f;
	return FMath::Lerp(FromAlpha, ToAlpha, T);
}

void UJamKitFadeWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bWaitingForShaders)
	{
		if (TickShaderWait())
		{
			return;
		}
		EndShaderWait();
	}

	if (bFinished || FPlatformTime::Seconds() - StartTime < Duration)
	{
		return;
	}
	bFinished = true;
	if (OnFinished)
	{
		// Opening a level tears this widget down; stay black until it does.
		OnFinished();
	}
	else if (ToAlpha <= 0.0f)
	{
		RemoveFromParent();
	}
}

int32 UJamKitFadeWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	LayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	const float Alpha = GetAlpha();
	if (Alpha > 0.0f)
	{
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(),
			FCoreStyle::Get().GetBrush("GenericWhiteBox"), ESlateDrawEffect::None, FLinearColor(0.0f, 0.0f, 0.0f, Alpha));
	}

	// The shader wait's progress bar: a thin line low in the middle of the screen.
	if (bWaitingForShaders && FPlatformTime::Seconds() - WaitStartTime >= ShowBarAfterSeconds)
	{
		const FVector2f Screen = AllottedGeometry.GetLocalSize();
		const FVector2f BarSize(Screen.X * 0.3f, FMath::Max(2.0f, Screen.Y * 0.004f));
		const FVector2f BarPos((Screen.X - BarSize.X) * 0.5f, Screen.Y * 0.82f);
		const FSlateBrush* White = FCoreStyle::Get().GetBrush("GenericWhiteBox");
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(BarSize, FSlateLayoutTransform(BarPos)),
			White, ESlateDrawEffect::None, FLinearColor(1.0f, 1.0f, 1.0f, 0.12f));
		if (Progress > 0.0f)
		{
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 3,
				AllottedGeometry.ToPaintGeometry(FVector2f(BarSize.X * Progress, BarSize.Y), FSlateLayoutTransform(BarPos)),
				White, ESlateDrawEffect::None, FLinearColor(1.0f, 1.0f, 1.0f, 0.7f));
		}
		return LayerId + 3;
	}
	return LayerId + 1;
}
