#include "Feedback/JamKitCameraShake.h"

void UJamKitShakePattern::GetShakePatternInfoImpl(FCameraShakeInfo& OutInfo) const
{
	OutInfo.Duration = FCameraShakeDuration(Duration);
}

void UJamKitShakePattern::StartShakePatternImpl(const FCameraShakePatternStartParams& Params)
{
	Elapsed = 0.0f;
	Seed = FMath::FRandRange(0.0f, 1000.0f);
	bPlaying = true;
}

void UJamKitShakePattern::UpdateShakePatternImpl(const FCameraShakePatternUpdateParams& Params, FCameraShakePatternUpdateResult& OutResult)
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

bool UJamKitShakePattern::IsFinishedImpl() const
{
	return !bPlaying;
}

void UJamKitShakePattern::StopShakePatternImpl(const FCameraShakePatternStopParams& Params)
{
	bPlaying = false;
}

UJamKitCameraShake::UJamKitCameraShake(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bSingleInstance = true;
	SetRootShakePattern(ObjectInitializer.CreateDefaultSubobject<UJamKitShakePattern>(this, TEXT("RootShakePattern")));
}
