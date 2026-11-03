#include "Verify/JamKitProbeSubsystem.h"

#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "JamKit.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "RenderCore.h"
#include "RHI.h"
#include "Settings/JamKitSettings.h"
#include "UnrealClient.h"
#include "Verify/JamKitProbePoint.h"
#include "Verify/JamKitTrace.h"
#include "Verify/JamKitVerify.h"

namespace JamKitProbeImpl
{
	constexpr double StartDelay = 1.0;
	constexpr float DefaultPerfSeconds = 5.0f;
	/** The most one frame counts towards a settle time. */
	constexpr float MaxSettleStep = 0.1f;

	bool IsRequested()
	{
		FString Value;
		return JamKitVerify::HasParam(TEXT("jamprobe")) || JamKitVerify::GetValue(TEXT("jamprobe="), Value);
	}

	bool Contains(const TArray<FString>& List, const FString& Item)
	{
		return List.ContainsByPredicate([&Item](const FString& Each) { return Each.Equals(Item, ESearchCase::IgnoreCase); });
	}
}

bool UJamKitProbeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return !GIsEditor && !IsRunningCommandlet() && JamKitProbeImpl::IsRequested() && Super::ShouldCreateSubsystem(Outer);
}

TArray<FString> UJamKitProbeSubsystem::ParseList(const TCHAR* Key)
{
	TArray<FString> List;
	FString Value;
	if (JamKitVerify::GetValue(Key, Value))
	{
		Value.ParseIntoArray(List, TEXT("+"));
	}
	return List;
}

void UJamKitProbeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Collection.InitializeDependency<UJamKitTraceSubsystem>();
	Super::Initialize(Collection);
	const UJamKitSettings* Settings = GetDefault<UJamKitSettings>();

	const TArray<FString> LevelFilter = ParseList(TEXT("jamprobe="));
	TArray<FJamKitProbeLevel> Configured = Settings->ProbeLevels;
	if (Configured.IsEmpty())
	{
		FJamKitProbeLevel Game;
		Game.Level = Settings->GameLevel;
		Configured.Add(Game);
	}
	for (const FJamKitProbeLevel& Each : Configured)
	{
		if (Each.Level.IsNull() || (!LevelFilter.IsEmpty() && !JamKitProbeImpl::Contains(LevelFilter, Each.GetLabel())))
		{
			continue;
		}
		Levels.Add({Each.Level.ToSoftObjectPath().GetLongPackageName(), Each.GetLabel(), Each.Options, Each.Commands});
	}

	const TArray<FString> MomentFilter = ParseList(TEXT("jamprobemoments="));
	for (const FJamKitProbeMoment& Each : Settings->ProbeMoments)
	{
		if (MomentFilter.IsEmpty() || JamKitProbeImpl::Contains(MomentFilter, Each.Name.ToString()))
		{
			Moments.Add({Each.Name, Each.Commands});
		}
	}
	if (Moments.IsEmpty())
	{
		Moments.Add({TEXT("Start"), {}});
	}

	PointFilter = ParseList(TEXT("jamprobepoints="));
	JamKitVerify::GetValue(TEXT("jamprobetag="), Tag);
	bShots = !JamKitVerify::HasParam(TEXT("jamprobenoshots"));
	FString Perf;
	if (JamKitVerify::GetValue(TEXT("jamprobeperf="), Perf))
	{
		PerfSeconds = FCString::Atof(*Perf);
	}
	else if (JamKitVerify::HasParam(TEXT("jamprobeperf")))
	{
		PerfSeconds = JamKitProbeImpl::DefaultPerfSeconds;
	}

	OutputDir = FPaths::ProjectSavedDir() / TEXT("Probe") / Tag;
	IFileManager::Get().DeleteDirectory(*OutputDir, false, true);
	IFileManager::Get().MakeDirectory(*OutputDir, true);

	FJamKitTraceFields Run;
	Run.Add(TEXT("tag"), Tag).Add(TEXT("cpu"), FPlatformMisc::GetCPUBrand().TrimStartAndEnd())
		.Add(TEXT("threads"), FPlatformMisc::NumberOfCoresIncludingHyperthreads()).Add(TEXT("workers"), FPlatformMisc::NumberOfWorkerThreadsToSpawn())
		.Add(TEXT("gpu"), GRHIAdapterName).Add(TEXT("perfSeconds"), PerfSeconds).Add(TEXT("cmd"), JamKitVerify::Args());
	WriteSummary(TEXT("{\"k\":\"run\"") + Run.Json + TEXT("}"));
	UE_LOG(LogJamKit, Display, TEXT("Probe: %d level(s), %d moment(s), tag %s%s"), Levels.Num(), Moments.Num(), *Tag,
		PerfSeconds > 0.0f ? *FString::Printf(TEXT(", timing %.0f s per view"), PerfSeconds) : TEXT(""));

	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UJamKitProbeSubsystem::Tick));
	// The picture has to be read while the frame is drawn (before it's presented), so it's the engine's screenshot
	// path: bound to this delegate it hands over the pixels instead of saving a file.
	ScreenshotHandle = UGameViewportClient::OnScreenshotCaptured().AddUObject(this, &UJamKitProbeSubsystem::HandleScreenshot);
}

