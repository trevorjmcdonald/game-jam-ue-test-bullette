#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "JamKitTrace.generated.h"

/** Extra fields for a trace event, written as JSON. Chain the Adds: FJamKitTraceFields().Add(TEXT("vol"), 0.5). */
struct JAMKIT_API FJamKitTraceFields
{
	FJamKitTraceFields& Add(const TCHAR* Key, const FString& Value);
	FJamKitTraceFields& Add(const TCHAR* Key, const TCHAR* Value) { return Add(Key, FString(Value)); }
	FJamKitTraceFields& Add(const TCHAR* Key, const FText& Value) { return Add(Key, Value.ToString()); }
	FJamKitTraceFields& Add(const TCHAR* Key, FName Value) { return Add(Key, Value.ToString()); }
	FJamKitTraceFields& Add(const TCHAR* Key, double Value);
	FJamKitTraceFields& Add(const TCHAR* Key, float Value) { return Add(Key, static_cast<double>(Value)); }
	FJamKitTraceFields& Add(const TCHAR* Key, int32 Value);
	FJamKitTraceFields& Add(const TCHAR* Key, bool Value);
	FJamKitTraceFields& Add(const TCHAR* Key, const FVector& Value);
	FJamKitTraceFields& Add(const TCHAR* Key, const TArray<FName>& Value);

	bool IsEmpty() const { return Json.IsEmpty(); }

	/** The fields so far, each as ,"key":value. */
	FString Json;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FJamKitTraceContextSignature, FJamKitTraceFields& /*Fields*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FJamKitTraceLineSignature, FName /*Kind*/, const FString& /*What*/, const FString& /*Line*/);

/**
 * A record of what happened in a run, for reading afterwards: one JSON object per line in
 * Saved/Trace/<date-time>.jsonl, a new file per game instance (each PIE session, each launch). Nobody has to watch
 * or listen: the sounds that played (wave, volume, length), what the HUD said, what was used, levels, and the
 * player's whereabouts every couple of seconds while they move. On in every build but Shipping; -nojamtrace turns it
 * off. The last 30 files are kept.
 *
 * Each line: {"t": seconds since the run started, "lvl": level, "k": kind, "w": what, ...context, ...fields}.
 * The game adds its own context to every line (the clock, the day) through OnContext.
 */
UCLASS()
class JAMKIT_API UJamKitTraceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	static UJamKitTraceSubsystem* Get(const UObject* WorldContext);

	void Write(const UObject* WorldContext, FName Kind, const FString& What, const FJamKitTraceFields& Fields);

	/** The newest lines, oldest first (at most the last few hundred). */
	const TArray<FString>& GetRecentLines() const { return Recent; }

	FString GetFilePath() const { return FilePath; }

	/** Called for every line; add fields that describe the moment (game clock, day, ...). */
	FJamKitTraceContextSignature OnContext;

	/** Every line as it's written (UJamKitScriptSubsystem checks its expectations against these). */
	FJamKitTraceLineSignature OnLine;

	/** How often the player's position is sampled (only written when they've moved or turned). */
	static constexpr float PositionInterval = 2.0f;

private:
	bool SamplePosition(float DeltaTime);
	void OpenFile();
	static void PruneOldFiles(const FString& Directory, int32 Keep);

	FString FilePath;
	TUniquePtr<FArchive> File;
	TArray<FString> Recent;
	double StartTime = 0.0;
	FTSTicker::FDelegateHandle TickerHandle;
	FVector LastPosition = FVector(TNumericLimits<float>::Max());
	FRotator LastRotation;
	bool bWriting = false;
};

namespace JamKitTrace
{
	/** Records an event (does nothing when the trace is off). Kind is a short category: sound, message, prompt, interact, level, ... */
	JAMKIT_API void Event(const UObject* WorldContext, FName Kind, const FString& What, const FJamKitTraceFields& Fields = FJamKitTraceFields());

	/** Escapes a string for a JSON string literal (without the quotes). */
	JAMKIT_API FString Escape(const FString& Text);
}
