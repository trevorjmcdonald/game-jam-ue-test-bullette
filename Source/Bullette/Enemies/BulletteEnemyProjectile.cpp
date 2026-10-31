#include "Enemies/BulletteEnemyProjectile.h"

#include "AbilitySystem/BulletteDamage.h"
#include "AbilitySystemComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameplayEffect.h"
#include "UObject/ConstructorHelpers.h"

ABulletteEnemyProjectile::ABulletteEnemyProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	InitialLifeSpan = 4.0f;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	PlaceholderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderMesh"));
	PlaceholderMesh->SetStaticMesh(SphereMesh.Object);
	PlaceholderMesh->SetRelativeScale3D(FVector(0.3f));
	PlaceholderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlaceholderMesh->SetCanEverAffectNavigation(false);
	PlaceholderMesh->SetCastShadow(false);
	RootComponent = PlaceholderMesh;
}

void ABulletteEnemyProjectile::Init(UAbilitySystemComponent* InSourceASC, TSubclassOf<UGameplayEffect> InDamageEffect, float InDamage, AActor* InTarget)
{
	SourceASC = InSourceASC;
	DamageEffect = InDamageEffect;
	Damage = InDamage;
	Target = InTarget;
}

void ABulletteEnemyProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	SetActorLocation(GetActorLocation() + GetActorForwardVector() * Speed * DeltaSeconds);

	AActor* TargetActor = Target.Get();
	if (!TargetActor)
	{
		return;
	}

	const float Reach = HitRadius + TargetActor->GetSimpleCollisionRadius();
	if (FVector::DistSquared2D(GetActorLocation(), TargetActor->GetActorLocation()) <= Reach * Reach)
	{
		BulletteDamage::ApplyDamage(SourceASC.Get(), TargetActor, DamageEffect, Damage);
		Destroy();
	}
}
