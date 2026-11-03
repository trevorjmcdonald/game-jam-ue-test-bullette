#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "JamKitFeedbackSubsystem.generated.h"

class UNiagaraSystem;

struct FJamKitDamageNumber
{
	FVector Location = FVector::ZeroVector;
	float Amount = 0.0f;
	double SpawnTime = 0.0;
	/** Small random screen offset, so numbers from a crowd don't stack on one spot. */
	FVector2f Jitter = FVector2f::ZeroVector;
	bool bPlayerDamage = false;
};

/**
 * Per-world queue of damage numbers. Gameplay adds them; UJamKitDamageNumbersWidget draws and expires them.
 * Keeping them as plain data (no widget or actor per number) makes hundreds of hits a second cheap.
 */
UCLASS()
class JAMKIT_API UJamKitFeedbackSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UJamKitFeedbackSubsystem* Get(const UObject* WorldContext);

	void AddDamageNumber(const FVector& Location, float Amount, bool bPlayerDamage);
	const TArray<FJamKitDamageNumber>& GetDamageNumbers() const { return DamageNumbers; }
	void RemoveDamageNumbersBefore(double Time);

	/** The oldest numbers are dropped past this many. */
	static constexpr int32 MaxDamageNumbers = 120;

private:
	TArray<FJamKitDamageNumber> DamageNumbers;
};

namespace JamKitFeedback
{
	/** Fire-and-forget Niagara burst at Location, pooled. Does nothing if System is null. */
	JAMKIT_API void SpawnEffect(const UObject* WorldContext, UNiagaraSystem* System, const FVector& Location);
}