void UJamKitProbeSubsystem::Deinitialize()
{
	FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
	UGameViewportClient::OnScreenshotCaptured().Remove(ScreenshotHandle);
	Super::Deinitialize();
}

UWorld* UJamKitProbeSubsystem::GetGameWorld() const
{
	UWorld* World = GetGameInstance()->GetWorld();
	return World && World->HasBegunPlay() && !World->bIsTearingDown ? World : nullptr;
}

void UJamKitProbeSubsystem::WriteSummary(const FString& Line)
{
	FFileHelper::SaveStringToFile(Line + TEXT("\n"), *(OutputDir / TEXT("summary.jsonl")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
		&IFileManager::Get(), FILEWRITE_Append);
}

bool UJamKitProbeSubsystem::Tick(float DeltaTime)
{
	UWorld* World = GetGameWorld();
	switch (State)
	{
	case EState::Starting:
		if (World && (Timer += DeltaTime) >= JamKitProbeImpl::StartDelay)
		{
			OpenNextLevel();
		}
		break;

	case EState::WaitingForLevel:
		if (World && World->GetFirstPlayerController()
			&& UWorld::RemovePIEPrefix(World->GetMapName()) == FPackageName::GetShortName(Levels[LevelIndex].Path)
			&& (Timer += FMath::Min(DeltaTime, JamKitProbeImpl::MaxSettleStep)) >= GetDefault<UJamKitSettings>()->ProbeLevelSettleSeconds)
		{
			BeginLevel(World);
		}
		break;

	case EState::Settling:
		// A hitch (a shader, the new view's first frames) doesn't count as settling time.
		if ((Timer += FMath::Min(DeltaTime, JamKitProbeImpl::MaxSettleStep)) >= Wait)
		{
			if (PerfSeconds > 0.0f)
			{
				State = EState::Measuring;
				PerfElapsed = SumGame = SumRender = SumGPU = 0.0;
				FrameMs.Reset();
			}
			else
			{
				RequestCapture();
			}
		}
		break;

	case EState::Measuring:
		PerfElapsed += DeltaTime;
		FrameMs.Add(DeltaTime * 1000.0f);
		SumGame += FPlatformTime::ToMilliseconds(GGameThreadTime);
		SumRender += FPlatformTime::ToMilliseconds(GRenderThreadTime);
		SumGPU += FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());
		if (PerfElapsed >= PerfSeconds)
		{
			RequestCapture();
		}
		break;

	case EState::Capturing:
		// Normally answered by the next frame drawn.
		if (bCaptured)
		{
			FinishShot(MoveTemp(CapturedPixels), CapturedSize);
		}
		else if ((Timer += DeltaTime) > 5.0)
		{
			UE_LOG(LogJamKit, Warning, TEXT("Probe: no picture came back"));
			FinishShot({}, FIntPoint::ZeroValue);
		}
		break;

	case EState::Done:
		break;
	}
	return true;
}

