#include "Random/JamKitRandom.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "JamKit.h"
#include "Misc/App.h"
#include "Verify/JamKitTrace.h"
#include "Verify/JamKitVerify.h"

namespace JamKitRandomImpl
{
	static FAutoConsoleCommandWithWorldAndArgs CmdSeed(
		TEXT("jamkit.Seed"),
		TEXT("jamkit.Seed <n>: the gameplay random seed from the next level on (no argument: print the current one)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UJamKitRandomSubsystem* Random = UJamKitRandomSubsystem::Get(World))
			{
				if (Args.Num() > 0)
				{
					Random->SetSeed(FCString::Atoi(*Args[0]));
				}
				UE_LOG(LogJamKit, Display, TEXT("Seed: %d"), Random->GetSeed());
			}
		}));

	void SetCVar(const TCHAR* Name, const TCHAR* Value)
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			CVar->Set(Value, ECVF_SetByCommandline);
		}
	}
}

void UJamKitRandomSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Collection.InitializeDependency<UJamKitTraceSubsystem>();
	Super::Initialize(Collection);

	FString Value;
	if (JamKitVerify::GetValue(TEXT("jamseed="), Value))
	{
		Seed = FCString::Atoi(*Value);
	}
	else
	{
		// Fresh per launch, and small enough to read out and type back in.
		Seed = static_cast<int32>(FPlatformTime::Cycles64() % 1000000);
	}
	UE_LOG(LogJamKit, Display, TEXT("Seed: %d"), Seed);

	if (JamKitVerify::GetValue(TEXT("jamfixedstep="), Value))
	{
		const double FramesPerSecond = FMath::Max(FCString::Atod(*Value), 1.0);
		FApp::SetUseFixedTimeStep(true);
		FApp::SetFixedDeltaTime(1.0 / FramesPerSecond);
		// Every frame is the same slice of game time however long it takes, so don't wait for a frame cap or vsync.
		JamKitRandomImpl::SetCVar(TEXT("t.MaxFPS"), TEXT("0"));
		JamKitRandomImpl::SetCVar(TEXT("r.VSync"), TEXT("0"));
		UE_LOG(LogJamKit, Display, TEXT("Fixed time step: %.1f frames per game second"), FramesPerSecond);
	}

	WorldHandle = FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &UJamKitRandomSubsystem::HandleWorldInitialized);
}

void UJamKitRandomSubsystem::Deinitialize()
{
	FWorldDelegates::OnPostWorldInitialization.Remove(WorldHandle);
	Super::Deinitialize();
}

UJamKitRandomSubsystem* UJamKitRandomSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UJamKitRandomSubsystem>() : nullptr;
}

void UJamKitRandomSubsystem::SetSeed(int32 NewSeed, bool bRestartStreams)
{
	if (bRestartStreams)
	{
		Seed = NewSeed;
		bPendingSeed = false;
		RestartStreams();
	}
	else
	{
		PendingSeed = NewSeed;
		bPendingSeed = true;
	}
}

void UJamKitRandomSubsystem::HandleWorldInitialized(UWorld* World, const UWorld::InitializationValues Values)
{
	if (World && World->IsGameWorld() && World->GetGameInstance() == GetGameInstance())
	{
		if (bPendingSeed)
		{
			Seed = PendingSeed;
			bPendingSeed = false;
		}
		RestartStreams();
		JamKitTrace::Event(World, TEXT("seed"), FString::FromInt(Seed));
	}
}

void UJamKitRandomSubsystem::RestartStreams()
{
	// Keep the stream objects (callers may hold references); put each back to its start.
	for (TPair<FName, TUniquePtr<FRandomStream>>& Pair : Streams)
	{
		Pair.Value->Initialize(StreamSeed(Pair.Key));
	}
}

int32 UJamKitRandomSubsystem::StreamSeed(FName Name) const
{
	// The name's text, not its FName index, so the same name gives the same sequence in every launch.
	return static_cast<int32>(HashCombine(static_cast<uint32>(Seed), GetTypeHash(Name.ToString())));
}

FRandomStream& UJamKitRandomSubsystem::Stream(FName Name)
{
	if (TUniquePtr<FRandomStream>* Existing = Streams.Find(Name))
	{
		return **Existing;
	}
	return *Streams.Add(Name, MakeUnique<FRandomStream>(StreamSeed(Name)));
}

FRandomStream& JamKitRandom::Stream(const UObject* WorldContext, FName Name)
{
	if (UJamKitRandomSubsystem* Random = UJamKitRandomSubsystem::Get(WorldContext))
	{
		return Random->Stream(Name);
	}
	static FRandomStream Fallback(static_cast<int32>(FPlatformTime::Cycles()));
	return Fallback;
}
