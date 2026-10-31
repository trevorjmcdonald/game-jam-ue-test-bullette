#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BulletteBeam.generated.h"

class UMaterialInterface;
class UStaticMeshComponent;

/** Placeholder line visual: a stretched cylinder from one point to another that disappears after a moment. */
UCLASS(NotBlueprintable)
class BULLETTE_API ABulletteBeam : public AActor
{
	GENERATED_BODY()

public:
	ABulletteBeam();

	static void SpawnBeam(UWorld* World, const FVector& From, const FVector& To, float Thickness, UMaterialInterface* Material, float Duration);

private:
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Mesh;
};
