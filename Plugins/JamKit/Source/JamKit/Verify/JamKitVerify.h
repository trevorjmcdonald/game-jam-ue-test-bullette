#pragma once

#include "CoreMinimal.h"

/** Shared by the unattended runs (UJamKitProbeSubsystem, UJamKitScriptSubsystem). */
namespace JamKitVerify
{
	/**
	 * The command line plus Saved/jamkit-args.txt, if it's there. On a Steam Deck the game starts from Steam, so an
	 * unattended run is asked for by leaving that file (the same arguments, e.g. "-jamprobe -jamprobeperf=5") over SSH.
	 * The file is read once and deleted, so a crash mid-run doesn't start another at every launch.
	 */
	JAMKIT_API const FString& Args();

	JAMKIT_API bool HasParam(const TCHAR* Name);
	JAMKIT_API bool GetValue(const TCHAR* KeyWithEquals, FString& OutValue);

	/** A probe run or a scripted run is playing, nobody's at the controls: games skip what only a player should
	 * cause (saving a best score, unlocking things, achievements). */
	JAMKIT_API bool IsUnattended();

	/** Brightness of a frame as it's displayed (8-bit, gamma-encoded, Rec. 709 weights): JSON fields to append to an
	 * object (,"mean":...) and a one-line summary. Makes the pixels opaque, ready for SavePng. */
	JAMKIT_API void AnalyseImage(TArray<FColor>& Pixels, FIntPoint Size, FString& OutJson, FString& OutLogLine);

	JAMKIT_API bool SavePng(const TArray<FColor>& Pixels, FIntPoint Size, const FString& Path);
}
