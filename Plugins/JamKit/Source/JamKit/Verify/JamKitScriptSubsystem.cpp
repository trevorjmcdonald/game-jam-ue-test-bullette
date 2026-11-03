#include "Verify/JamKitScriptSubsystem.h"

#include "Components/SceneComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "JamKit.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/OutputDeviceNull.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Random/JamKitRandom.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Settings/JamKitSettings.h"
#include "UnrealClient.h"
#include "Verify/JamKitChoice.h"
#include "Verify/JamKitTrace.h"
#include "Verify/JamKitVerify.h"

namespace JamKitScriptImpl
{
	constexpr double StartDelay = 1.0;
	constexpr double OpenTimeout = 90.0;
	constexpr double ShotTimeout = 5.0;
	constexpr float MaxSettleStep = 0.1f;
	constexpr int32 MaxStepsPerTick = 200;

	const TCHAR* const Ops[] = {TEXT("open"), TEXT("exec"), TEXT("teleport"), TEXT("look"), TEXT("lookAt"), TEXT("press"), TEXT("hold"),
		TEXT("moveTo"), TEXT("wait"), TEXT("set"), TEXT("call"), TEXT("choose"), TEXT("expect"), TEXT("waitFor"), TEXT("repeat"),
		TEXT("shot"), TEXT("note")};
	/** Steps that change something: an "expect" looks at what happened since the last of these began. */
	const TCHAR* const Actions[] = {TEXT("open"), TEXT("exec"), TEXT("teleport"), TEXT("look"), TEXT("lookAt"), TEXT("press"), TEXT("hold"),
		TEXT("moveTo"), TEXT("set"), TEXT("call"), TEXT("choose")};

	FString OpOf(const FJsonObject& Step)
	{
		for (const TCHAR* Op : Ops)
		{
			if (Step.HasField(Op))
			{
				return Op;
			}
		}
		return FString();
	}

	bool IsAction(const FString& Op)
	{
		for (const TCHAR* Each : Actions)
		{
			if (Op == Each)
			{
				return true;
			}
		}
		return false;
	}

	/** A field as text: strings as they are, numbers and booleans written out, arrays joined with commas. */
	FString Text(const FJsonObject& Step, const TCHAR* Field)
	{
		const TSharedPtr<FJsonValue> Value = Step.TryGetField(Field);
		if (!Value.IsValid())
		{
			return FString();
		}
		switch (Value->Type)
		{
		case EJson::String: return Value->AsString();
		case EJson::Boolean: return Value->AsBool() ? TEXT("True") : TEXT("False");
		case EJson::Number: return FString::SanitizeFloat(Value->AsNumber());
		case EJson::Array:
			return FString::JoinBy(Value->AsArray(), TEXT(","), [](const TSharedPtr<FJsonValue>& Each)
			{
				return Each->Type == EJson::Number ? FString::SanitizeFloat(Each->AsNumber()) : Each->Type == EJson::String ? Each->AsString() : FString(TEXT("..."));
			});
		case EJson::Object: return TEXT("{...}");
		default: return FString();
		}
	}

	/** A number or [x, y, z] (missing parts 0); Default if the field isn't there. */
	FVector Vector(const FJsonObject& Step, const TCHAR* Field, const FVector& Default)
	{
		const TSharedPtr<FJsonValue> Value = Step.TryGetField(Field);
		if (!Value.IsValid())
		{
			return Default;
		}
		if (Value->Type == EJson::Number)
		{
			return FVector(Value->AsNumber(), 0.0, 0.0);
		}
		if (Value->Type == EJson::Boolean)
		{
			return FVector(Value->AsBool() ? 1.0 : 0.0, 0.0, 0.0);
		}
		FVector Out = FVector::ZeroVector;
		if (Value->Type == EJson::Array)
		{
			const TArray<TSharedPtr<FJsonValue>>& Parts = Value->AsArray();
			for (int32 Index = 0; Index < FMath::Min(Parts.Num(), 3); ++Index)
			{
				Out[Index] = Parts[Index]->AsNumber();
			}
		}
		return Out;
	}

	double Number(const FJsonObject& Step, const TCHAR* Field, double Default)
	{
		double Value = Default;
		Step.TryGetNumberField(Field, Value);
		return Value;
	}

	bool Flag(const FJsonObject& Step, const TCHAR* Field)
	{
		bool Value = false;
		Step.TryGetBoolField(Field, Value);
		return Value;
	}

	/** The steps in an array field ({"repeat": [...]}, a handler's "do"). */
	TArray<TSharedPtr<FJsonObject>> StepsOf(const FJsonObject& Object, const TCHAR* Field)
	{
		TArray<TSharedPtr<FJsonObject>> Steps;
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (Object.TryGetArrayField(Field, Values))
		{
			for (const TSharedPtr<FJsonValue>& Value : *Values)
			{
				if (const TSharedPtr<FJsonObject>* Step = nullptr; Value->TryGetObject(Step))
				{
					Steps.Add(*Step);
				}
			}
		}
		return Steps;
	}
}

