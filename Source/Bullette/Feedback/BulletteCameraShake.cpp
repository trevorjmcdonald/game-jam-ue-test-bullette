#include "Feedback/BulletteCameraShake.h"

void UBulletteShakePattern::GetShakePatternInfoImpl(FCameraShakeInfo& OutInfo) const
{
	OutInfo.Duration = FCameraShakeDuration(Duration);
}

void UBulletteShakePattern::StartShakePatternImpl(const FCameraShakePatternStartParams& Params)
{
	Elapsed = 0.0f;
	Seed = FMath::FRandRange(0.0f, 1000.0f);
	bPlaying = true;
}

void UBulletteShakePattern::UpdateShakePatternImpl(const FCameraShakePatternUpdateParams& Params, FCameraShakePatternUpdateResult& OutResult)
{
	Elapsed += Params.DeltaTime;
	if (Elapsed >= Duration)
	{
		bPlaying = false;
		return;
	}

	// The shake base applies the play scale afterwards. Quadratic falloff: a sharp start that settles quickly.
	const float Falloff = FMath::Square(1.0f - Elapsed / Duration);
	const float T = Seed + Elapsed * Frequency;
	const float Amplitude = RotationAmplitude * Falloff;
	OutResult.Rotation.Pitch = Amplitude * FMath::PerlinNoise1D(T);
	OutResult.Rotation.Yaw = Amplitude * FMath::PerlinNoise1D(T + 31.7f);
	OutResult.Rotation.Roll = Amplitude * 1.5f * FMath::PerlinNoise1D(T + 77.3f);
}

bool UBulletteShakePattern::IsFinishedImpl() const
{
	return !bPlaying;
}

void UBulletteShakePattern::StopShakePatternImpl(const FCameraShakePatternStopParams& Params)
{
	bPlaying = false;
}

UBulletteCameraShake::UBulletteCameraShake(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bSingleInstance = true;
	SetRootShakePattern(ObjectInitializer.CreateDefaultSubobject<UBulletteShakePattern>(this, TEXT("RootShakePattern")));
}
