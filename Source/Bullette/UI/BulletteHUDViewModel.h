#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "BulletteHUDViewModel.generated.h"

/**
 * Display-ready values for the in-run HUD. UBulletteHUDWidget fills it; WBP_HUD's MVVM view bindings
 * connect these fields to its progress bars and text. Fields only broadcast when their value changes.
 */
UCLASS()
class BULLETTE_API UBulletteHUDViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	void SetHealth(float Current, float Max);
	void SetXP(int32 Current, int32 Needed, int32 Level);
	void SetRunTime(double Seconds);
	void SetKills(int32 Kills);
	void SetWeapons(const FText& Weapons);

	float GetHealthPercent() const { return HealthPercent; }
	FText GetHealthText() const { return HealthText; }
	float GetXPPercent() const { return XPPercent; }
	FText GetLevelText() const { return LevelText; }
	FText GetTimeText() const { return TimeText; }
	FText GetKillsText() const { return KillsText; }
	FText GetWeaponsText() const { return WeaponsText; }

private:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess))
	float HealthPercent = 1.0f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess))
	FText HealthText;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess))
	float XPPercent = 0.0f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess))
	FText LevelText;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess))
	FText TimeText;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess))
	FText KillsText;

	/** Weapons held and their levels, one per line. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess))
	FText WeaponsText;

	// Last values the text was built from, so text is only rebuilt (and broadcast) when it would change.
	int32 ShownHealth = -1;
	int32 ShownMaxHealth = -1;
	int32 ShownLevel = -1;
	int32 ShownSeconds = -1;
	int32 ShownKills = -1;
};
