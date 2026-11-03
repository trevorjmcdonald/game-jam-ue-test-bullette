#include "Enemies/BulletteEnemySpawner.h"

#include "Bullette.h"
#include "Enemies/BulletteEnemy.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Random/JamKitRandom.h"
#include "TimerManager.h"

ABulletteEnemySpawner::ABulletteEnemySpawner()
{
	// Ticks only to sample frame times for the stats log.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ABulletteEnemySpawner::BeginPlay()
{
	Super::BeginPlay();

	if (!ensureMsgf(EnemyClass || !SpawnTable.IsEmpty(), TEXT("%s has no EnemyClass or SpawnTable"), *GetName()))
	{
		return;
	}

	FTimerManager& Timers = GetWorldTimerManager();
	Timers.SetTimer(WaveTimer, this, &ABulletteEnemySpawner::SpawnWave, WaveInterval, true, 0.0f);
	if (StatsLogInterval > 0.0f)
	{
		Timers.SetTimer(StatsTimer, this, &ABulletteEnemySpawner::LogStats, StatsLogInterval, true);
	}
}

void ABulletteEnemySpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearAllTimersForObject(this);

	Super::EndPlay(EndPlayReason);
}

void ABulletteEnemySpawner::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	++StatsFrameCount;
	StatsFrameTimeSum += DeltaSeconds;
	StatsFrameTimeMax = FMath::Max(StatsFrameTimeMax, static_cast<double>(DeltaSeconds));
}

void ABulletteEnemySpawner::SpawnWave()
{
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player)
	{
		return;
	}

	AliveEnemies.RemoveAll([](const TWeakObjectPtr<ABulletteEnemy>& Enemy) { return !Enemy.IsValid(); });

	const int32 WaveSize = FirstWaveSize + WaveNumber * WaveSizeGrowth;
	const int32 ToSpawn = FMath::Min(WaveSize, MaxAliveEnemies - AliveEnemies.Num());
	if (ToSpawn <= 0)
	{
		++WaveNumber;
		return;
	}

	// Keep enemies on the same floor as the player.
	const FVector PlayerLocation = Player->GetActorLocation();
	const double GroundZ = PlayerLocation.Z - Player->GetSimpleCollisionHalfHeight();
	const FVector ArenaCenter = GetActorLocation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (int32 Index = 0; Index < ToSpawn; ++Index)
	{
		const double Angle = JamKitRandom::Stream(this, TEXT("Spawns")).FRandRange(0.0, UE_TWO_PI);
		FVector SpawnLocation = PlayerLocation + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0) * SpawnRadius;
		SpawnLocation.X = FMath::Clamp(SpawnLocation.X, ArenaCenter.X - ArenaHalfExtent, ArenaCenter.X + ArenaHalfExtent);
		SpawnLocation.Y = FMath::Clamp(SpawnLocation.Y, ArenaCenter.Y - ArenaHalfExtent, ArenaCenter.Y + ArenaHalfExtent);
		const TSubclassOf<ABulletteEnemy> SpawnClass = PickEnemyClass();
		if (!SpawnClass)
		{
			continue;
		}
		SpawnLocation.Z = GroundZ + SpawnClass->GetDefaultObject<ABulletteEnemy>()->GetCollisionRadius();

		if (ABulletteEnemy* Enemy = GetWorld()->SpawnActor<ABulletteEnemy>(SpawnClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams))
		{
			AliveEnemies.Add(Enemy);
		}
	}
	++WaveNumber;
}

TSubclassOf<ABulletteEnemy> ABulletteEnemySpawner::PickEnemyClass() const
{
	float TotalWeight = 0.0f;
	for (const FBulletteEnemySpawnEntry& Entry : SpawnTable)
	{
		if (Entry.EnemyClass && Entry.FirstWave <= WaveNumber)
		{
			TotalWeight += Entry.Weight;
		}
	}

	float Roll = static_cast<float>(JamKitRandom::Stream(this, TEXT("Spawns")).FRandRange(0.0f, TotalWeight));
	for (const FBulletteEnemySpawnEntry& Entry : SpawnTable)
	{
		if (Entry.EnemyClass && Entry.FirstWave <= WaveNumber && Entry.Weight > 0.0f)
		{
			Roll -= Entry.Weight;
			if (Roll <= 0.0f)
			{
				return Entry.EnemyClass;
			}
		}
	}
	return EnemyClass;
}

void ABulletteEnemySpawner::LogStats()
{
	if (StatsFrameCount == 0)
	{
		return;
	}

	const double AverageMs = StatsFrameTimeSum / StatsFrameCount * 1000.0;
	UE_LOG(LogBullette, Display, TEXT("EnemyStats wave=%d alive=%d avg_frame_ms=%.2f max_frame_ms=%.2f fps=%.0f"),
		WaveNumber, AliveEnemies.Num(), AverageMs, StatsFrameTimeMax * 1000.0, 1000.0 / AverageMs);

	StatsFrameCount = 0;
	StatsFrameTimeSum = 0.0;
	StatsFrameTimeMax = 0.0;
}
