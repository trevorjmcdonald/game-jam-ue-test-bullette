#include "Verify/JamKitTrace.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "JamKit.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "Misc/Paths.h"

namespace JamKitTraceImpl
{
	constexpr int32 RecentLines = 400;
	constexpr int32 FilesKept = 30;

	FString Number(double Value)
	{
		// Short and still exact enough: 3 decimals, no trailing zeros.
		FString Text = FString::Printf(TEXT("%.3f"), Value);
		Text.TrimEndInline();
		while (Text.EndsWith(TEXT("0")))
		{
			Text.LeftChopInline(1);
		}
		if (Text.EndsWith(TEXT(".")))
		{
			Text.LeftChopInline(1);
		}
		return Text == TEXT("-0") ? TEXT("0") : Text;
	}
}

FString JamKitTrace::Escape(const FString& Text)
{
	FString Out;
	Out.Reserve(Text.Len() + 8);
	for (const TCHAR C : Text)
	{
		switch (C)
		{
		case TEXT('"'): Out += TEXT("\\\""); break;
		case TEXT('\\'): Out += TEXT("\\\\"); break;
		case TEXT('\n'): Out += TEXT("\\n"); break;
		case TEXT('\r'): break;
		case TEXT('\t'): Out += TEXT("\\t"); break;
		default:
			if (C < 0x20)
			{
				Out += FString::Printf(TEXT("\\u%04x"), static_cast<int32>(C));
			}
			else
			{
				Out.AppendChar(C);
			}
		}
	}
	return Out;
}

FJamKitTraceFields& FJamKitTraceFields::Add(const TCHAR* Key, const FString& Value)
{
	Json += FString::Printf(TEXT(",\"%s\":\"%s\""), Key, *JamKitTrace::Escape(Value));
	return *this;
}

FJamKitTraceFields& FJamKitTraceFields::Add(const TCHAR* Key, double Value)
{
	Json += FString::Printf(TEXT(",\"%s\":%s"), Key, *JamKitTraceImpl::Number(Value));
	return *this;
}

FJamKitTraceFields& FJamKitTraceFields::Add(const TCHAR* Key, int32 Value)
{
	Json += FString::Printf(TEXT(",\"%s\":%d"), Key, Value);
	return *this;
}

FJamKitTraceFields& FJamKitTraceFields::Add(const TCHAR* Key, bool Value)
{
	Json += FString::Printf(TEXT(",\"%s\":%s"), Key, Value ? TEXT("true") : TEXT("false"));
	return *this;
}

FJamKitTraceFields& FJamKitTraceFields::Add(const TCHAR* Key, const FVector& Value)
{
	Json += FString::Printf(TEXT(",\"%s\":[%.0f,%.0f,%.0f]"), Key, Value.X, Value.Y, Value.Z);
	return *this;
}

FJamKitTraceFields& FJamKitTraceFields::Add(const TCHAR* Key, const TArray<FName>& Value)
{
	Json += FString::Printf(TEXT(",\"%s\":["), Key);
	for (int32 Index = 0; Index < Value.Num(); ++Index)
	{
		Json += FString::Printf(TEXT("%s\"%s\""), Index ? TEXT(",") : TEXT(""), *JamKitTrace::Escape(Value[Index].ToString()));
	}
	Json += TEXT("]");
	return *this;
}

bool UJamKitTraceSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	return !IsRunningCommandlet() && !FParse::Param(FCommandLine::Get(), TEXT("nojamtrace")) && Super::ShouldCreateSubsystem(Outer);
#endif
}

void UJamKitTraceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	StartTime = FPlatformTime::Seconds();
	// Automation tests make game instances of their own: keep their events in memory only.
	bWriting = !GIsAutomationTesting;
	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UJamKitTraceSubsystem::SamplePosition), PositionInterval);
}

void UJamKitTraceSubsystem::Deinitialize()
{
	FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
	if (File)
	{
		Write(nullptr, TEXT("end"), FString(), FJamKitTraceFields());
		File->Close();
		File.Reset();
	}
	Super::Deinitialize();
}

UJamKitTraceSubsystem* UJamKitTraceSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UJamKitTraceSubsystem>() : nullptr;
}

