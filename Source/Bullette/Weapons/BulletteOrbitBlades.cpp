#include "Weapons/BulletteOrbitBlades.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Enemies/BulletteEnemy.h"
#include "Enemies/BulletteEnemySubsystem.h"
#include "Engine/World.h"
#include "Weapons/BulletteOrbitAbility.h"

ABulletteOrbitBlades::ABulletteOrbitBlades()
{
	PrimaryActorTick.bCanEverTick = true;
	// Follow the player after it has moved this frame.
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ABulletteOrbitBlades::Init(UBulletteOrbitAbility* InAbility, AActor* InCenter)
{
	Ability = InAbility;
	Center = InCenter;
	SetBladeCount(InAbility->GetCount());
}

void ABulletteOrbitBlades::SetBladeCount(int32 NewCount)
{
	const UBulletteOrbitAbility* OrbitAbility = Ability.Get();
	if (!OrbitAbility)
	{
		return;
	}

	while (BladeMeshes.Num() > NewCount)
	{
		BladeMeshes.Pop()->DestroyComponent();
	}

	while (BladeMeshes.Num() < NewCount)
	{
		UStaticMeshComponent* Blade = NewObject<UStaticMeshComponent>(this);
		Blade->SetStaticMesh(OrbitAbility->GetBladeMesh());
		Blade->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Blade->SetCanEverAffectNavigation(false);
		Blade->SetupAttachment(RootComponent);
		Blade->RegisterComponent();
		BladeMeshes.Add(Blade);
	}
}

void ABulletteOrbitBlades::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const UBulletteOrbitAbility* OrbitAbility = Ability.Get();
	const AActor* CenterActor = Center.Get();
	if (!OrbitAbility || !CenterActor)
	{
		Destroy();
		return;
	}

	SetActorLocation(CenterActor->GetActorLocation());
	Angle = FMath::Fmod(Angle + OrbitAbility->GetOrbitSpeed() * DeltaSeconds, 360.0);

	const UBulletteEnemySubsystem* Enemies = GetWorld()->GetSubsystem<UBulletteEnemySubsystem>();
	const double Now = GetWorld()->GetTimeSeconds();
	const double HitInterval = OrbitAbility->GetHitInterval();
	const float Radius = OrbitAbility->GetOrbitRadius();

	// Each blade hits like a spoke from the player out to the blade, not just at the blade itself:
	// enemies that reach the player stack on its centre, inside the orbit.
	const FVector Hub(GetActorLocation().X, GetActorLocation().Y, 0.0);
	const float HitRadius = OrbitAbility->GetHitRadius();
	TArray<ABulletteEnemy*> Nearby;
	if (Enemies)
	{
		Enemies->FindEnemiesInRadius(GetActorLocation(), Radius + HitRadius, Nearby);
	}

	TArray<AActor*> ToHit;
	for (int32 Index = 0; Index < BladeMeshes.Num(); ++Index)
	{
		const double BladeAngle = Angle + 360.0 * Index / BladeMeshes.Num();
		const FVector Offset = FRotator(0.0, BladeAngle, 0.0).RotateVector(FVector(Radius, 0.0, 0.0));

		// Orbit point at Offset, facing the direction of travel (counter-clockwise seen from above is +yaw).
		const FTransform OrbitPoint(FRotator(0.0, BladeAngle + 90.0, 0.0), Offset);
		BladeMeshes[Index]->SetRelativeTransform(OrbitAbility->GetBladeTransform() * OrbitPoint);

		for (ABulletteEnemy* Enemy : Nearby)
		{
			const FVector EnemyPoint(Enemy->GetActorLocation().X, Enemy->GetActorLocation().Y, 0.0);
			if (FMath::PointDistToSegment(EnemyPoint, Hub, Hub + Offset) > HitRadius + Enemy->GetCollisionRadius())
			{
				continue;
			}

			double& LastHit = LastHitTimes.FindOrAdd(Enemy, -1.0e9);
			if (Now - LastHit >= HitInterval)
			{
				LastHit = Now;
				ToHit.Add(Enemy);
			}
		}
	}

	// Damage after the loop: a kill unregisters the enemy from the subsystem.
	const float HitDamage = OrbitAbility->GetDamage();
	for (AActor* Enemy : ToHit)
	{
		OrbitAbility->DealDamage(Enemy, HitDamage);
	}

	if (LastHitTimes.Num() > 256)
	{
		for (auto It = LastHitTimes.CreateIterator(); It; ++It)
		{
			if (!It->Key.IsValid() || Now - It->Value > HitInterval)
			{
				It.RemoveCurrent();
			}
		}
	}
}
