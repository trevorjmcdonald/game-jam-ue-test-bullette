#include "Feedback/JamKitHitFlashComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"

UJamKitHitFlashComponent::UJamKitHitFlashComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UJamKitHitFlashComponent::Flash()
{
	if (!bTargetsCreated)
	{
		CreateTargets();
	}
	if (Targets.IsEmpty())
	{
		return;
	}

	Elapsed = 0.0f;
	Apply(1.0f);
	SetComponentTickEnabled(true);
}

void UJamKitHitFlashComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	Elapsed += DeltaTime;
	const float Alpha = 1.0f - FMath::Clamp(Elapsed / Duration, 0.0f, 1.0f);
	Apply(Alpha);
	if (Alpha <= 0.0f)
	{
		SetComponentTickEnabled(false);
	}
}

void UJamKitHitFlashComponent::CreateTargets()
{
	bTargetsCreated = true;

	TArray<USkeletalMeshComponent*> Meshes;
	GetOwner()->GetComponents(Meshes);
	for (USkeletalMeshComponent* Mesh : Meshes)
	{
		if (!Mesh->IsVisible() || Mesh->bHiddenInGame)
		{
			continue;
		}
		for (int32 Index = 0; Index < Mesh->GetNumMaterials(); ++Index)
		{
			UMaterialInterface* Material = Mesh->GetMaterial(Index);
			FHashedMaterialParameterInfo Info(ParameterName);
			FLinearColor Original;
			// Skip materials without the parameter (or with nothing assigned).
			if (!Material || !Material->GetVectorParameterValue(Info, Original))
			{
				continue;
			}
			if (UMaterialInstanceDynamic* Dynamic = Mesh->CreateDynamicMaterialInstance(Index))
			{
				Targets.Add({ Dynamic, Original });
			}
		}
	}
}

void UJamKitHitFlashComponent::Apply(float Alpha)
{
	for (const FFlashTarget& Target : Targets)
	{
		if (UMaterialInstanceDynamic* Material = Target.Material.Get())
		{
			Material->SetVectorParameterValue(ParameterName, FMath::Lerp(Target.OriginalValue, FlashColor, Alpha));
		}
	}
}
