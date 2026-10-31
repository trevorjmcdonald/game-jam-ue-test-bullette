#include "Weapons/BulletteOrbitAbility.h"

#include "Engine/World.h"
#include "Weapons/BulletteOrbitBlades.h"

void UBulletteOrbitAbility::StartWeapon()
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Avatar;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Blades = GetWorld()->SpawnActor<ABulletteOrbitBlades>(Avatar->GetActorLocation(), FRotator::ZeroRotator, SpawnParams);
	if (Blades)
	{
		Blades->Init(this, Avatar);
	}
}

void UBulletteOrbitAbility::OnWeaponLevelChanged()
{
	if (Blades)
	{
		Blades->SetBladeCount(GetCount());
	}
}

void UBulletteOrbitAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (Blades)
	{
		Blades->Destroy();
		Blades = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
