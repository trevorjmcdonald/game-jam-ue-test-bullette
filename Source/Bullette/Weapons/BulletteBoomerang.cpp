#include "Weapons/BulletteBoomerang.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Enemies/BulletteEnemy.h"
#include "Enemies/BulletteEnemySubsystem.h"
#include "Engine/World.h"
#include "Weapons/BulletteWeaponAbility.h"

ABulletteBoomerang::ABulletteBoomerang()
{
	PrimaryActorTick.bCanEverTick = true;
	// Safety net in case the player is gone and the axe never arrives.
	InitialLifeSpan = 6.0f;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Pivot = CreateDefaultSubobject<USceneComponent>(TEXT("Pivot"));
	Pivot->SetupAttachment(RootComponent);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Pivot);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCanEverAffectNavigation(false);
}

ABulletteBoomerang* ABulletteBoomerang::Throw(UWorld* World, const FVector& From, const FBulletteBoomerangParams& Params)
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ABulletteBoomerang* Axe = World ? World->SpawnActor<ABulletteBoomerang>(From, FRotator::ZeroRotator, SpawnParams) : nullptr;
	if (Axe)
	{
		Axe->Params = Params;
		Axe->Mesh->SetStaticMesh(Params.Mesh);
		Axe->Mesh->SetRelativeTransform(Params.MeshTransform);
	}
	return Axe;
}

void ABulletteBoomerang::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Pivot->AddLocalRotation(FRotator(0.0, Params.SpinSpeed * DeltaSeconds, 0.0));

	const double Step = Params.Speed * DeltaSeconds;
	if (!bReturning)
	{
		SetActorLocation(GetActorLocation() + Params.Direction * Step);
		Travelled += Step;
		if (Travelled >= Params.Distance)
		{
			bReturning = true;
			HitThisLeg.Reset();
		}
	}
	else
	{
		const AActor* Thrower = Params.ReturnTo.Get();
		if (!Thrower)
		{
			Destroy();
			return;
		}

		// Come back a little faster than it went out, so it catches up with a running player.
		FVector ToThrower = Thrower->GetActorLocation() - GetActorLocation();
		ToThrower.Z = 0.0;
		const double Remaining = ToThrower.Size();
		const double ReturnStep = Step * 1.25;
		if (Remaining <= ReturnStep + 60.0)
		{
			Destroy();
			return;
		}
		SetActorLocation(GetActorLocation() + ToThrower / Remaining * ReturnStep);
	}

	HitEnemiesInReach();
}

void ABulletteBoomerang::HitEnemiesInReach()
{
	const UBulletteWeaponAbility* Weapon = Params.Weapon.Get();
	const UBulletteEnemySubsystem* Enemies = GetWorld()->GetSubsystem<UBulletteEnemySubsystem>();
	if (!Weapon || !Enemies)
	{
		return;
	}

	TArray<ABulletteEnemy*> InReach;
	Enemies->FindEnemiesInRadius(GetActorLocation(), Params.HitRadius, InReach);
	for (ABulletteEnemy* Enemy : InReach)
	{
		bool bAlreadyHit = false;
		HitThisLeg.Add(Enemy, &bAlreadyHit);
		if (!bAlreadyHit)
		{
			Weapon->DealDamage(Enemy, Params.Damage);
		}
	}
}
