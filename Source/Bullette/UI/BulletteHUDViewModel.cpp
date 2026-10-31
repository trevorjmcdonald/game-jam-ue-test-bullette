#include "UI/BulletteHUDViewModel.h"

#define LOCTEXT_NAMESPACE "BulletteHUD"

void UBulletteHUDViewModel::SetHealth(float Current, float Max)
{
	UE_MVVM_SET_PROPERTY_VALUE(HealthPercent, Max > 0.0f ? FMath::Clamp(Current / Max, 0.0f, 1.0f) : 0.0f);

	const int32 RoundedHealth = FMath::CeilToInt(Current);
	const int32 RoundedMax = FMath::RoundToInt(Max);
	if (RoundedHealth != ShownHealth || RoundedMax != ShownMaxHealth)
	{
		ShownHealth = RoundedHealth;
		ShownMaxHealth = RoundedMax;
		HealthText = FText::Format(LOCTEXT("Health", "{0} / {1}"), FText::AsNumber(RoundedHealth), FText::AsNumber(RoundedMax));
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(HealthText);
	}
}

void UBulletteHUDViewModel::SetXP(int32 Current, int32 Needed, int32 Level)
{
	UE_MVVM_SET_PROPERTY_VALUE(XPPercent, Needed > 0 ? FMath::Clamp(static_cast<float>(Current) / Needed, 0.0f, 1.0f) : 0.0f);

	if (Level != ShownLevel)
	{
		ShownLevel = Level;
		LevelText = FText::Format(LOCTEXT("Level", "Lv {0}"), FText::AsNumber(Level));
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(LevelText);
	}
}

void UBulletteHUDViewModel::SetRunTime(double Seconds)
{
	const int32 WholeSeconds = FMath::Max(0, FMath::FloorToInt(Seconds));
	if (WholeSeconds != ShownSeconds)
	{
		ShownSeconds = WholeSeconds;
		FNumberFormattingOptions TwoDigits;
		TwoDigits.MinimumIntegralDigits = 2;
		TimeText = FText::Format(LOCTEXT("Time", "{0}:{1}"), FText::AsNumber(WholeSeconds / 60), FText::AsNumber(WholeSeconds % 60, &TwoDigits));
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(TimeText);
	}
}

void UBulletteHUDViewModel::SetKills(int32 Kills)
{
	if (Kills != ShownKills)
	{
		ShownKills = Kills;
		KillsText = FText::Format(LOCTEXT("Kills", "Kills {0}"), FText::AsNumber(Kills));
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(KillsText);
	}
}

void UBulletteHUDViewModel::SetWeapons(const FText& Weapons)
{
	if (!WeaponsText.IdenticalTo(Weapons))
	{
		WeaponsText = Weapons;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(WeaponsText);
	}
}

#undef LOCTEXT_NAMESPACE
