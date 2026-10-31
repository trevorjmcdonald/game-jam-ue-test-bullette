#include "Weapons/BulletteBeam.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

ABulletteBeam::ABulletteBeam()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetStaticMesh(CylinderMesh.Object);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->SetCastShadow(false);
	RootComponent = Mesh;
}

void ABulletteBeam::SpawnBeam(UWorld* World, const FVector& From, const FVector& To, float Thickness, UMaterialInterface* Material, float Duration)
{
	const FVector Segment = To - From;
	const double Length = Segment.Size();
	if (!World || Length < UE_KINDA_SMALL_NUMBER)
	{
		return;
	}

	// The engine cylinder is 100 units tall along Z and 100 across, centred on its origin.
	const FRotator Rotation = FRotationMatrix::MakeFromZ(Segment / Length).Rotator();
	ABulletteBeam* Beam = World->SpawnActor<ABulletteBeam>(From + Segment * 0.5, Rotation);
	if (Beam)
	{
		Beam->SetActorScale3D(FVector(Thickness / 100.0, Thickness / 100.0, Length / 100.0));
		if (Material)
		{
			Beam->Mesh->SetMaterial(0, Material);
		}
		Beam->SetLifeSpan(Duration);
	}
}
