#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BulletteGameMode.generated.h"

/**
 * Arena game mode: counts kills and ends the run when the player dies. The player controller shows the game over
 * screen and calls RestartRun; without one, the arena restarts on its own after RestartDelay.
 * Pawn and controller classes are assigned in the BP_BulletteGameMode subclass.
 */
UCLASS(Abstract)
class BULLETTE_API ABulletteGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	void HandleEnemyKilled(AActor* Enemy);
	void HandlePlayerDied(APawn* Player);

	int32 GetKills() const { return Kills; }

	/** Reloads the arena for a fresh run. */
	void RestartRun();
	bool IsRunOver() const { return bRunOver; }

	/** Game time since the run started (pauses with the game), frozen once the player dies. */
	double GetRunTimeSeconds() const;

protected:
	virtual void StartPlay() override;

	/** Used only when the player controller has no game over screen. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run", meta = (ClampMin = "0", Units = "s"))
	float RestartDelay = 2.0f;

private:

	int32 Kills = 0;
	double RunStartTime = 0.0;
	double RunEndTime = 0.0;
	bool bRunOver = false;
	FTimerHandle RestartTimer;
};
