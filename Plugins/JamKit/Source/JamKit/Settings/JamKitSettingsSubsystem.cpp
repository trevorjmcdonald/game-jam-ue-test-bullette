#include "Settings/JamKitSettingsSubsystem.h"

#include "JamKit.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Settings/JamKitSaveGame.h"
#include "Settings/JamKitSettings.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

namespace
{
	constexpr int32 SaveUserIndex = 0;
}

UJamKitSettingsSubsystem* UJamKitSettingsSubsystem::Get(const UObject* WorldContext)
{
	const UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContext);
	return GameInstance ? GameInstance->GetSubsystem<UJamKitSettingsSubsystem>() : nullptr;
}

FString UJamKitSettingsSubsystem::GetSlotName()
{
	return FString(FApp::GetProjectName()) + TEXT("Settings");
}

void UJamKitSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UGameplayStatics::DoesSaveGameExist(GetSlotName(), SaveUserIndex))
	{
		SaveGame = Cast<UJamKitSaveGame>(UGameplayStatics::LoadGameFromSlot(GetSlotName(), SaveUserIndex));
	}
	if (!SaveGame)
	{
		SaveGame = Cast<UJamKitSaveGame>(UGameplayStatics::CreateSaveGameObject(UJamKitSaveGame::StaticClass()));
	}
	UE_LOG(LogJamKit, Display, TEXT("Settings loaded master=%.2f music=%.2f effects=%.2f"),
		SaveGame->MasterVolume, SaveGame->MusicVolume, SaveGame->EffectsVolume);
}

float& UJamKitSettingsSubsystem::VolumeRef(EJamKitVolume Volume) const
{
	switch (Volume)
	{
	case EJamKitVolume::Music: return SaveGame->MusicVolume;
	case EJamKitVolume::Effects: return SaveGame->EffectsVolume;
	default: return SaveGame->MasterVolume;
	}
}

float UJamKitSettingsSubsystem::GetVolume(EJamKitVolume Volume) const
{
	return VolumeRef(Volume);
}

void UJamKitSettingsSubsystem::SetVolume(EJamKitVolume Volume, float Value)
{
	VolumeRef(Volume) = FMath::Clamp(Value, 0.0f, 1.0f);
	ApplyVolumes();
}

void UJamKitSettingsSubsystem::ApplyVolumes() const
{
	const UJamKitSettings* Settings = GetDefault<UJamKitSettings>();
	USoundMix* Mix = Settings->VolumeMix.LoadSynchronous();
	const UGameInstance* GameInstance = GetGameInstance();
	if (!Mix || !GameInstance || !GameInstance->GetWorld())
	{
		return;
	}

	// Master applies to its children too; the audio device multiplies a child's own override in.
	const TPair<const TSoftObjectPtr<USoundClass>*, float> Overrides[] = {
		{ &Settings->MasterClass, SaveGame->MasterVolume },
		{ &Settings->MusicClass, SaveGame->MusicVolume },
		{ &Settings->EffectsClass, SaveGame->EffectsVolume },
	};
	for (const TPair<const TSoftObjectPtr<USoundClass>*, float>& Override : Overrides)
	{
		if (USoundClass* SoundClass = Override.Key->LoadSynchronous())
		{
			UGameplayStatics::SetSoundMixClassOverride(GameInstance, Mix, SoundClass, Override.Value, 1.0f, 0.0f, true);
		}
	}
	UGameplayStatics::PushSoundMixModifier(GameInstance, Mix);
}

bool UJamKitSettingsSubsystem::GetInvertLook(bool bGamepad) const
{
	return SaveGame && (bGamepad ? SaveGame->bInvertLookGamepad : SaveGame->bInvertLookMouse);
}

void UJamKitSettingsSubsystem::SetInvertLook(bool bGamepad, bool bInvert)
{
	(bGamepad ? SaveGame->bInvertLookGamepad : SaveGame->bInvertLookMouse) = bInvert;
}

void UJamKitSettingsSubsystem::SaveSettings()
{
	UGameplayStatics::AsyncSaveGameToSlot(SaveGame, GetSlotName(), SaveUserIndex);
}
