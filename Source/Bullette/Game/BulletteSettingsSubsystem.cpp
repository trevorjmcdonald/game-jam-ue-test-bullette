#include "Game/BulletteSettingsSubsystem.h"

#include "Audio/BulletteAudioSettings.h"
#include "Bullette.h"
#include "Engine/GameInstance.h"
#include "Game/BulletteSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

namespace
{
	const FString SaveSlotName = TEXT("Bullette");
	constexpr int32 SaveUserIndex = 0;
}

UBulletteSettingsSubsystem* UBulletteSettingsSubsystem::Get(const UObject* WorldContext)
{
	const UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContext);
	return GameInstance ? GameInstance->GetSubsystem<UBulletteSettingsSubsystem>() : nullptr;
}

void UBulletteSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, SaveUserIndex))
	{
		SaveGame = Cast<UBulletteSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, SaveUserIndex));
	}
	if (!SaveGame)
	{
		SaveGame = Cast<UBulletteSaveGame>(UGameplayStatics::CreateSaveGameObject(UBulletteSaveGame::StaticClass()));
	}
	UE_LOG(LogBullette, Display, TEXT("Settings loaded master=%.2f music=%.2f effects=%.2f best=%.1fs"),
		SaveGame->MasterVolume, SaveGame->MusicVolume, SaveGame->EffectsVolume, SaveGame->BestSurvivedSeconds);
}

float& UBulletteSettingsSubsystem::VolumeRef(EBulletteVolume Volume) const
{
	switch (Volume)
	{
	case EBulletteVolume::Music: return SaveGame->MusicVolume;
	case EBulletteVolume::Effects: return SaveGame->EffectsVolume;
	default: return SaveGame->MasterVolume;
	}
}

float UBulletteSettingsSubsystem::GetVolume(EBulletteVolume Volume) const
{
	return VolumeRef(Volume);
}

void UBulletteSettingsSubsystem::SetVolume(EBulletteVolume Volume, float Value)
{
	VolumeRef(Volume) = FMath::Clamp(Value, 0.0f, 1.0f);
	ApplyVolumes();
}

void UBulletteSettingsSubsystem::ApplyVolumes() const
{
	const UBulletteAudioSettings* Settings = GetDefault<UBulletteAudioSettings>();
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

void UBulletteSettingsSubsystem::SaveSettings()
{
	UGameplayStatics::AsyncSaveGameToSlot(SaveGame, SaveSlotName, SaveUserIndex);
}

bool UBulletteSettingsSubsystem::RecordRun(float SurvivedSeconds, int32 Kills)
{
	const bool bNewBest = SurvivedSeconds > SaveGame->BestSurvivedSeconds;
	if (bNewBest)
	{
		SaveGame->BestSurvivedSeconds = SurvivedSeconds;
		SaveGame->BestKills = Kills;
		SaveSettings();
	}
	UE_LOG(LogBullette, Display, TEXT("RunRecorded survived_s=%.1f kills=%d new_best=%d"), SurvivedSeconds, Kills, bNewBest);
	return bNewBest;
}

bool UBulletteSettingsSubsystem::HasBestRun() const
{
	return SaveGame->BestSurvivedSeconds > 0.0f;
}

float UBulletteSettingsSubsystem::GetBestSurvivedSeconds() const
{
	return SaveGame->BestSurvivedSeconds;
}

int32 UBulletteSettingsSubsystem::GetBestKills() const
{
	return SaveGame->BestKills;
}
