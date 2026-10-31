#include "Enemies/BulletteEnemySubsystem.h"

#include "Enemies/BulletteEnemy.h"

void UBulletteEnemySubsystem::RegisterEnemy(ABulletteEnemy* Enemy)
{
	Enemies.AddUnique(Enemy);
}

void UBulletteEnemySubsystem::UnregisterEnemy(ABulletteEnemy* Enemy)
{
	Enemies.RemoveSwap(Enemy);
}

ABulletteEnemy* UBulletteEnemySubsystem::FindNearestEnemy(const FVector& Location, float MaxRange, TConstArrayView<const AActor*> Exclude) const
{
	ABulletteEnemy* Nearest = nullptr;
	double NearestDistSq = FMath::Square(static_cast<double>(MaxRange));

	for (ABulletteEnemy* Enemy : Enemies)
	{
		const double DistSq = FVector::DistSquared2D(Location, Enemy->GetActorLocation());
		if (DistSq < NearestDistSq && !Exclude.Contains(Enemy))
		{
			NearestDistSq = DistSq;
			Nearest = Enemy;
		}
	}

	return Nearest;
}

void UBulletteEnemySubsystem::FindNearestEnemies(const FVector& Location, float MaxRange, int32 Count, TArray<ABulletteEnemy*>& OutEnemies) const
{
	OutEnemies.Reset();
	if (Count <= 0)
	{
		return;
	}

	const double MaxRangeSq = FMath::Square(static_cast<double>(MaxRange));
	TArray<TPair<double, ABulletteEnemy*>> InRange;
	for (ABulletteEnemy* Enemy : Enemies)
	{
		const double DistSq = FVector::DistSquared2D(Location, Enemy->GetActorLocation());
		if (DistSq <= MaxRangeSq)
		{
			InRange.Emplace(DistSq, Enemy);
		}
	}

	InRange.Sort([](const TPair<double, ABulletteEnemy*>& A, const TPair<double, ABulletteEnemy*>& B) { return A.Key < B.Key; });
	for (int32 Index = 0; Index < FMath::Min(Count, InRange.Num()); ++Index)
	{
		OutEnemies.Add(InRange[Index].Value);
	}
}

void UBulletteEnemySubsystem::FindEnemiesInRadius(const FVector& Location, float Radius, TArray<ABulletteEnemy*>& OutEnemies) const
{
	OutEnemies.Reset();
	for (ABulletteEnemy* Enemy : Enemies)
	{
		const double Reach = Radius + Enemy->GetCollisionRadius();
		if (FVector::DistSquared2D(Location, Enemy->GetActorLocation()) <= Reach * Reach)
		{
			OutEnemies.Add(Enemy);
		}
	}
}
