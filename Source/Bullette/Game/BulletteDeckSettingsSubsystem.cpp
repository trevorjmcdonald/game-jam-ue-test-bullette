#include "Game/BulletteDeckSettingsSubsystem.h"

#include "Bullette.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Scalability.h"

bool UBulletteDeckSettingsSubsystem::IsRunningOnSteamDeck()
{
	return FPlatformMisc::GetEnvironmentVariable(TEXT("SteamDeck")) == TEXT("1")
		|| FParse::Param(FCommandLine::Get(), TEXT("SteamDeck"));
}

void UBulletteDeckSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (GIsEditor || !IsRunningOnSteamDeck())
	{
		return;
	}

	// Low scalability turns off Lumen GI/reflections and drops shadow, post-process and effects cost.
	// Keep textures at Medium: the Deck has the memory, and Low textures look muddy at 1280x800.
	Scalability::FQualityLevels Levels;
	Levels.SetFromSingleQualityLevel(0);
	Levels.TextureQuality = 1;
	Scalability::SetQualityLevels(Levels);

	UE_LOG(LogBullette, Display, TEXT("DeckProfile applied: scalability Low, textures Medium"));
}