bool UJamKitScriptSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	FString Path;
	return !GIsEditor && !IsRunningCommandlet() && JamKitVerify::GetValue(TEXT("jamscript="), Path) && Super::ShouldCreateSubsystem(Outer);
}

void UJamKitScriptSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	using namespace JamKitScriptImpl;
	UJamKitTraceSubsystem* Trace = Collection.InitializeDependency<UJamKitTraceSubsystem>();
	Collection.InitializeDependency<UJamKitRandomSubsystem>();
	Collection.InitializeDependency<UJamKitChoiceSubsystem>();
	Super::Initialize(Collection);
	StartTime = FPlatformTime::Seconds();

	FString Path;
	JamKitVerify::GetValue(TEXT("jamscript="), Path);
	if (FPaths::IsRelative(Path))
	{
		Path = FPaths::ProjectDir() / Path;
	}
	Name = FPaths::GetBaseFilename(Path);

	FString Text;
	TSharedPtr<FJsonObject> Root;
	FSteps Main;
	if (!FFileHelper::LoadFileToString(Text, *Path) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid())
	{
		UE_LOG(LogJamKit, Error, TEXT("Script: can't read %s"), *Path);
	}
	else
	{
		Root->TryGetStringField(TEXT("name"), Name);
		Root->TryGetStringField(TEXT("report"), ReportKind);
		Root->TryGetNumberField(TEXT("timeout"), Timeout);
		// The script's own seed makes it repeatable without arguments; -jamseed (e.g. play-script.ps1 -Seeds) wins.
		FString CommandLineSeed;
		double ScriptSeed = 0.0;
		if (Root->TryGetNumberField(TEXT("seed"), ScriptSeed) && !JamKitVerify::GetValue(TEXT("jamseed="), CommandLineSeed))
		{
			if (UJamKitRandomSubsystem* SeedFrom = Collection.InitializeDependency<UJamKitRandomSubsystem>())
			{
				SeedFrom->SetSeed(static_cast<int32>(ScriptSeed), true);
			}
		}
		Main = StepsOf(*Root, TEXT("steps"));
		const TArray<TSharedPtr<FJsonValue>>* On = nullptr;
		if (Root->TryGetArrayField(TEXT("on"), On))
		{
			for (const TSharedPtr<FJsonValue>& Value : *On)
			{
				if (const TSharedPtr<FJsonObject>* Handler = nullptr; Value->TryGetObject(Handler))
				{
					FHandler& Added = Handlers.AddDefaulted_GetRef();
					Added.Kind = FName(*JamKitScriptImpl::Text(**Handler, TEXT("event")));
					Added.Contains = JamKitScriptImpl::Text(**Handler, TEXT("contains"));
					Added.Steps = StepsOf(**Handler, TEXT("do"));
				}
			}
		}
	}

	OutputDir = FPaths::ProjectSavedDir() / TEXT("Script") / Name;
	IFileManager::Get().DeleteDirectory(*OutputDir, false, true);
	IFileManager::Get().MakeDirectory(*OutputDir, true);
	const UJamKitRandomSubsystem* Random = Collection.InitializeDependency<UJamKitRandomSubsystem>();
	WriteResult(FString::Printf(TEXT("{\"k\":\"run\",\"name\":\"%s\",\"steps\":%d,\"handlers\":%d,\"seed\":%d,\"file\":\"%s\"}"),
		*JamKitTrace::Escape(Name), Main.Num(), Handlers.Num(), Random ? Random->GetSeed() : 0, *JamKitTrace::Escape(Path)));
	UE_LOG(LogJamKit, Display, TEXT("Script: %s, %d step(s), %d handler(s)"), *Name, Main.Num(), Handlers.Num());

	FFrame& First = Stack.AddDefaulted_GetRef();
	First.Steps = MoveTemp(Main);

	if (Trace)
	{
		TraceHandle = Trace->OnLine.AddUObject(this, &UJamKitScriptSubsystem::HandleTraceLine);
	}
	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UJamKitScriptSubsystem::Tick));
	ScreenshotHandle = UGameViewportClient::OnScreenshotCaptured().AddUObject(this, &UJamKitScriptSubsystem::HandleScreenshot);
}

void UJamKitScriptSubsystem::Deinitialize()
{
	FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
	UGameViewportClient::OnScreenshotCaptured().Remove(ScreenshotHandle);
	if (UJamKitTraceSubsystem* Trace = GetGameInstance()->GetSubsystem<UJamKitTraceSubsystem>())
	{
		Trace->OnLine.Remove(TraceHandle);
	}
	Super::Deinitialize();
}

UWorld* UJamKitScriptSubsystem::GetGameWorld() const
{
	UWorld* World = GetGameInstance()->GetWorld();
	return World && World->HasBegunPlay() && !World->bIsTearingDown ? World : nullptr;
}

APlayerController* UJamKitScriptSubsystem::GetPlayer() const
{
	const UWorld* World = GetGameWorld();
	return World ? World->GetFirstPlayerController() : nullptr;
}

