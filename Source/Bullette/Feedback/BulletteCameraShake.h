#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraShakeBase.h"
#include "BulletteCameraShake.generated.h"

/**
 * A short, decaying jolt: Perlin noise on pitch, yaw and roll that fades out over Duration.
 * Rotation rather than location, because the top-down camera sits 16 m away and small moves barely show.
 */
UCLASS()
class BULLETTE_API UBulletteShakePattern : public UCameraShakePattern
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Shake", meta = (ClampMin = "0.01", Units = "s"))
	float Duration = 0.25f;

	/** Peak rotation in degrees, before the play scale. */
	UPROPERTY(EditAnywhere, Category = "Shake", meta = (ClampMin = "0"))
	float RotationAmplitude = 0.6f;

	/** Noise speed (cycles per second). */
	UPROPERTY(EditAnywhere, Category = "Shake", meta = (ClampMin = "0"))
	float Frequency = 25.0f;

protected:
	virtual void GetShakePatternInfoImpl(FCameraShakeInfo& OutInfo) const override;
	virtual void StartShakePatternImpl(const FCameraShakePatternStartParams& Params) override;
	virtual void UpdateShakePatternImpl(const FCameraShakePatternUpdateParams& Params, FCameraShakePatternUpdateResult& OutResult) override;
	virtual bool IsFinishedImpl() const override;
	virtual void StopShakePatternImpl(const FCameraShakePatternStopParams& Params) override;

private:
	float Elapsed = 0.0f;
	float Seed = 0.0f;
	bool bPlaying = false;
};

/** The game's camera shake. Play it with a scale: 1 for a hit, more for bigger moments. Single instance, so hits restart it. */
UCLASS()
class BULLETTE_API UBulletteCameraShake : public UCameraShakeBase
{
	GENERATED_BODY()

public:
	UBulletteCameraShake(const FObjectInitializer& ObjectInitializer);
};