void UJamKitTraceSubsystem::OpenFile()
{
	const FString Directory = FPaths::ProjectSavedDir() / TEXT("Trace");
	PruneOldFiles(Directory, JamKitTraceImpl::FilesKept - 1);
	FilePath = Directory / FDateTime::Now().ToString(TEXT("%Y%m%d-%H%M%S")) + TEXT(".jsonl");
	// Two game instances in the same second (PIE restarted quickly): don't share a file.
	for (int32 Suffix = 2; IFileManager::Get().FileExists(*FilePath); ++Suffix)
	{
		FilePath = Directory / FDateTime::Now().ToString(TEXT("%Y%m%d-%H%M%S")) + FString::Printf(TEXT("_%d.jsonl"), Suffix);
	}
	File.Reset(IFileManager::Get().CreateFileWriter(*FilePath, FILEWRITE_AllowRead));
	if (File)
	{
		UE_LOG(LogJamKit, Display, TEXT("Trace: writing %s"), *FPaths::ConvertRelativePathToFull(FilePath));
	}
	else
	{
		UE_LOG(LogJamKit, Warning, TEXT("Trace: couldn't open %s"), *FilePath);
		bWriting = false;
	}
}

void UJamKitTraceSubsystem::PruneOldFiles(const FString& Directory, int32 Keep)
{
	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *(Directory / TEXT("*.jsonl")), true, false);
	if (Files.Num() <= Keep)
	{
		return;
	}
	Files.Sort(); // date-time names: oldest first
	for (int32 Index = 0; Index < Files.Num() - Keep; ++Index)
	{
		IFileManager::Get().Delete(*(Directory / Files[Index]));
	}
}

void UJamKitTraceSubsystem::Write(const UObject* WorldContext, FName Kind, const FString& What, const FJamKitTraceFields& Fields)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : GetGameInstance()->GetWorld();
	const FString Level = World ? UWorld::RemovePIEPrefix(World->GetMapName()) : FString();

	FJamKitTraceFields Context;
	OnContext.Broadcast(Context);

	const FString Line = FString::Printf(TEXT("{\"t\":%s,\"lvl\":\"%s\",\"k\":\"%s\",\"w\":\"%s\"%s%s}"),
		*JamKitTraceImpl::Number(FPlatformTime::Seconds() - StartTime), *JamKitTrace::Escape(Level), *Kind.ToString(),
		*JamKitTrace::Escape(What), *Context.Json, *Fields.Json);

	if (Recent.Num() >= JamKitTraceImpl::RecentLines)
	{
		Recent.RemoveAt(0, Recent.Num() - JamKitTraceImpl::RecentLines + 1, EAllowShrinking::No);
	}
	Recent.Add(Line);
	OnLine.Broadcast(Kind, What, Line);

	if (!bWriting)
	{
		return;
	}
	if (!File)
	{
		OpenFile();
		if (!File)
		{
			return;
		}
	}
	// Flushed every line: a crash or a killed process still leaves the run up to that moment.
	FTCHARToUTF8 Utf8(*(Line + TEXT("\n")));
	File->Serialize(const_cast<ANSICHAR*>(Utf8.Get()), Utf8.Length());
	File->Flush();
}

bool UJamKitTraceSubsystem::SamplePosition(float DeltaTime)
{
	const UWorld* World = GetGameInstance()->GetWorld();
	const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (!Pawn || World->IsPaused())
	{
		return true;
	}
	const FVector Position = Pawn->GetActorLocation();
	const FRotator Rotation = Controller->GetControlRotation();
	if (FVector::DistSquared(Position, LastPosition) < FMath::Square(25.0) && Rotation.Equals(LastRotation, 10.0))
	{
		return true;
	}
	LastPosition = Position;
	LastRotation = Rotation;
	Write(World, TEXT("pos"), FString(), FJamKitTraceFields().Add(TEXT("at"), Position)
		.Add(TEXT("yaw"), FMath::RoundToInt32(Rotation.Yaw)).Add(TEXT("pitch"), FMath::RoundToInt32(FRotator::NormalizeAxis(Rotation.Pitch))));
	return true;
}

void JamKitTrace::Event(const UObject* WorldContext, FName Kind, const FString& What, const FJamKitTraceFields& Fields)
{
	if (UJamKitTraceSubsystem* Trace = UJamKitTraceSubsystem::Get(WorldContext))
	{
		Trace->Write(WorldContext, Kind, What, Fields);
	}
}
