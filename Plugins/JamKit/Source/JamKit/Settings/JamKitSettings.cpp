#include "Settings/JamKitSettings.h"

#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

UJamKitSettings::UJamKitSettings()
{
	VolumeMix = TSoftObjectPtr<USoundMix>(FSoftObjectPath(TEXT("/JamKit/Audio/Classes/MIX_Volume.MIX_Volume")));
	MasterClass = TSoftObjectPtr<USoundClass>(FSoftObjectPath(TEXT("/JamKit/Audio/Classes/SC_Master.SC_Master")));
	MusicClass = TSoftObjectPtr<USoundClass>(FSoftObjectPath(TEXT("/JamKit/Audio/Classes/SC_Music.SC_Music")));
	EffectsClass = TSoftObjectPtr<USoundClass>(FSoftObjectPath(TEXT("/JamKit/Audio/Classes/SC_Effects.SC_Effects")));
}

FString FJamKitProbeLevel::GetLabel() const
{
	return Label.IsEmpty() ? Level.GetAssetName() : Label;
}
