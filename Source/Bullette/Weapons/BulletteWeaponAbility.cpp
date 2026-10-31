#include "Weapons/BulletteWeaponAbility.h"

#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystem/BulletteCombatSet.h"
#include "AbilitySystem/BulletteDamage.h"
#include "AbilitySystemComponent.h"
#include "Audio/BulletteAudio.h"
#include "GameplayEffect.h"
#include "Sound/SoundBase.h"

UBulletteWeaponAbility::UBulletteWeaponAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UBulletteWeaponAbility::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);

	// Weapons are always on: start as soon as there's someone to fire from.
	if (ActorInfo && ActorInfo->AvatarActor.IsValid() && !Spec.IsActive())
	{
		ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle);
	}
}

void UBulletteWeaponAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	StartWeapon();
}

void UBulletteWeaponAbility::StartWeapon()
{
	WaitForNextAttack();
}

void UBulletteWeaponAbility::WaitForNextAttack()
{
	// WaitDelay runs on the world timer, so it stops while the level-up screen pauses the game.
	const float Seconds = FMath::Max(GetStat(Interval) / GetAttackSpeed(), 0.05f);
	UAbilityTask_WaitDelay* Wait = UAbilityTask_WaitDelay::WaitDelay(this, Seconds);
	Wait->OnFinish.AddDynamic(this, &UBulletteWeaponAbility::HandleAttackTimer);
	Wait->ReadyForActivation();
}

void UBulletteWeaponAbility::HandleAttackTimer()
{
	if (!IsActive())
	{
		return;
	}

	Fire();
	WaitForNextAttack();
}

void UBulletteWeaponAbility::PlayFireSound() const
{
	BulletteAudio::PlaySound(GetAvatarActorFromActorInfo(), FireSound);
}

float UBulletteWeaponAbility::GetStat(const FScalableFloat& Stat) const
{
	return Stat.GetValueAtLevel(static_cast<float>(GetAbilityLevel()));
}

float UBulletteWeaponAbility::GetDamage() const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const float Scale = ASC ? ASC->GetNumericAttribute(UBulletteCombatSet::GetDamageScaleAttribute()) : 1.0f;
	return GetStat(Damage) * Scale;
}

float UBulletteWeaponAbility::GetAttackSpeed() const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	return ASC ? FMath::Max(ASC->GetNumericAttribute(UBulletteCombatSet::GetAttackSpeedAttribute()), 0.1f) : 1.0f;
}

int32 UBulletteWeaponAbility::GetCount() const
{
	return FMath::Max(FMath::RoundToInt(GetStat(Count)), 1);
}

void UBulletteWeaponAbility::DealDamage(AActor* Target, float Amount) const
{
	BulletteDamage::ApplyDamage(GetAbilitySystemComponentFromActorInfo(), Target, DamageEffect, Amount);
}