void UJamKitScriptSubsystem::WriteResult(const FString& Line) const
{
	FFileHelper::SaveStringToFile(Line + TEXT("\n"), *(OutputDir / TEXT("result.jsonl")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
		&IFileManager::Get(), FILEWRITE_Append);
}

void UJamKitScriptSubsystem::HandleTraceLine(FName Kind, const FString& What, const FString& Line)
{
	Lines.Add({Kind, What, Line});
	for (FHandler& Handler : Handlers)
	{
		if (Handler.Kind == Kind && (Handler.Contains.IsEmpty() || What.Contains(Handler.Contains, ESearchCase::IgnoreCase)))
		{
			++Handler.Pending;
		}
	}
}

void UJamKitScriptSubsystem::HandleScreenshot(int32 Width, int32 Height, const TArray<FColor>& Colors)
{
	// Mid-draw: keep the pixels and carry on from the next tick.
	if (bCapturing && !bCaptured)
	{
		CapturedPixels = Colors;
		CapturedSize = FIntPoint(Width, Height);
		bCaptured = true;
	}
}

int32 UJamKitScriptSubsystem::FindLine(FName Kind, const FString& Contains, int32 From) const
{
	for (int32 Index = FMath::Max(From, 0); Index < Lines.Num(); ++Index)
	{
		if (Lines[Index].Kind == Kind && (Contains.IsEmpty() || Lines[Index].What.Contains(Contains, ESearchCase::IgnoreCase)))
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

bool UJamKitScriptSubsystem::LoopIsOver(const FFrame& Frame) const
{
	if (Frame.Times > 0 && Frame.Rounds >= Frame.Times)
	{
		return true;
	}
	if (Frame.Seconds > 0.0 && Frame.Elapsed >= Frame.Seconds)
	{
		return true;
	}
	if (Frame.Until.IsValid())
	{
		const FName Kind(*JamKitScriptImpl::Text(*Frame.Until, TEXT("event")));
		return FindLine(Kind, JamKitScriptImpl::Text(*Frame.Until, TEXT("contains")), Frame.LoopCursor) != INDEX_NONE;
	}
	return Frame.Times <= 0 && Frame.Seconds <= 0.0 && Frame.Rounds >= 1; // no limit at all: once through
}

bool UJamKitScriptSubsystem::Tick(float DeltaTime)
{
	if (ExitTimer >= 0.0)
	{
		// A moment for the last lines to reach the trace and the log.
		if ((ExitTimer += DeltaTime) > 1.0)
		{
			ExitTimer = -2.0;
			FPlatformMisc::RequestExitWithStatus(false, Failed > 0 ? 1 : 0, TEXT("JamKitScript"));
		}
		return true;
	}
	if (ExitTimer < -1.0)
	{
		return true;
	}
	if (!bStarted)
	{
		bStarted = GetGameWorld() && FPlatformTime::Seconds() - StartTime > JamKitScriptImpl::StartDelay;
		return true;
	}
	if ((RunTime += DeltaTime) > Timeout)
	{
		++Failed;
		Finish(FString::Printf(TEXT("timed out after %.0f s"), Timeout));
		return true;
	}
	for (FFrame& Frame : Stack)
	{
		if (Frame.bLoop)
		{
			Frame.Elapsed += DeltaTime;
		}
	}

	// A handler whose event came runs on top of whatever is running, which carries on after it. One at a time.
	if (GetGameWorld() && !Stack.ContainsByPredicate([](const FFrame& Frame) { return Frame.Handler != INDEX_NONE; }))
	{
		for (int32 Index = 0; Index < Handlers.Num(); ++Index)
		{
			if (Handlers[Index].Pending > 0)
			{
				--Handlers[Index].Pending;
				FFrame& Frame = Stack.AddDefaulted_GetRef();
				Frame.Steps = Handlers[Index].Steps;
				Frame.Handler = Index;
				UE_LOG(LogJamKit, Display, TEXT("Script: handler %d (%s)"), Index, *Handlers[Index].Kind.ToString());
				break;
			}
		}
	}

	// Steps that finish at once run back to back; one step that takes time gets one update per tick.
	bool bUpdated = false;
	for (int32 Guard = 0; Guard < JamKitScriptImpl::MaxStepsPerTick; ++Guard)
	{
		if (Stack.IsEmpty())
		{
			Finish();
			return true;
		}
		const int32 Top = Stack.Num() - 1;
		{
			FFrame& Frame = Stack[Top];
			if (!Frame.bStarted && Frame.bLoop && LoopIsOver(Frame))
			{
				PopFrame();
				continue;
			}
			if (Frame.Index >= Frame.Steps.Num())
			{
				++Frame.Rounds;
				if (Frame.bLoop && !Frame.Steps.IsEmpty() && !LoopIsOver(Frame))
				{
					Frame.Index = 0;
					if (bUpdated)
					{
						return true; // a loop goes round at most once a tick
					}
					continue;
				}
				PopFrame();
				continue;
			}
		}

		const TSharedPtr<FJsonObject> Step = Stack[Top].Steps[Stack[Top].Index];
		bool bDone;
		if (!Stack[Top].bStarted)
		{
			Stack[Top].bStarted = true;
			Stack[Top].StepTime = 0.0;
			Stack[Top].SettleTime = 0.0;
			if (Stack[Top].Handler == INDEX_NONE && JamKitScriptImpl::IsAction(JamKitScriptImpl::OpOf(*Step)))
			{
				ActionCursor = Lines.Num();
			}
			bDone = StartStep(Stack[Top], *Step);
			if (!bDone && Stack.Num() > Top + 1)
			{
				continue; // it opened a frame of its own (repeat): start that
			}
		}
		else
		{
			if (bUpdated)
			{
				return true;
			}
			bUpdated = true;
			Stack[Top].StepTime += DeltaTime;
			bDone = UpdateStep(Stack[Top], *Step, DeltaTime);
		}
		if (ExitTimer >= 0.0)
		{
			return true;
		}
		if (!bDone)
		{
			return true;
		}
		if (Stack.IsValidIndex(Top))
		{
			++Stack[Top].Index;
			Stack[Top].bStarted = false;
		}
	}
	return true;
}

void UJamKitScriptSubsystem::PopFrame()
{
	FFrame Popped = Stack.Pop();
	StopHold(Popped);
	if (Popped.Handler != INDEX_NONE)
	{
		Handlers[Popped.Handler].bRunning = false;
	}
	else if (Popped.bLoop && !Stack.IsEmpty())
	{
		// The repeat step that opened it is done.
		FFrame& Parent = Stack.Last();
		EndStep(Parent, *Parent.Steps[Parent.Index], true, FString::Printf(TEXT("%d round(s), %.1f s"), Popped.Rounds, Popped.Elapsed));
		++Parent.Index;
		Parent.bStarted = false;
	}
}

bool UJamKitScriptSubsystem::StartStep(FFrame& Frame, const FJsonObject& Step)
{
	using namespace JamKitScriptImpl;
	const FString Op = OpOf(Step);
	UWorld* World = GetGameInstance()->GetWorld();
	APlayerController* Player = GetPlayer();
	APawn* Pawn = Player ? Player->GetPawn() : nullptr;

	if (Op == TEXT("open"))
	{
		TArray<FString> Commands;
		Step.TryGetStringArrayField(TEXT("commands"), Commands);
		for (const FString& Command : Commands)
		{
			GEngine->Exec(World, *Command);
		}
		const FString Level = Step.GetStringField(TEXT("open"));
		OpeningLevel = FPackageName::GetShortName(Level);
		UGameplayStatics::OpenLevel(World, FName(*Level), true, Text(Step, TEXT("options")));
		return false;
	}
	if (Op == TEXT("exec"))
	{
		GEngine->Exec(World, *Step.GetStringField(TEXT("exec")));
		EndStep(Frame, Step, true, FString());
		return true;
	}
	if (Op == TEXT("teleport") || Op == TEXT("look") || Op == TEXT("lookAt"))
	{
		if (!Player || !Pawn)
		{
			EndStep(Frame, Step, false, TEXT("no player"));
			return true;
		}
		FRotator View = Player->GetControlRotation();
		if (Op == TEXT("teleport"))
		{
			Pawn->SetActorLocation(Vector(Step, TEXT("teleport"), Pawn->GetActorLocation()), false, nullptr, ETeleportType::TeleportPhysics);
			if (UPawnMovementComponent* Movement = Pawn->GetMovementComponent())
			{
				Movement->StopMovementImmediately();
			}
			View.Yaw = Number(Step, TEXT("yaw"), View.Yaw);
			View.Pitch = Number(Step, TEXT("pitch"), View.Pitch);
		}
		else if (Op == TEXT("look"))
		{
			const FVector PitchYaw = Vector(Step, TEXT("look"), FVector(View.Pitch, View.Yaw, 0.0));
			View = FRotator(PitchYaw.X, PitchYaw.Y, 0.0);
		}
		else
		{
			FVector Target;
			if (!FindTarget(Step, TEXT("lookAt"), Target))
			{
				EndStep(Frame, Step, false, TEXT("no such target"));
				return true;
			}
			View = (Target - Pawn->GetPawnViewLocation()).Rotation();
		}
		Player->SetControlRotation(View);
		EndStep(Frame, Step, true, FString());
		return true;
	}
	if (Op == TEXT("press") || Op == TEXT("hold") || Op == TEXT("moveTo"))
	{
		FString Note;
		if (Op == TEXT("moveTo"))
		{
			FVector Target;
			if (!FindTarget(Step, TEXT("moveTo"), Target))
			{
				EndStep(Frame, Step, false, TEXT("no such target"));
				return true;
			}
			return UpdateStep(Frame, Step, 0.0f);
		}
		if (!Inject(Frame, Step, Op == TEXT("hold"), Note))
		{
			EndStep(Frame, Step, false, Note);
			return true;
		}
		return false;
	}
	if (Op == TEXT("wait"))
	{
		return false;
	}
	if (Op == TEXT("set"))
	{
		FString Note;
		const bool bOk = SetProperty(Step, Note);
		EndStep(Frame, Step, bOk, Note);
		return true;
	}
	if (Op == TEXT("call"))
	{
		AActor* Actor = FindActor(Step.GetStringField(TEXT("call")));
		FOutputDeviceNull Output;
		const bool bOk = Actor && Actor->CallFunctionByNameWithArguments(*Text(Step, TEXT("function")), Output, nullptr, true);
		EndStep(Frame, Step, bOk, Actor ? (bOk ? FString() : TEXT("no such function")) : TEXT("no such actor"));
		return true;
	}
	if (Op == TEXT("choose") || Op == TEXT("expect") || Op == TEXT("waitFor"))
	{
		return UpdateStep(Frame, Step, 0.0f);
	}
	if (Op == TEXT("repeat"))
	{
		FFrame Loop;
		Loop.Steps = StepsOf(Step, TEXT("repeat"));
		Loop.bLoop = true;
		Loop.Times = static_cast<int32>(Number(Step, TEXT("times"), 0.0));
		Loop.Seconds = Number(Step, TEXT("seconds"), 0.0);
		const TSharedPtr<FJsonObject>* Until = nullptr;
		if (Step.TryGetObjectField(TEXT("until"), Until))
		{
			Loop.Until = *Until;
		}
		Loop.LoopCursor = Lines.Num();
		Stack.Add(MoveTemp(Loop)); // Frame is not to be used after this
		return false;
	}
	if (Op == TEXT("shot"))
	{
		bCapturing = true;
		bCaptured = false;
		bool bUI = true;
		Step.TryGetBoolField(TEXT("ui"), bUI);
		FScreenshotRequest::RequestScreenshot(bUI);
		return false;
	}
	if (Op == TEXT("note"))
	{
		JamKitTrace::Event(World, TEXT("script"), Step.GetStringField(TEXT("note")));
		EndStep(Frame, Step, true, FString());
		return true;
	}
	EndStep(Frame, Step, false, TEXT("unknown step"));
	return true;
}

bool UJamKitScriptSubsystem::UpdateStep(FFrame& Frame, const FJsonObject& Step, float DeltaTime)
{
	using namespace JamKitScriptImpl;
	const FString Op = OpOf(Step);

	if (Op == TEXT("open"))
	{
		const UWorld* World = GetGameWorld();
		const APlayerController* Player = GetPlayer();
		if (World && Player && Player->GetPawn() && UWorld::RemovePIEPrefix(World->GetMapName()) == OpeningLevel)
		{
			// A hitch doesn't count as settling.
			if ((Frame.SettleTime += FMath::Min(DeltaTime, MaxSettleStep)) >= Number(Step, TEXT("settle"), 3.0))
			{
				EndStep(Frame, Step, true, FString());
				return true;
			}
		}
		if (Frame.StepTime > OpenTimeout)
		{
			EndStep(Frame, Step, false, TEXT("the level didn't open"));
			Finish(TEXT("couldn't open a level"));
			return true;
		}
		return false;
	}
	if (Op == TEXT("press"))
	{
		// Injected for one frame: pressed this frame, released the next.
		EndStep(Frame, Step, true, FString());
		return true;
	}
	if (Op == TEXT("hold") || Op == TEXT("wait"))
	{
		if (Frame.StepTime < Number(Step, Op == TEXT("hold") ? TEXT("seconds") : TEXT("wait"), 1.0))
		{
			return false;
		}
		StopHold(Frame);
		EndStep(Frame, Step, true, FString());
		return true;
	}
	if (Op == TEXT("moveTo"))
	{
		APlayerController* Player = GetPlayer();
		APawn* Pawn = Player ? Player->GetPawn() : nullptr;
		FVector Target;
		if (!Pawn || !FindTarget(Step, TEXT("moveTo"), Target))
		{
			EndStep(Frame, Step, false, Pawn ? TEXT("the target went") : TEXT("no player"));
			return true;
		}
		const FVector2D ToTarget(Target - Pawn->GetActorLocation());
		if (ToTarget.Size() <= Number(Step, TEXT("radius"), 100.0))
		{
			EndStep(Frame, Step, true, FString::Printf(TEXT("%.0f s"), Frame.StepTime));
			return true;
		}
		if (Frame.StepTime > Number(Step, TEXT("within"), 10.0))
		{
			EndStep(Frame, Step, false, FString::Printf(TEXT("still %.0f cm away"), ToTarget.Size()));
			return true;
		}
		// Into the move action's terms: X right, Y forward, of the view (or of the world's X axis).
		const double Yaw = Text(Step, TEXT("frame")).Equals(TEXT("world"), ESearchCase::IgnoreCase) ? 0.0 : FMath::DegreesToRadians(Player->GetControlRotation().Yaw);
		const FVector2D Forward(FMath::Cos(Yaw), FMath::Sin(Yaw));
		const FVector2D Right(-FMath::Sin(Yaw), FMath::Cos(Yaw));
		const FVector2D Direction = ToTarget.GetSafeNormal();
		FString ActionName = Text(Step, TEXT("action"));
		const UInputAction* Action = FindAction(ActionName.IsEmpty() ? TEXT("IA_Move") : ActionName);
		UEnhancedInputLocalPlayerSubsystem* Input = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Player->GetLocalPlayer());
		if (!Action || !Input)
		{
			EndStep(Frame, Step, false, TEXT("no move action"));
			return true;
		}
		Input->InjectInputForAction(Action, FInputActionValue(Action->ValueType, FVector(FVector2D::DotProduct(Direction, Right), FVector2D::DotProduct(Direction, Forward), 0.0)), {}, {});
		return false;
	}
	if (Op == TEXT("choose"))
	{
		FString Note;
		if (TryChoose(Step, Note))
		{
			EndStep(Frame, Step, true, Note);
			return true;
		}
		if (Frame.StepTime >= Number(Step, TEXT("within"), 5.0))
		{
			EndStep(Frame, Step, false, Note);
			return true;
		}
		return false;
	}
	if (Op == TEXT("expect") || Op == TEXT("waitFor"))
	{
		const bool bNot = Flag(Step, TEXT("not"));
		const FString Contains = Text(Step, TEXT("contains"));
		const FName Kind(*Step.GetStringField(Op));
		// Since the last action step began, or ("since": "start") the whole run; "count": at least that many.
		const int32 Want = FMath::Max(static_cast<int32>(Number(Step, TEXT("count"), 1.0)), 1);
		int32 Seen = INDEX_NONE;
		int32 Count = 0;
		for (int32 From = Text(Step, TEXT("since")) == TEXT("start") ? 0 : ActionCursor; Count < Want; ++Count, From = Seen + 1)
		{
			const int32 Next = FindLine(Kind, Contains, From);
			if (Next == INDEX_NONE)
			{
				break;
			}
			Seen = Next;
		}
		const bool bSeen = Count >= Want;
		if (bSeen || Frame.StepTime >= Number(Step, TEXT("within"), Op == TEXT("waitFor") ? 60.0 : 3.0))
		{
			const bool bOk = bSeen != bNot;
			const FString What = FString::Printf(TEXT("%s \"%s\"%s"), *Kind.ToString(), *Contains, Want > 1 ? *FString::Printf(TEXT(" x%d"), Want) : TEXT(""));
			EndStep(Frame, Step, bOk, bOk ? (Want > 1 ? FString::Printf(TEXT("%d seen"), Count) : FString())
				: bNot ? TEXT("came: ") + What : FString::Printf(TEXT("only %d of "), Count) + What,
				bSeen && Seen != INDEX_NONE ? Lines[Seen].Line : FString());
			return true;
		}
		return false;
	}
	if (Op == TEXT("shot"))
	{
		if (!bCaptured && Frame.StepTime < ShotTimeout)
		{
			return false;
		}
		bCapturing = false;
		FString Json;
		FString Summary;
		JamKitVerify::AnalyseImage(CapturedPixels, CapturedSize, Json, Summary);
		const FString File = Step.GetStringField(TEXT("shot")) + TEXT(".png");
		const bool bOk = bCaptured && JamKitVerify::SavePng(CapturedPixels, CapturedSize, OutputDir / File);
		WriteResult(FString::Printf(TEXT("{\"k\":\"shot\",\"file\":\"%s\"%s}"), *JamKitTrace::Escape(File), *Json));
		EndStep(Frame, Step, bOk, bOk ? File + TEXT(": ") + Summary : TEXT("no picture"));
		CapturedPixels.Reset();
		return true;
	}
	// repeat: finished by its frame (PopFrame).
	return false;
}

bool UJamKitScriptSubsystem::TryChoose(const FJsonObject& Step, FString& OutNote)
{
	const FName Id(*Step.GetStringField(TEXT("choose")));
	UJamKitChoiceSubsystem* Choices = GetGameInstance()->GetSubsystem<UJamKitChoiceSubsystem>();
	const FJamKitChoice* Choice = Choices ? Choices->Find(Id) : nullptr;
	if (!Choice || Choice->Options.IsEmpty())
	{
		OutNote = TEXT("no choice ") + Id.ToString() + TEXT(" was offered");
		return false;
	}
	const TArray<FName>& Options = Choice->Options;
	auto IndexContaining = [&Options](const FString& Text)
	{
		return Options.IndexOfByPredicate([&Text](FName Option) { return Option.ToString().Contains(Text, ESearchCase::IgnoreCase); });
	};

	int32 Index = INDEX_NONE;
	FString How;
	TArray<FString> Prefer;
	if (Step.TryGetStringArrayField(TEXT("prefer"), Prefer))
	{
		for (const FString& Each : Prefer)
		{
			if ((Index = IndexContaining(Each)) != INDEX_NONE)
			{
				How = TEXT("preferred");
				break;
			}
		}
	}
	if (Index == INDEX_NONE && Step.HasField(TEXT("pick")))
	{
		double Number = -1.0;
		Index = Step.TryGetNumberField(TEXT("pick"), Number) ? static_cast<int32>(Number) : IndexContaining(Step.GetStringField(TEXT("pick")));
		How = TEXT("picked");
	}
	if (!Options.IsValidIndex(Index) && JamKitScriptImpl::Flag(Step, TEXT("random")))
	{
		Index = JamKitRandom::Stream(GetGameInstance()->GetWorld(), TEXT("Script")).RandRange(0, Options.Num() - 1);
		How = TEXT("random");
	}
	if (!Options.IsValidIndex(Index))
	{
		Index = 0;
		How = TEXT("first");
	}
	OutNote = Options[Index].ToString() + TEXT(" (") + How + TEXT(" of ") + FString::JoinBy(Options, TEXT(", "), [](FName Option) { return Option.ToString(); }) + TEXT(")");
	return Choices->Choose(Id, Index, TEXT("script"));
}

const UInputAction* UJamKitScriptSubsystem::FindAction(const FString& ActionName) const
{
	if (ActionName.Contains(TEXT("/")))
	{
		return LoadObject<UInputAction>(nullptr, *ActionName);
	}
	if (const UInputMappingContext* Context = GetDefault<UJamKitSettings>()->RebindContext.LoadSynchronous())
	{
		for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
		{
			if (Mapping.Action && Mapping.Action->GetName().Equals(ActionName, ESearchCase::IgnoreCase))
			{
				return Mapping.Action;
			}
		}
	}
	return FindFirstObject<UInputAction>(*ActionName, EFindFirstObjectOptions::None);
}

bool UJamKitScriptSubsystem::Inject(FFrame& Frame, const FJsonObject& Step, bool bContinuous, FString& OutNote)
{
	const FString ActionName = Step.GetStringField(bContinuous ? TEXT("hold") : TEXT("press"));
	const UInputAction* Action = FindAction(ActionName);
	const APlayerController* Player = GetPlayer();
	UEnhancedInputLocalPlayerSubsystem* Input = Player ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Player->GetLocalPlayer()) : nullptr;
	if (!Action || !Input)
	{
		OutNote = Action ? TEXT("no player input") : TEXT("no input action ") + ActionName;
		return false;
	}
	const FInputActionValue Value(Action->ValueType, JamKitScriptImpl::Vector(Step, TEXT("value"), FVector(1.0, 0.0, 0.0)));
	if (bContinuous)
	{
		Input->StartContinuousInputInjectionForAction(Action, Value, {}, {});
		Frame.HeldAction = Action;
	}
	else
	{
		Input->InjectInputForAction(Action, Value, {}, {});
	}
	return true;
}

void UJamKitScriptSubsystem::StopHold(FFrame& Frame)
{
	if (const UInputAction* Action = Frame.HeldAction.Get())
	{
		const APlayerController* Player = GetPlayer();
		if (UEnhancedInputLocalPlayerSubsystem* Input = Player ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Player->GetLocalPlayer()) : nullptr)
		{
			Input->StopContinuousInputInjectionForAction(Action);
		}
	}
	Frame.HeldAction.Reset();
}

