#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "JamKitSettings.generated.h"

class UInputMappingContext;
class USoundClass;
class USoundMix;
class UWorld;

/** A level for probe runs (see UJamKitProbeSubsystem). */
USTRUCT()
struct FJamKitProbeLevel
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Probe")
	TSoftObjectPtr<UWorld> Level;

	/** Names the folder the shots go in (Saved/Probe/<Label>) and picks the level on the command line; empty = the level's name. */
	UPROPERTY(EditAnywhere, Category = "Probe")
	FString Label;

	/** URL options for opening it, e.g. Arrival=Pryor. */
	UPROPERTY(EditAnywhere, Category = "Probe")
	FString Options;

	/** Console commands run before it opens (e.g. a week or a save state the level reads when it loads). */
	UPROPERTY(EditAnywhere, Category = "Probe")
	TArray<FString> Commands;

	FString GetLabel() const;
};

/** A moment to shoot every probe point at: console commands that set it up (time of day, weather, ...). */
USTRUCT()
struct FJamKitProbeMoment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Probe")
	FName Name;

	UPROPERTY(EditAnywhere, Category = "Probe")
	TArray<FString> Commands;
};

/**
 * Everything the kit needs to know about the game, in one place: Project Settings > Game > Jam Kit, saved to
 * DefaultGame.ini. The kit's widget assets reference nothing game-specific; they read these instead.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Jam Kit"))
class JAMKIT_API UJamKitSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UJamKitSettings();

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	/** Shown on the main menu (TitleText). */
	UPROPERTY(Config, EditAnywhere, Category = "Game")
	FText GameTitle;

	UPROPERTY(Config, EditAnywhere, Category = "Levels")
	TSoftObjectPtr<UWorld> MainMenuLevel;

	/** The main menu's Play button opens this. */
	UPROPERTY(Config, EditAnywhere, Category = "Levels")
	TSoftObjectPtr<UWorld> GameLevel;

	/** Fade to black before opening a level, and from black when one starts. 0 turns fades off. */
	UPROPERTY(Config, EditAnywhere, Category = "Levels", meta = (ClampMin = "0", Units = "s"))
	float FadeSeconds = 0.35f;

	/** When a game level starts (not the main menu), stay black (with a progress bar if it takes a moment) until the shaders the level needs
	 * have finished compiling in the background, for at most this long. Keeps the first-run shader stutter, worst
	 * under Proton on the Steam Deck, behind the fade instead of in play. 0 fades in straight away. */
	UPROPERTY(Config, EditAnywhere, Category = "Levels", meta = (ClampMin = "0", Units = "s"))
	float MaxShaderWaitSeconds = 10.0f;

	/** The options screen lists this context's player-mappable mappings (rows are grouped by display name). */
	UPROPERTY(Config, EditAnywhere, Category = "Input")
	TSoftObjectPtr<UInputMappingContext> RebindContext;

	/** One line each on the credits screen. Add a line whenever third-party assets go in. */
	UPROPERTY(Config, EditAnywhere, Category = "Credits")
	TArray<FString> Credits;

	/** Carries the volume overrides. Every sound must use MusicClass or EffectsClass (both children of MasterClass),
	 * or the volume sliders won't affect it. Defaults point at the kit's assets in /JamKit/Audio/Classes. */
	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	TSoftObjectPtr<USoundMix> VolumeMix;

	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	TSoftObjectPtr<USoundClass> MasterClass;

	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	TSoftObjectPtr<USoundClass> MusicClass;

	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	TSoftObjectPtr<USoundClass> EffectsClass;

	/** Console variables the Steam Deck profile sets after Low scalability (e.g. r.RayTracing.Enable=0, which needs
	 * r.RayTracing.EnableOnDemand=1 at startup). In DefaultGame.ini, one line: DeckConsoleVariables=(("r.RayTracing.Enable", "0"), ("r.VolumetricFog", "0")) */
	UPROPERTY(Config, EditAnywhere, Category = "Steam Deck")
	TMap<FString, FString> DeckConsoleVariables;

	/** Levels a probe run (-jamprobe) visits, in order; empty = GameLevel alone. */
	UPROPERTY(Config, EditAnywhere, Category = "Probe")
	TArray<FJamKitProbeLevel> ProbeLevels;

	/** Moments every probe point is shot at, in order; empty = as the level starts. */
	UPROPERTY(Config, EditAnywhere, Category = "Probe")
	TArray<FJamKitProbeMoment> ProbeMoments;

	/** After a level opens, before the first shot (streaming, shaders, the fade). */
	UPROPERTY(Config, EditAnywhere, Category = "Probe", meta = (ClampMin = "0", Units = "s"))
	float ProbeLevelSettleSeconds = 6.0f;

	/** After a moment's commands (Lumen and exposure catching up with new light). */
	UPROPERTY(Config, EditAnywhere, Category = "Probe", meta = (ClampMin = "0", Units = "s"))
	float ProbeMomentSettleSeconds = 4.0f;

	/** After moving to the next probe point. */
	UPROPERTY(Config, EditAnywhere, Category = "Probe", meta = (ClampMin = "0", Units = "s"))
	float ProbeViewSettleSeconds = 2.5f;
};
