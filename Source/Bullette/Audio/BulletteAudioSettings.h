#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "BulletteAudioSettings.generated.h"

class USoundClass;
class USoundMix;

/**
 * The sound classes the volume settings drive, and the mix that carries their volume overrides.
 * Every sound asset is assigned to MusicClass or EffectsClass; both are children of MasterClass.
 * Lives in DefaultGame.ini. These are only referenced from config, so /Game/Audio is in DirectoriesToAlwaysCook.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Bullette Audio"))
class UBulletteAudioSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	UPROPERTY(Config, EditAnywhere, Category = "Volume")
	TSoftObjectPtr<USoundMix> VolumeMix;

	UPROPERTY(Config, EditAnywhere, Category = "Volume")
	TSoftObjectPtr<USoundClass> MasterClass;

	UPROPERTY(Config, EditAnywhere, Category = "Volume")
	TSoftObjectPtr<USoundClass> MusicClass;

	UPROPERTY(Config, EditAnywhere, Category = "Volume")
	TSoftObjectPtr<USoundClass> EffectsClass;
};