AActor* UJamKitScriptSubsystem::FindActor(const FString& ActorName) const
{
	UWorld* World = GetGameWorld();
	if (!World)
	{
		return nullptr;
	}
	// An exact name or class first (BP_Door_C_2, BP_Darkroom_C, BP_Darkroom), then a part of either.
	for (int32 Pass = 0; Pass < 2; ++Pass)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			const FString Object = It->GetName();
			FString Class = It->GetClass()->GetName();
			if (Pass == 0)
			{
				Class.RemoveFromEnd(TEXT("_C"));
				if (Object.Equals(ActorName, ESearchCase::IgnoreCase) || It->GetClass()->GetName().Equals(ActorName, ESearchCase::IgnoreCase)
					|| Class.Equals(ActorName, ESearchCase::IgnoreCase))
				{
					return *It;
				}
			}
			else if (Object.Contains(ActorName) || Class.Contains(ActorName))
			{
				return *It;
			}
		}
	}
	return nullptr;
}

bool UJamKitScriptSubsystem::FindTarget(const FJsonObject& Step, const TCHAR* Field, FVector& OutLocation) const
{
	const TSharedPtr<FJsonValue> Value = Step.TryGetField(Field);
	if (!Value.IsValid())
	{
		return false;
	}
	if (Value->Type == EJson::Array)
	{
		OutLocation = JamKitScriptImpl::Vector(Step, Field, FVector::ZeroVector);
		return true;
	}
	const AActor* Actor = FindActor(Value->AsString());
	if (Actor)
	{
		OutLocation = Actor->GetActorLocation();
	}
	return Actor != nullptr;
}

