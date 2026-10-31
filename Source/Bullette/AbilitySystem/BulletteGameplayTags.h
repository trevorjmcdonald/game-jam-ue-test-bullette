#pragma once

#include "NativeGameplayTags.h"

namespace BulletteTags
{
	/** SetByCaller magnitude carrying the damage amount on damage effects. */
	BULLETTE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Damage);

	/** One chain lightning jump: EffectContext origin = start, Location = end. */
	BULLETTE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Weapon_ChainLightning);
}
