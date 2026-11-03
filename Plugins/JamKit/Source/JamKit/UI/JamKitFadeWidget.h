#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "JamKitFadeWidget.generated.h"

class APlayerController;
class UWorld;

/**
 * Full-screen black fade, drawn in NativePaint (no layout asset). Uses real time, so it runs while the game is paused.
 * Use the statics: OpenLevelWithFade on the way out, FadeIn when a level starts (the kit's player controllers do).
 * The duration is UJamKitSettings::FadeSeconds; 0 skips the fade.
 * FadeIn first holds the screen black while PSO precaching (the shaders the level's materials need) is still running,
 * up to UJamKitSettings::MaxShaderWaitSeconds, with player input ignored and a progress bar if the wait shows.
 */
UCLASS()
class JAMKIT_API UJamKitFadeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Fades to black, then opens Level with URL Options, without the leading "?" (e.g. "Arrival=TruckStop", which a game mode can read from OptionsString to pick the player start
	 * tagged TruckStop). Ignored while a fade-out is already running. */
	static void OpenLevelWithFade(APlayerController* PlayerController, const TSoftObjectPtr<UWorld>& Level, const FString& Options = FString());

	/** Starts the screen black and fades it in. With bWaitForShaders, first waits for the level's shaders (see above):
	 * the game controller does, the main menu doesn't (its compiling carries on in the background while the player
	 * reads the menu, and the first startup can queue thousands of pipelines). */
	static void FadeIn(APlayerController* PlayerController, bool bWaitForShaders = false);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	static UJamKitFadeWidget* Start(APlayerController* PlayerController, float From, float To);
	float GetAlpha() const;
	/** True while still holding black for shaders; updates the progress bar. */
	bool TickShaderWait();
	void EndShaderWait();

	float FromAlpha = 0.0f;
	float ToAlpha = 1.0f;
	float Duration = 0.35f;
	double StartTime = 0.0;
	bool bFinished = false;
	TFunction<void()> OnFinished;

	bool bWaitingForShaders = false;
	double WaitStartTime = 0.0;
	double IdleSince = 0.0;
	float MaxWaitSeconds = 0.0f;
	uint32 PeakRequests = 0;
	float Progress = 0.0f;
};
