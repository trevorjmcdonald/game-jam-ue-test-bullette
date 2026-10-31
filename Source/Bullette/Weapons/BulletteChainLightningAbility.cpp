#include "Weapons/BulletteChainLightningAbility.h"

#include "AbilitySystemComponent.h"
#include "Enemies/BulletteEnemy.h"
#include "Enemies/BulletteEnemySubsystem.h"
#include "Engine/World.h"
#include "Weapons/BulletteBeam.h"

void UBulletteChainLightningAbility::Fire()
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	const UBulletteEnemySubsystem* Enemies = GetWorld()->GetSubsystem<UBulletteEnemySubsystem>();
	if (!Avatar || !Enemies)
	{
		return;
	}

	FVector From = Avatar->GetActorLocation();
	ABulletteEnemy* Target = Enemies->FindNearestEnemy(From, Range);
	const float HitDamage = GetDamage();
	const int32 Jumps = GetCount();

	TArray<const AActor*> Struck;
	for (int32 Strike = 0; Target && Strike <= Jumps; ++Strike)
	{
		const FVector To = Target->GetActorLocation();
		ShowBeam(From, To);
		Struck.Add(Target);

		// A kill unregisters the enemy, so look for the next one from where this one was.
		DealDamage(Target, HitDamage);
		From = To;
		Target = Enemies->FindNearestEnemy(From, JumpRange, Struck);
	}
}

void UBulletteChainLightningAbility::ShowBeam(const FVector& From, const FVector& To) const
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!BeamCue.IsValid() || !ASC)
	{
		ABulletteBeam::SpawnBeam(GetWorld(), From, To, BeamThickness, BeamMaterial, BeamDuration);
		return;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddOrigin(From);
	FGameplayCueParameters Parameters(Context);
	Parameters.Location = To;
	ASC->ExecuteGameplayCue(BeamCue, Parameters);
}
