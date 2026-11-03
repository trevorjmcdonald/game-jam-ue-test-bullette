#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "JamKitProbeSubsystem.generated.h"

class AJamKitProbePoint;

/**
 * Photographs (and optionally times) the game unattended: launch with -jamprobe and it opens each of the Jam Kit
 * settings' ProbeLevels in turn, runs each ProbeMoment's console commands (time of day, ...), looks through every
 * AJamKitProbePoint in the level, waits for the light to settle and saves what the renderer drew (no UI) to
 * Saved/Probe/<Tag>/<Level>/<Point>_<Moment>.png, then quits. Each shot also gets a line in
 * Saved/Probe/<Tag>/summary.jsonl with how bright it came out (mean, percentiles, share near black and blown out,
 * a 3x3 grid of means), so "too dark to judge" is a number.
 *
 * Command line (lists separated by +):
 *   -jamprobe                every level;  -jamprobe=L_PostOffice+L_Pryor  only those (by label)
 *   -jamprobemoments=Noon+Dusk   -jamprobepoints=Bench+Line   filters
 *   -jamprobeperf[=Seconds]  also time each view (frame, game, render, GPU ms) for that long (default 5)
 *   -jamprobetag=Name        output folder (default "run"), e.g. one per hardware profile
 *   -jamprobenoshots         timings only
 * The arguments can also come from Saved/jamkit-args.txt (JamKitVerify::Args), e.g. on a Steam Deck.
 * Use a fixed window size (-windowed -ResX=1280 -ResY=720) so shots and timings compare between runs.
 */
UCLASS()
class JAMKIT_API UJamKitProbeSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	enum class EState : uint8
	{
		Starting,
		WaitingForLevel,
		Settling,
		Measuring,
		Capturing,
		Done,
	};

	struct FLevelJob
	{
		FString Path;
		FString Label;
		FString Options;
		TArray<FString> Commands;
	};

	struct FMoment
	{
		FName Name;
		TArray<FString> Commands;
	};

	struct FShot
	{
		int32 Moment = INDEX_NONE;
		TWeakObjectPtr<AJamKitProbePoint> Point;
	};

	bool Tick(float DeltaTime);
	void OpenNextLevel();
	void BeginLevel(UWorld* World);
	void NextShot();
	void RequestCapture();
	void HandleScreenshot(int32 Width, int32 Height, const TArray<FColor>& Colors);
	void FinishShot(TArray<FColor> Pixels, FIntPoint Size);
	void WriteSummary(const FString& Line);
	void Finish();
	UWorld* GetGameWorld() const;
	static TArray<FString> ParseList(const TCHAR* Key);

	TArray<FLevelJob> Levels;
	TArray<FMoment> Moments;
	TArray<FString> PointFilter;
	TArray<FShot> Shots;
	FString Tag = TEXT("run");
	FString OutputDir;
	FTSTicker::FDelegateHandle TickerHandle;
	FDelegateHandle ScreenshotHandle;
	TArray<FColor> CapturedPixels;
	FIntPoint CapturedSize = FIntPoint::ZeroValue;
	bool bCaptured = false;
	EState State = EState::Starting;
	int32 LevelIndex = INDEX_NONE;
	int32 ShotIndex = INDEX_NONE;
	int32 CurrentMoment = INDEX_NONE;
	int32 ShotCount = 0;
	double Timer = 0.0;
	double Wait = 0.0;
	float PlayerFOV = 90.0f;
	bool bShots = true;
	float PerfSeconds = 0.0f;

	// Timing the current view.
	double PerfElapsed = 0.0;
	TArray<float> FrameMs;
	double SumGame = 0.0;
	double SumRender = 0.0;
	double SumGPU = 0.0;
};