bool UJamKitScriptSubsystem::SetProperty(const FJsonObject& Step, FString& OutNote) const
{
	AActor* Actor = FindActor(Step.GetStringField(TEXT("set")));
	if (!Actor)
	{
		OutNote = TEXT("no such actor");
		return false;
	}
	UObject* Target = Actor;
	const FString ComponentName = JamKitScriptImpl::Text(Step, TEXT("component"));
	if (!ComponentName.IsEmpty())
	{
		Target = nullptr;
		for (UActorComponent* Component : Actor->GetComponents())
		{
			if (Component && Component->GetName().Contains(ComponentName))
			{
				Target = Component;
				break;
			}
		}
		if (!Target)
		{
			OutNote = TEXT("no such component");
			return false;
		}
	}
	const FString PropertyName = JamKitScriptImpl::Text(Step, TEXT("property"));
	FProperty* Property = FindFProperty<FProperty>(Target->GetClass(), *PropertyName);
	if (!Property)
	{
		OutNote = TEXT("no property ") + PropertyName + TEXT(" on ") + Target->GetName();
		return false;
	}
	const FString Value = JamKitScriptImpl::Text(Step, TEXT("value"));
	if (!Property->ImportText_InContainer(*Value, Target, Target, PPF_None))
	{
		OutNote = TEXT("couldn't read the value ") + Value;
		return false;
	}
	if (USceneComponent* Scene = Cast<USceneComponent>(Target))
	{
		Scene->UpdateComponentToWorld();
	}
	OutNote = Target->GetName() + TEXT(".") + PropertyName + TEXT(" = ") + Value;
	return true;
}

