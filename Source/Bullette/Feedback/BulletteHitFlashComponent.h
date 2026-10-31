#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BulletteHitFlashComponent.generated.h"

class UMaterialInstanceDynamic;

/**
 * Flashes the owner's skeletal meshes by pushing a color into a material vector parameter and fading it back.
 * The default parameter is EmissiveFactor, which every glTF-imported material (KayKit, Quaternius, Meshy) has,
 * so no material needs editing. Dynamic material instances are made on the first flash; it only ticks while fading.
 */
UCLASS(ClassGroup = (Bullette), meta = (BlueprintSpawnableComponent))
class BULLETTE_API UBulletteHitFlashComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBulletteHitFlashComponent();

	/** Starts a flash in FlashColor, or restarts it if one is running. */
	UFUNCTION(BlueprintCallable, Category = "Feedback")
	void Flash();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Vector parameter on the mesh materials that the flash drives. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Feedback")
	FName ParameterName = TEXT("EmissiveFactor");

	/** Peak value written to the parameter. Above 1 for an emissive glow. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Feedback")
	FLinearColor FlashColor = FLinearColor(1.5f, 1.5f, 1.5f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Feedback", meta = (ClampMin = "0.01", Units = "s"))
	float Duration = 0.12f;

private:
	struct FFlashTarget
	{
		TWeakObjectPtr<UMaterialInstanceDynamic> Material;
		FLinearColor OriginalValue;
	};

	void CreateTargets();
	void Apply(float Alpha);

	TArray<FFlashTarget> Targets;
	bool bTargetsCreated = false;
	float Elapsed = 0.0f;
};
