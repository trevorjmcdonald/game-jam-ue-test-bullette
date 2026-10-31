#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BullettePlayerController.generated.h"

class UBulletteGameOverWidget;
class UBulletteHUDWidget;
class UBulletteLevelUpWidget;
class UBullettePauseMenuWidget;
class UBulletteUpgradeDefinition;
class UInputAction;
class UInputMappingContext;

/**
 * Adds the default Enhanced Input mapping context for the local player, and runs the level-up flow:
 * pause, offer random upgrades from UpgradePool, apply the chosen upgrade (effect or weapon), resume.
 * Also gives the pawn StartingWeapon on possession, counted as that upgrade's first pick, and shows the game over
 * screen when the run ends. The Pause action opens the pause menu, unless the game is already paused by another screen.
 */
UCLASS(Abstract)
class BULLETTE_API ABullettePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/** Weapons held and their levels, one per line. Rebuilt whenever an upgrade is applied. */
	const FText& GetWeaponSummary() const { return WeaponSummary; }

	/** Called by the game mode when the player dies. Returns false if there is no game over screen to show. */
	bool HandleRunOver();

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void SetupInputComponent() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> PauseAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pause")
	TSubclassOf<UBullettePauseMenuWidget> PauseMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD")
	TSubclassOf<UBulletteHUDWidget> HUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Level Up")
	TSubclassOf<UBulletteLevelUpWidget> LevelUpWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Level Up")
	TArray<TObjectPtr<UBulletteUpgradeDefinition>> UpgradePool;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Level Up", meta = (ClampMin = "1"))
	int32 ChoicesPerLevel = 3;

	/** A weapon upgrade the player starts with. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Level Up")
	TObjectPtr<UBulletteUpgradeDefinition> StartingWeapon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Over")
	TSubclassOf<UBulletteGameOverWidget> GameOverWidgetClass;

	/** Time between dying and the game over screen, so the death animation can play. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Over", meta = (ClampMin = "0", Units = "s"))
	float GameOverDelay = 1.5f;

	/** Once the player holds this many weapons, new weapons stop being offered (their level-ups still are). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Level Up", meta = (ClampMin = "1"))
	int32 MaxWeapons = 4;

private:
	void HandleLevelUp(int32 NewLevel);
	void ShowNextLevelUp();
	void HandleUpgradeChosen(const UBulletteUpgradeDefinition* Upgrade);
	void ApplyUpgrade(const UBulletteUpgradeDefinition* Upgrade);
	int32 CountWeapons() const;
	void RebuildWeaponSummary();
	void ShowGameOver();
	void HandleRestart();
	void CloseLevelUp();
	void OpenPauseMenu();
	void ClosePauseMenu();

	UPROPERTY()
	TObjectPtr<UBulletteHUDWidget> HUDWidget;

	UPROPERTY()
	TObjectPtr<UBulletteLevelUpWidget> LevelUpWidget;

	UPROPERTY()
	TObjectPtr<UBulletteGameOverWidget> GameOverWidget;

	UPROPERTY()
	TObjectPtr<UBullettePauseMenuWidget> PauseMenuWidget;

	FTimerHandle GameOverTimer;
	FText WeaponSummary;

	UPROPERTY()
	TMap<TObjectPtr<const UBulletteUpgradeDefinition>, int32> TimesPicked;

	/** Level-ups earned but not yet chosen (several can arrive from one big XP pickup). */
	int32 PendingLevelUps = 0;
	int32 LevelBeingChosen = 1;
};
