#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BulletteEnemyProjectile.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UStaticMeshComponent;

/**
 * Enemy shot: flies straight along its forward vector and damages the player when it comes within reach (XY distance
 * check, like the rest of the game's hits). Passes through everything else.
 */
UCLASS(Abstract)
class BULLETTE_API ABulletteEnemyProjectile : public AActor
{
	GENERATED_BODY()

public:
	ABulletteEnemyProjectile();

	void Init(UAbilitySystemComponent* InSourceASC, TSubclassOf<UGameplayEffect> InDamageEffect, float InDamage, AActor* InTarget);

	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Placeholder")
	TObjectPtr<UStaticMeshComponent> PlaceholderMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "0", Units = "cm/s"))
	float Speed = 700.0f;

	/** Added to the target's collision radius. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "1", Units = "cm"))
	float HitRadius = 20.0f;

private:
	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;
	TSubclassOf<UGameplayEffect> DamageEffect;
	TWeakObjectPtr<AActor> Target;
	float Damage = 0.0f;
};
