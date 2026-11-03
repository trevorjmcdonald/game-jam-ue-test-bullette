#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"
#include "Engine/World.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "JamKitRandom.generated.h"

/**
 * Seeded randomness for gameplay, so a run can be repeated: the same seed gives the same spawns, drops and offers.
 * Ask for a named stream rather than calling FMath::Rand: JamKitRandom::Stream(this, TEXT("Spawns")).FRandRange(0, 1).
 * Each name is its own sequence (derived from the seed and the name), so adding a roll in one system doesn't shift
 * another's. Cosmetic randomness (sound pitch, shake, damage number jitter) can stay on FMath.
 *
 * The seed: -jamseed=<n> on the command line (or in Saved/jamkit-args.txt), else jamkit.Seed <n> in the console
 * (from the next level), else a fresh one per launch. Every stream starts over when a game level starts, so each run
 * (a level load, e.g. a restart) replays the same sequence for the same seed. The seed is written to the trace
 * ("seed" lines) and the log, so any run, a playtest included, can be repeated.
 *
 * For runs that repeat exactly, timing has to repeat too: -jamfixedstep=<fps> makes every frame advance the game by
 * exactly 1/fps seconds however long it took to draw (and uncaps the frame rate, so a fast machine plays faster than
 * real time). Physics can still drift a little.
 */
UCLASS()
class JAMKIT_API UJamKitRandomSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	static UJamKitRandomSubsystem* Get(const UObject* WorldContext);

	int32 GetSeed() const { return Seed; }

	/** Takes effect when the next level starts (or now, with bRestartStreams). */
	void SetSeed(int32 NewSeed, bool bRestartStreams = false);

	FRandomStream& Stream(FName Name);

	/** Every stream back to its start (done for you when a game level starts). */
	void RestartStreams();

private:
	void HandleWorldInitialized(UWorld* World, const UWorld::InitializationValues Values);
	int32 StreamSeed(FName Name) const;

	int32 Seed = 0;
	int32 PendingSeed = 0;
	bool bPendingSeed = false;
	/** Boxed so a stream stays where it is when others are added. */
	TMap<FName, TUniquePtr<FRandomStream>> Streams;
	FDelegateHandle WorldHandle;
};

namespace JamKitRandom
{
	/** The named stream of the world's game instance; outside a game (tests, editor tools) a shared unseeded one. */
	JAMKIT_API FRandomStream& Stream(const UObject* WorldContext, FName Name);
}
