#include "Weapons/BulletteProjectile.h"

#include "AbilitySystem/BulletteDamage.h"
#include "AbilitySystemComponent.h"
#include "Bullette.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "UObject/ConstructorHelpers.h"

ABulletteProjectile::ABulletteProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	InitialLifeSpan = 2.0f;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	// Hits come from the sweep in Tick, so the mesh itself never collides.
	PlaceholderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderMesh"));
	PlaceholderMesh->SetStaticMesh(SphereMesh.Object);
	PlaceholderMesh->SetRelativeScale3D(FVector(0.25f));
	PlaceholderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlaceholderMesh->SetCanEverAffectNavigation(false);
	PlaceholderMesh->SetCastShadow(false);
	RootComponent = PlaceholderMesh;
}

void ABulletteProjectile::InitDamage(UAbilitySystemComponent* InSourceASC, TSubclassOf<UGameplayEffect> InDamageEffect, float InDamage)
{
	SourceASC = InSourceASC;
	DamageEffect = InDamageEffect;
	Damage = InDamage;
}

void ABulletteProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const FVector Start = GetActorLocation();
	const FVector End = Start + GetActorForwardVector() * Speed * DeltaSeconds;

	FHitResult Hit;
	const bool bHit = GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_BulletteProjectile, FCollisionShape::MakeSphere(CollisionRadius));
	if (bHit && Hit.GetActor())
	{
		BulletteDamage::ApplyDamage(SourceASC.Get(), Hit.GetActor(), DamageEffect, Damage);
		Destroy();
		return;
	}

	SetActorLocation(End);
}