void UJamKitScriptSubsystem::EndStep(const FFrame& Frame, const FJsonObject& Step, bool bOk, const FString& Note, const FString& MatchedLine)
{
	const FString Op = JamKitScriptImpl::OpOf(Step);
	const FString Subject = Op.IsEmpty() ? FString() : JamKitScriptImpl::Text(Step, *Op);
	const bool bCheck = Op == TEXT("expect");
	if (bCheck || !bOk)
	{
		bOk ? ++Passed : ++Failed;
	}
	const FString Where = Frame.Handler != INDEX_NONE ? FString::Printf(TEXT("on%d."), Frame.Handler)
		: Frame.bLoop ? FString::Printf(TEXT("r%d."), Frame.Rounds) : FString();
	WriteResult(FString::Printf(TEXT("{\"k\":\"step\",\"step\":\"%s%d\",\"op\":\"%s\",\"what\":\"%s\",\"ok\":%s,\"t\":%.2f,\"note\":\"%s\"%s}"),
		*Where, Frame.Index, *Op, *JamKitTrace::Escape(Subject), bOk ? TEXT("true") : TEXT("false"), RunTime, *JamKitTrace::Escape(Note),
		MatchedLine.IsEmpty() ? TEXT("") : *(TEXT(",\"line\":") + MatchedLine)));
	if (bOk)
	{
		UE_LOG(LogJamKit, Display, TEXT("Script: %5s%-3d %-8s %s %s"), *Where, Frame.Index, *Op, *Subject, *Note);
	}
	else
	{
		UE_LOG(LogJamKit, Warning, TEXT("Script: %5s%-3d %-8s %s FAILED: %s"), *Where, Frame.Index, *Op, *Subject, *Note);
	}
	JamKitTrace::Event(GetGameWorld(), TEXT("script"), Op + TEXT(" ") + Subject, FJamKitTraceFields().Add(TEXT("ok"), bOk).Add(TEXT("note"), Note));
}

