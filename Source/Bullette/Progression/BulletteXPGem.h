#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BulletteXPGem.generated.h"

class UAbilitySystemComponent;
class UStaticMeshComponent;

/**
 * Dropped by enemies. Sits still until the player is within their PickupRadius attribute,
 * then flies to the player and grants its XP. Uses distance checks, not collision.
 */
UCLASS(Abstract)
class BULLETTE_API ABulletteXPGem : public AActor
{
	GENERATED_BODY()

public:
	ABulletteXPGem();

	virtual void Tick(float DeltaSeconds) override;

	void SetXPValue(int32 InXPValue) { XPValue = InXPValue; }

protected:
	virtual void BeginPlay() override;

	/** Stand-in visual until real gem art exists. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Placeholder")
	TObjectPtr<UStaticMeshComponent> PlaceholderMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup", meta = (ClampMin = "0", Units = "cm/s"))
	float MagnetSpeed = 1200.0f;

	/** The gem is collected once it's this close to the player (XY). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup", meta = (ClampMin = "0", Units = "cm"))
	float CollectDistance = 60.0f;

private:
	int32 XPValue = 1;
	bool bMagnetized = false;
	TWeakObjectPtr<AActor> Player;
	TWeakObjectPtr<UAbilitySystemComponent> PlayerASC;
};
