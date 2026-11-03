#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "JamKitChoice.generated.h"

/** A decision the game is waiting on: a perk to pick, a dialogue answer, a shop. */
struct FJamKitChoice
{
	FName Id;
	TArray<FName> Options;
	/** What picking option N does: the same code the game's own buttons run. */
	TFunction<void(int32 /*Index*/)> OnChosen;
};

/**
 * Where the game says "I'm waiting for the player to choose", so an unattended run can choose too
 * (UJamKitScriptSubsystem's "choose" step, a bot, a test). The game's UI stays as it is: when it shows the choice it
 * calls Offer with the options and what each does, and when the player picks one it calls Resolved. Offer writes a
 * "choice" line to the trace (what = the id, with the options), Resolved a "chose" line, so a run's decisions are on
 * record either way. Costs nothing when nobody's scripting.
 */
UCLASS()
class JAMKIT_API UJamKitChoiceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UJamKitChoiceSubsystem* Get(const UObject* WorldContext);

	/** The game shows a choice (replaces an open one with the same id). */
	void Offer(FName Id, TArray<FName> Options, TFunction<void(int32)> OnChosen);

	/** The player picked through the game's UI (or the choice went away): it's no longer open. */
	void Resolved(FName Id, FName Picked = NAME_None);

	/** Picks option Index of an open choice as if the player had; false if there's no such choice or option. */
	bool Choose(FName Id, int32 Index, const FString& By = TEXT("script"));

	const FJamKitChoice* Find(FName Id) const;
	bool IsOpen(FName Id) const { return Find(Id) != nullptr; }

private:
	TArray<FJamKitChoice> Open;
};

namespace JamKitChoice
{
	/** Shorthands that do nothing outside a game. */
	JAMKIT_API void Offer(const UObject* WorldContext, FName Id, TArray<FName> Options, TFunction<void(int32)> OnChosen);
	JAMKIT_API void Resolved(const UObject* WorldContext, FName Id, FName Picked = NAME_None);
}
