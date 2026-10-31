#include "Audio/BulletteAudio.h"

#include "Bullette.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

void BulletteAudio::PlaySound(const UObject* WorldContext, USoundBase* Sound, bool bIgnorePause, float PitchVariation)
{
	if (!Sound || !WorldContext)
	{
		return;
	}

	const float Pitch = PitchVariation > 0.0f ? FMath::FRandRange(1.0f - PitchVariation, 1.0f + PitchVariation) : 1.0f;
	UE_LOG(LogBullette, Verbose, TEXT("PlaySound sound=%s ignorePause=%d"), *Sound->GetName(), bIgnorePause);
	UGameplayStatics::PlaySound2D(WorldContext, Sound, 1.0f, Pitch, 0.0f, nullptr, nullptr, bIgnorePause);
}

UAudioComponent* BulletteAudio::PlayMusic(const UObject* WorldContext, USoundBase* Music, float FadeInSeconds)
{
	if (!Music || !WorldContext)
	{
		return nullptr;
	}

	// Created rather than spawned so bIsUISound is set before the sound starts (it's read at play time).
	UAudioComponent* Component = UGameplayStatics::CreateSound2D(WorldContext, Music, 1.0f, 1.0f, 0.0f, nullptr, false, false);
	if (Component)
	{
		UE_LOG(LogBullette, Display, TEXT("PlayMusic track=%s"), *Music->GetName());
		Component->bIsUISound = true;
		Component->FadeIn(FadeInSeconds);
	}
	return Component;
}