void UJamKitScriptSubsystem::Finish(const FString& Why)
{
	for (FFrame& Frame : Stack)
	{
		StopHold(Frame);
	}
	if (!ReportKind.IsEmpty())
	{
		for (int32 Index = Lines.Num() - 1; Index >= 0; --Index)
		{
			if (Lines[Index].Kind == FName(*ReportKind))
			{
				WriteResult(TEXT("{\"k\":\"report\",\"line\":") + Lines[Index].Line + TEXT("}"));
				break;
			}
		}
	}
	const UJamKitRandomSubsystem* Random = GetGameInstance()->GetSubsystem<UJamKitRandomSubsystem>();
	WriteResult(FString::Printf(TEXT("{\"k\":\"done\",\"passed\":%d,\"failed\":%d,\"seed\":%d,\"gameSeconds\":%.1f,\"realSeconds\":%.1f,\"why\":\"%s\"}"),
		Passed, Failed, Random ? Random->GetSeed() : 0, RunTime, FPlatformTime::Seconds() - StartTime, *JamKitTrace::Escape(Why)));
	UE_LOG(LogJamKit, Display, TEXT("Script: %s: %s, %d check(s) passed, %d failed%s"), *Name, Failed ? TEXT("FAIL") : TEXT("PASS"), Passed, Failed,
		Why.IsEmpty() ? TEXT("") : *(TEXT(" (") + Why + TEXT(")")));
	Stack.Reset();
	ExitTimer = 0.0;
}
