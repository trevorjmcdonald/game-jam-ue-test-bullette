#include "Verify/JamKitProbePoint.h"

#include "Camera/CameraComponent.h"

AJamKitProbePoint::AJamKitProbePoint(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Shot at whatever the window's shape is, like the game.
	GetCameraComponent()->SetConstraintAspectRatio(false);
}
