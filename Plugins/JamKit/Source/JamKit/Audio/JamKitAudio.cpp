#include "Audio/JamKitAudio.h"

#include "JamKit.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Verify/JamKitTrace.h"

void JamKitAudio::PlaySound(const UObject* WorldContext, USoundBase* Sound, bool bIgnorePause, float PitchVariation)
{
	if (!Sound || !WorldContext)
	{
		return;
	}

	const float Pitch = PitchVariation > 0.0f ? FMath::FRandRange(1.0f - PitchVariation, 1.0f + PitchVariation) : 1.0f;
	UE_LOG(LogJamKit, Verbose, TEXT("PlaySound sound=%s ignorePause=%d"), *Sound->GetName(), bIgnorePause);
	UGameplayStatics::PlaySound2D(WorldContext, Sound, 1.0f, Pitch, 0.0f, nullptr, nullptr, bIgnorePause);
	JamKitTrace::Event(WorldContext, TEXT("sound"), Sound->GetName(), FJamKitTraceFields().Add(TEXT("vol"), 1.0).Add(TEXT("pitch"), Pitch).Add(TEXT("dur"), Sound->GetDuration()));
}

UAudioComponent* JamKitAudio::PlayMusic(const UObject* WorldContext, USoundBase* Music, float FadeInSeconds)
{
	if (!Music || !WorldContext)
	{
		return nullptr;
	}

	// Created rather than spawned so bIsUISound is set before the sound starts (it's read at play time).
	UAudioComponent* Component = UGameplayStatics::CreateSound2D(WorldContext, Music, 1.0f, 1.0f, 0.0f, nullptr, false, false);
	if (Component)
	{
		UE_LOG(LogJamKit, Display, TEXT("PlayMusic track=%s"), *Music->GetName());
		Component->bIsUISound = true;
		Component->FadeIn(FadeInSeconds);
		JamKitTrace::Event(WorldContext, TEXT("music"), Music->GetName());
	}
	return Component;
}
