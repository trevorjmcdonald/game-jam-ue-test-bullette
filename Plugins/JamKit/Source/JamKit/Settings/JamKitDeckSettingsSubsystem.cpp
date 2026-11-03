#include "Settings/JamKitDeckSettingsSubsystem.h"

#include "JamKit.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/IConsoleManager.h"
#include "Scalability.h"
#include "Settings/JamKitSettings.h"

bool UJamKitDeckSettingsSubsystem::IsRunningOnSteamDeck()
{
	return FPlatformMisc::GetEnvironmentVariable(TEXT("SteamDeck")) == TEXT("1")
		|| FParse::Param(FCommandLine::Get(), TEXT("SteamDeck"));
}

void UJamKitDeckSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
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

	UE_LOG(LogJamKit, Display, TEXT("DeckProfile applied: scalability Low, textures Medium"));

	for (const TPair<FString, FString>& Pair : GetDefault<UJamKitSettings>()->DeckConsoleVariables)
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(*Pair.Key))
		{
			CVar->Set(*Pair.Value, ECVF_SetByGameSetting);
			UE_LOG(LogJamKit, Display, TEXT("DeckProfile: %s = %s"), *Pair.Key, *Pair.Value);
		}
		else
		{
			UE_LOG(LogJamKit, Warning, TEXT("DeckProfile: no console variable %s"), *Pair.Key);
		}
	}
}
