#pragma once

#include "CoreMinimal.h"

class UAudioComponent;
class USoundBase;

/**
 * Sound helpers for non-spatial (2D) sounds: UI, music, and arcade-style effects. For positional sound use
 * UGameplayStatics::PlaySoundAtLocation or an audio component with attenuation.
 * Volume and concurrency limits live on the sound assets themselves (MaxCount/RetriggerTime overrides), which is what
 * keeps a crowd of enemies dying in one frame from stacking dozens of copies of the same sound.
 */
namespace JamKitAudio
{
	/**
	 * Plays a one-shot with a small random pitch shift so rapid repeats don't sound identical. Ignores null sounds.
	 * Gameplay sounds pause with the game. Pass bIgnorePause for sounds that start as the game pauses or while it's
	 * paused (level-up, menus); otherwise they'd only be heard after unpausing.
	 */
	JAMKIT_API void PlaySound(const UObject* WorldContext, USoundBase* Sound, bool bIgnorePause = false, float PitchVariation = 0.05f);

	/**
	 * Starts a music track and fades it in. Set looping on the asset. The track keeps playing while the game is
	 * paused and is destroyed with the level. Returns null for a null track.
	 */
	JAMKIT_API UAudioComponent* PlayMusic(const UObject* WorldContext, USoundBase* Music, float FadeInSeconds = 1.0f);
}