void UJamKitProbeSubsystem::OpenNextLevel()
{
	if (++LevelIndex >= Levels.Num())
	{
		Finish();
		return;
	}
	UWorld* World = GetGameInstance()->GetWorld();
	const FLevelJob& Level = Levels[LevelIndex];
	for (const FString& Command : Level.Commands)
	{
		GEngine->Exec(World, *Command);
	}
	UE_LOG(LogJamKit, Display, TEXT("Probe: opening %s (%s)"), *Level.Label, *Level.Path);
	UGameplayStatics::OpenLevel(World, FName(*Level.Path), true, Level.Options);
	State = EState::WaitingForLevel;
	Timer = 0.0;
	CurrentMoment = INDEX_NONE;
}

void UJamKitProbeSubsystem::BeginLevel(UWorld* World)
{
	APlayerController* Controller = World->GetFirstPlayerController();
	if (Controller->PlayerCameraManager)
	{
		PlayerFOV = Controller->PlayerCameraManager->GetFOVAngle();
	}
	// Out of the pictures: the player and whatever they carry.
	if (APawn* Pawn = Controller->GetPawn())
	{
		TArray<AActor*> Carried;
		Pawn->GetAttachedActors(Carried, true, true);
		Carried.Add(Pawn);
		for (AActor* Each : Carried)
		{
			Each->SetActorHiddenInGame(true);
		}
		Pawn->DisableInput(Controller);
	}
	if (PerfSeconds > 0.0f)
	{
		// Measure the real cost, not the frame cap.
		GEngine->Exec(World, TEXT("t.MaxFPS 0"));
		GEngine->Exec(World, TEXT("r.VSync 0"));
	}

	TArray<AJamKitProbePoint*> Points;
	for (TActorIterator<AJamKitProbePoint> It(World); It; ++It)
	{
		if (PointFilter.IsEmpty() || JamKitProbeImpl::Contains(PointFilter, It->GetProbeName().ToString()))
		{
			Points.Add(*It);
		}
	}
	Points.Sort([](const AJamKitProbePoint& A, const AJamKitProbePoint& B) { return A.GetProbeName().LexicalLess(B.GetProbeName()); });

	Shots.Reset();
	for (int32 Moment = 0; Moment < Moments.Num(); ++Moment)
	{
		for (AJamKitProbePoint* Point : Points)
		{
			if (Point->Moments.IsEmpty() || Point->Moments.Contains(Moments[Moment].Name))
			{
				Shots.Add({Moment, Point});
			}
		}
	}
	UE_LOG(LogJamKit, Display, TEXT("Probe: %s has %d point(s), %d shot(s)"), *Levels[LevelIndex].Label, Points.Num(), Shots.Num());
	ShotIndex = INDEX_NONE;
	NextShot();
}

void UJamKitProbeSubsystem::NextShot()
{
	UWorld* World = GetGameWorld();
	if (!World || ++ShotIndex >= Shots.Num())
	{
		OpenNextLevel();
		return;
	}
	const UJamKitSettings* Settings = GetDefault<UJamKitSettings>();
	const FShot& Shot = Shots[ShotIndex];
	Wait = Settings->ProbeViewSettleSeconds;
	if (Shot.Moment != CurrentMoment)
	{
		CurrentMoment = Shot.Moment;
		for (const FString& Command : Moments[CurrentMoment].Commands)
		{
			GEngine->Exec(World, *Command);
		}
		Wait = FMath::Max(Wait, static_cast<double>(Settings->ProbeMomentSettleSeconds));
	}
	if (AJamKitProbePoint* Point = Shot.Point.Get())
	{
		if (Point->bMatchPlayerFOV)
		{
			Point->GetCameraComponent()->SetFieldOfView(PlayerFOV);
		}
		World->GetFirstPlayerController()->SetViewTarget(Point);
	}
	State = EState::Settling;
	Timer = 0.0;
}

