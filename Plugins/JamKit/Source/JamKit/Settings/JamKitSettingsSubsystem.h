#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "JamKitSettingsSubsystem.generated.h"

class UJamKitSaveGame;

UENUM(BlueprintType)
enum class EJamKitVolume : uint8
{
	Master,
	Music,
	Effects,
};

/**
 * Loads the player's settings when the game starts (save slot "<Project>Settings") and owns them.
 * Volumes are applied as overrides in UJamKitSettings::VolumeMix on the sound classes. The kit's player controllers
 * call ApplyVolumes in BeginPlay, so every level starts with the saved volumes.
 */
UCLASS()
class JAMKIT_API UJamKitSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UJamKitSettingsSubsystem* Get(const UObject* WorldContext);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	float GetVolume(EJamKitVolume Volume) const;

	/** Sets and applies a volume (0-1). Call SaveSettings to keep it. */
	void SetVolume(EJamKitVolume Volume, float Value);

	void ApplyVolumes() const;

	/** "Invert look up/down" for the controller (bGamepad) or the mouse. Read every frame by UJamKitInputModifierInvertLook. */
	bool GetInvertLook(bool bGamepad) const;

	/** Call SaveSettings to keep it. */
	void SetInvertLook(bool bGamepad, bool bInvert);

	/** Writes the save game (asynchronously). */
	void SaveSettings();

private:
	float& VolumeRef(EJamKitVolume Volume) const;
	static FString GetSlotName();

	UPROPERTY()
	TObjectPtr<UJamKitSaveGame> SaveGame;
};
