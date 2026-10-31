#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BulletteBoomerang.generated.h"

class UBulletteWeaponAbility;
class UStaticMesh;
class UStaticMeshComponent;

struct FBulletteBoomerangParams
{
	TWeakObjectPtr<const UBulletteWeaponAbility> Weapon;
	TWeakObjectPtr<AActor> ReturnTo;
	FVector Direction = FVector::ForwardVector;
	float Damage = 0.0f;
	float Distance = 700.0f;
	float Speed = 1400.0f;
	float HitRadius = 70.0f;
	UStaticMesh* Mesh = nullptr;
	FTransform MeshTransform;
	float SpinSpeed = 900.0f;
};

/**
 * One thrown axe from UBulletteBoomerangAbility. Flies straight out for Distance, then homes back to the player
 * and disappears on arrival. Pierces: every enemy it passes is hit once per leg.
 */
UCLASS(NotBlueprintable)
class BULLETTE_API ABulletteBoomerang : public AActor
{
	GENERATED_BODY()

public:
	ABulletteBoomerang();

	static ABulletteBoomerang* Throw(UWorld* World, const FVector& From, const FBulletteBoomerangParams& Params);

	virtual void Tick(float DeltaSeconds) override;

private:
	void HitEnemiesInReach();

	UPROPERTY()
	TObjectPtr<USceneComponent> Pivot;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Mesh;

	FBulletteBoomerangParams Params;
	double Travelled = 0.0;
	bool bReturning = false;

	/** Enemies already hit on the current leg. */
	TSet<TWeakObjectPtr<AActor>> HitThisLeg;
};