void UJamKitProbeSubsystem::RequestCapture()
{
	State = EState::Capturing;
	Timer = 0.0;
	bCaptured = false;
	FScreenshotRequest::RequestScreenshot(/*bInShowUI*/ false);
}

void UJamKitProbeSubsystem::HandleScreenshot(int32 Width, int32 Height, const TArray<FColor>& Colors)
{
	// Mid-draw: keep the pixels and carry on from the next tick.
	if (State == EState::Capturing && !bCaptured)
	{
		CapturedPixels = Colors;
		CapturedSize = FIntPoint(Width, Height);
		bCaptured = true;
	}
}

void UJamKitProbeSubsystem::FinishShot(TArray<FColor> Pixels, FIntPoint Size)
{
	const FShot& Shot = Shots[ShotIndex];
	const AJamKitProbePoint* Point = Shot.Point.Get();
	const FString PointName = Point ? Point->GetProbeName().ToString() : TEXT("(gone)");
	const FString MomentName = Moments[Shot.Moment].Name.ToString();
	const FString Label = Levels[LevelIndex].Label;
	const FString RelativeFile = Label / FString::Printf(TEXT("%s_%s.png"), *PointName, *MomentName);

	FString StatsJson;
	FString LogLine;
	JamKitVerify::AnalyseImage(Pixels, Size, StatsJson, LogLine);
	if (bShots)
	{
		JamKitVerify::SavePng(Pixels, Size, OutputDir / RelativeFile);
	}

	FJamKitTraceFields Perf;
	if (PerfSeconds > 0.0f && FrameMs.Num() > 0)
	{
		const int32 N = FrameMs.Num();
		TArray<float> Sorted = FrameMs;
		Sorted.Sort();
		const double Average = PerfElapsed * 1000.0 / N;
		Perf.Add(TEXT("fps"), N / PerfElapsed).Add(TEXT("frameMs"), Average).Add(TEXT("p95Ms"), Sorted[FMath::Min(N - 1, N * 95 / 100)])
			.Add(TEXT("worstMs"), Sorted.Last()).Add(TEXT("gameMs"), SumGame / N).Add(TEXT("renderMs"), SumRender / N).Add(TEXT("gpuMs"), SumGPU / N);
		LogLine += FString::Printf(TEXT(" | %5.1f fps, frame %5.1f ms (p95 %5.1f) game %5.1f render %5.1f GPU %5.1f"),
			N / PerfElapsed, Average, Sorted[FMath::Min(N - 1, N * 95 / 100)], SumGame / N, SumRender / N, SumGPU / N);
	}

	FJamKitTraceFields Line;
	Line.Add(TEXT("lvl"), Label).Add(TEXT("point"), PointName).Add(TEXT("moment"), MomentName);
	if (bShots)
	{
		Line.Add(TEXT("file"), RelativeFile);
	}
	WriteSummary(TEXT("{\"k\":\"shot\"") + Line.Json + StatsJson + Perf.Json + TEXT("}"));
	UE_LOG(LogJamKit, Display, TEXT("Probe: %-14s %-14s %-10s %s"), *Label, *PointName, *MomentName, *LogLine);
	JamKitTrace::Event(GetGameWorld(), TEXT("probe"), PointName, FJamKitTraceFields().Add(TEXT("moment"), MomentName));
	++ShotCount;
	NextShot();
}

void UJamKitProbeSubsystem::Finish()
{
	State = EState::Done;
	WriteSummary(FString::Printf(TEXT("{\"k\":\"done\",\"shots\":%d}"), ShotCount));
	UE_LOG(LogJamKit, Display, TEXT("Probe: done, %d shot(s) in %s"), ShotCount, *FPaths::ConvertRelativePathToFull(OutputDir));
	FPlatformMisc::RequestExit(false, TEXT("JamKitProbe"));
}
