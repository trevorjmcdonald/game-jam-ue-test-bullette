#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BulletteEnemySpawner.generated.h"

class ABulletteEnemy;

USTRUCT(BlueprintType)
struct FBulletteEnemySpawnEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawning")
	TSubclassOf<ABulletteEnemy> EnemyClass;

	/** Relative chance of each spawned enemy being this one, among the entries already unlocked. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawning", meta = (ClampMin = "0"))
	float Weight = 1.0f;

	/** First wave (0-based) this enemy can appear in. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawning", meta = (ClampMin = "0"))
	int32 FirstWave = 0;
};

/**
 * Place one in the arena. Spawns growing waves of enemies in a ring around the player, just off-screen,
 * clamped to the arena so nothing appears past the floor edge. Logs alive count and frame times to LogBullette.
 * Each enemy is picked from SpawnTable by weight among the entries unlocked by the current wave.
 */
UCLASS(Abstract)
class BULLETTE_API ABulletteEnemySpawner : public AActor
{
	GENERATED_BODY()

public:
	ABulletteEnemySpawner();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Used when SpawnTable is empty, and while no SpawnTable entry is unlocked yet. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawning")
	TSubclassOf<ABulletteEnemy> EnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawning")
	TArray<FBulletteEnemySpawnEntry> SpawnTable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawning", meta = (ClampMin = "0.1", Units = "s"))
	float WaveInterval = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawning", meta = (ClampMin = "1"))
	int32 FirstWaveSize = 5;

	/** Added to the wave size after every wave. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawning", meta = (ClampMin = "0"))
	int32 WaveSizeGrowth = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawning", meta = (ClampMin = "1"))
	int32 MaxAliveEnemies = 300;

	/** Distance from the player to the spawn ring. Should sit just outside the camera view. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawning", meta = (ClampMin = "0", Units = "cm"))
	float SpawnRadius = 2500.0f;

	/** Spawn points are clamped to this square around the spawner so enemies stay over the floor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawning", meta = (ClampMin = "0", Units = "cm"))
	float ArenaHalfExtent = 3900.0f;

	/** How often to log alive count and frame times. 0 disables. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug", meta = (ClampMin = "0", Units = "s"))
	float StatsLogInterval = 5.0f;

private:
	void SpawnWave();
	TSubclassOf<ABulletteEnemy> PickEnemyClass() const;
	void LogStats();

	TArray<TWeakObjectPtr<ABulletteEnemy>> AliveEnemies;
	int32 WaveNumber = 0;
	FTimerHandle WaveTimer;
	FTimerHandle StatsTimer;

	// Frame time samples since the last stats log.
	int32 StatsFrameCount = 0;
	double StatsFrameTimeSum = 0.0;
	double StatsFrameTimeMax = 0.0;
};
