// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBullette, Log, All);

// Custom collision channels. Names and default responses (Ignore) are defined in Config/DefaultEngine.ini.
#define ECC_BulletteEnemy ECC_GameTraceChannel1      // Object channel: enemy bodies
#define ECC_BulletteProjectile ECC_GameTraceChannel2 // Trace channel: projectile sweeps

