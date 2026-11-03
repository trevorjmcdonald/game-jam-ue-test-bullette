#include "Feedback/JamKitFeedbackSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "NiagaraFunctionLibrary.h"

UJamKitFeedbackSubsystem* UJamKitFeedbackSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UJamKitFeedbackSubsystem>() : nullptr;
}

void UJamKitFeedbackSubsystem::AddDamageNumber(const FVector& Location, float Amount, bool bPlayerDamage)
{
	if (Amount <= 0.0f)
	{
		return;
	}
	if (DamageNumbers.Num() >= MaxDamageNumbers)
	{
		DamageNumbers.RemoveAt(0, DamageNumbers.Num() - MaxDamageNumbers + 1, EAllowShrinking::No);
	}

	FJamKitDamageNumber& Number = DamageNumbers.AddDefaulted_GetRef();
	Number.Location = Location;
	Number.Amount = Amount;
	Number.SpawnTime = GetWorld()->GetTimeSeconds();
	Number.Jitter = FVector2f(FMath::FRandRange(-18.0f, 18.0f), FMath::FRandRange(-8.0f, 8.0f));
	Number.bPlayerDamage = bPlayerDamage;
}

void UJamKitFeedbackSubsystem::RemoveDamageNumbersBefore(double Time)
{
	// Numbers are added in time order, so the expired ones are at the front.
	int32 Expired = 0;
	while (Expired < DamageNumbers.Num() && DamageNumbers[Expired].SpawnTime < Time)
	{
		++Expired;
	}
	if (Expired > 0)
	{
		DamageNumbers.RemoveAt(0, Expired, EAllowShrinking::No);
	}
}

void JamKitFeedback::SpawnEffect(const UObject* WorldContext, UNiagaraSystem* System, const FVector& Location)
{
	if (System && WorldContext)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(WorldContext, System, Location, FRotator::ZeroRotator, FVector::OneVector,
			/*bAutoDestroy*/ true, /*bAutoActivate*/ true, ENCPoolMethod::AutoRelease, /*bPreCullCheck*/ true);
	}
}
