#include "UI/BulletteHUDWidget.h"

#include "AbilitySystem/BulletteAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Bullette.h"
#include "Engine/World.h"
#include "Game/BulletteGameMode.h"
#include "Player/BullettePlayerController.h"
#include "MVVMSubsystem.h"
#include "Progression/BulletteLevelingComponent.h"
#include "UI/BulletteHUDViewModel.h"
#include "View/MVVMView.h"

void UBulletteHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!ViewModel)
	{
		ViewModel = NewObject<UBulletteHUDViewModel>(this);
	}

	UMVVMView* View = UMVVMSubsystem::GetViewFromUserWidget(this);
	const bool bBound = View && View->SetViewModelByClass(ViewModel);
	UE_LOG(LogBullette, Display, TEXT("HUD view model %s"), bBound ? TEXT("bound") : TEXT("NOT bound (check WBP_HUD's view model)"));
}

void UBulletteHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Polling is cheap here, and the view model only broadcasts fields whose values changed.
	const APawn* Pawn = GetOwningPlayerPawn();
	if (const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn))
	{
		ViewModel->SetHealth(
			ASC->GetNumericAttribute(UBulletteAttributeSet::GetHealthAttribute()),
			ASC->GetNumericAttribute(UBulletteAttributeSet::GetMaxHealthAttribute()));
	}

	if (const UBulletteLevelingComponent* Leveling = Pawn ? Pawn->FindComponentByClass<UBulletteLevelingComponent>() : nullptr)
	{
		ViewModel->SetXP(Leveling->GetXP(), Leveling->GetXPForNextLevel(), Leveling->GetLevel());
	}

	if (const ABulletteGameMode* GameMode = GetWorld()->GetAuthGameMode<ABulletteGameMode>())
	{
		ViewModel->SetRunTime(GameMode->GetRunTimeSeconds());
		ViewModel->SetKills(GameMode->GetKills());
	}
	if (const ABullettePlayerController* PC = GetOwningPlayer<ABullettePlayerController>())
	{
		ViewModel->SetWeapons(PC->GetWeaponSummary());
	}
}
