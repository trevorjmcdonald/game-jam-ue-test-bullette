#include "Game/BulletteGameMode.h"

#include "Bullette.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Player/BullettePlayerController.h"
#include "TimerManager.h"

void ABulletteGameMode::StartPlay()
{
	Super::StartPlay();

	RunStartTime = GetWorld()->GetTimeSeconds();
}

double ABulletteGameMode::GetRunTimeSeconds() const
{
	return (bRunOver ? RunEndTime : GetWorld()->GetTimeSeconds()) - RunStartTime;
}

void ABulletteGameMode::HandleEnemyKilled(AActor* Enemy)
{
	if (!bRunOver)
	{
		++Kills;
	}
}

void ABulletteGameMode::HandlePlayerDied(APawn* Player)
{
	if (bRunOver)
	{
		return;
	}
	bRunOver = true;
	RunEndTime = GetWorld()->GetTimeSeconds();

	UE_LOG(LogBullette, Display, TEXT("RunOver survived_s=%.1f kills=%d"), GetRunTimeSeconds(), Kills);
	ABullettePlayerController* PC = Player ? Cast<ABullettePlayerController>(Player->GetController()) : nullptr;
	if (!PC || !PC->HandleRunOver())
	{
		GetWorldTimerManager().SetTimer(RestartTimer, this, &ABulletteGameMode::RestartRun, RestartDelay, false);
	}
}

void ABulletteGameMode::RestartRun()
{
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));
}
