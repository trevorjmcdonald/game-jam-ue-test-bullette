#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BulletteSettingsSubsystem.generated.h"

class UBulletteSaveGame;

UENUM(BlueprintType)
enum class EBulletteVolume : uint8
{
	Master,
	Music,
	Effects,
};

/**
 * Loads the save game when the game starts and owns it: volume settings and the best run.
 * Volumes are applied as overrides in UBulletteAudioSettings::VolumeMix on the sound classes. The player controllers
 * call ApplyVolumes in BeginPlay, so every level starts with the saved volumes.
 */
UCLASS()
class BULLETTE_API UBulletteSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UBulletteSettingsSubsystem* Get(const UObject* WorldContext);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	float GetVolume(EBulletteVolume Volume) const;

	/** Sets and applies a volume (0-1). Call SaveSettings to keep it. */
	void SetVolume(EBulletteVolume Volume, float Value);

	void ApplyVolumes() const;

	/** Writes the save game (asynchronously). */
	void SaveSettings();

	/** Records a finished run and saves. Returns true if it's the new best (longest survival). */
	bool RecordRun(float SurvivedSeconds, int32 Kills);

	bool HasBestRun() const;
	float GetBestSurvivedSeconds() const;
	int32 GetBestKills() const;

private:
	float& VolumeRef(EBulletteVolume Volume) const;

	UPROPERTY()
	TObjectPtr<UBulletteSaveGame> SaveGame;
};
