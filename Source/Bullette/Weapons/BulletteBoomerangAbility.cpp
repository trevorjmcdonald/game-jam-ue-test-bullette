#include "Weapons/BulletteBoomerangAbility.h"

#include "Enemies/BulletteEnemy.h"
#include "Enemies/BulletteEnemySubsystem.h"
#include "Engine/World.h"
#include "Weapons/BulletteBoomerang.h"

void UBulletteBoomerangAbility::Fire()
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	const UBulletteEnemySubsystem* Enemies = GetWorld()->GetSubsystem<UBulletteEnemySubsystem>();
	const ABulletteEnemy* Target = Enemies && Avatar ? Enemies->FindNearestEnemy(Avatar->GetActorLocation(), Range) : nullptr;
	if (!Target)
	{
		return;
	}

	PlayFireSound();

	FVector Aim = Target->GetActorLocation() - Avatar->GetActorLocation();
	Aim.Z = 0.0;
	const double AimYaw = Aim.Rotation().Yaw;

	FBulletteBoomerangParams Params;
	Params.Weapon = this;
	Params.ReturnTo = Avatar;
	Params.Damage = GetDamage();
	Params.Distance = GetStat(Distance);
	Params.Speed = Speed;
	Params.HitRadius = HitRadius;
	Params.Mesh = AxeMesh;
	Params.MeshTransform = AxeTransform;
	Params.SpinSpeed = SpinSpeed;

	const int32 NumAxes = GetCount();
	for (int32 Index = 0; Index < NumAxes; ++Index)
	{
		const double Yaw = AimYaw + (Index - (NumAxes - 1) * 0.5) * SpreadAngle;
		Params.Direction = FRotator(0.0, Yaw, 0.0).Vector();
		ABulletteBoomerang::Throw(GetWorld(), Avatar->GetActorLocation(), Params);
	}
}
