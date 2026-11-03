#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "JamKitProbePoint.generated.h"

/**
 * A view a probe run (UJamKitProbeSubsystem, -jamprobe) photographs and measures, so nobody has to stand there:
 * place it in a level and aim it like any camera (Pilot it in the editor). Invisible in game.
 */
UCLASS()
class JAMKIT_API AJamKitProbePoint : public ACameraActor
{
	GENERATED_BODY()

public:
	AJamKitProbePoint(const FObjectInitializer& ObjectInitializer);

	/** Names the shot (Saved/Probe/<Level>/<ProbeName>_<Moment>.png) and picks it on the command line; None = the actor's name. */
	UPROPERTY(EditAnywhere, Category = "Probe")
	FName ProbeName;

	/** Only these moments (the Jam Kit settings' ProbeMoments); empty = all of them. */
	UPROPERTY(EditAnywhere, Category = "Probe")
	TArray<FName> Moments;

	/** Use the player camera's field of view rather than this camera's own. */
	UPROPERTY(EditAnywhere, Category = "Probe")
	bool bMatchPlayerFOV = true;

	FName GetProbeName() const { return ProbeName.IsNone() ? GetFName() : ProbeName; }
};
