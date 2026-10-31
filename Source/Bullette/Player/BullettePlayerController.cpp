#include "Player/BullettePlayerController.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Audio/BulletteAudio.h"
#include "Components/AudioComponent.h"
#include "Components/Button.h"
#include "Bullette.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Game/BulletteGameMode.h"
#include "Game/BulletteSettingsSubsystem.h"
#include "GameplayEffect.h"
#include "InputMappingContext.h"
#include "Progression/BulletteLevelingComponent.h"
#include "Progression/BulletteUpgradeDefinition.h"
#include "TimerManager.h"
#include "UI/BulletteGameOverWidget.h"
#include "UI/BulletteHUDWidget.h"
#include "UI/BulletteLevelUpWidget.h"
#include "UI/BullettePauseMenuWidget.h"
#include "Weapons/BulletteWeaponAbility.h"

void ABullettePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	// The viewport keeps the last input mode across level loads; after Restart on the game over screen it would
	// still be UI-only.
	SetInputMode(FInputModeGameOnly());

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (ensureMsgf(DefaultMappingContext, TEXT("%s has no DefaultMappingContext assigned"), *GetName()))
		{
			// Registers the context's player-mappable keys, so rebinding in the options screen applies to it.
			FModifyContextOptions Options;
			Options.bNotifyUserSettings = true;
			Subsystem->AddMappingContext(DefaultMappingContext, 0, Options);
		}
	}

	if (ensureMsgf(HUDWidgetClass, TEXT("%s has no HUDWidgetClass assigned"), *GetName()))
	{
		HUDWidget = CreateWidget<UBulletteHUDWidget>(this, HUDWidgetClass);
		HUDWidget->AddToViewport();
	}

	if (const UBulletteSettingsSubsystem* Settings = UBulletteSettingsSubsystem::Get(this))
	{
		Settings->ApplyVolumes();
	}
	MusicComponent = BulletteAudio::PlayMusic(this, RunMusic);
}

void ABullettePlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (UBulletteLevelingComponent* Leveling = InPawn ? InPawn->FindComponentByClass<UBulletteLevelingComponent>() : nullptr)
	{
		LevelBeingChosen = Leveling->GetLevel();
		Leveling->OnLevelUp.AddUObject(this, &ABullettePlayerController::HandleLevelUp);
	}

	if (StartingWeapon)
	{
		ApplyUpgrade(StartingWeapon);
	}
}

void ABullettePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (PauseAction)
		{
			EnhancedInput->BindAction(PauseAction, ETriggerEvent::Started, this, &ABullettePlayerController::OpenPauseMenu);
		}
	}
}

void ABullettePlayerController::OpenPauseMenu()
{
	// The level-up and game over screens pause too; don't stack the pause menu on top of them.
	const ABulletteGameMode* GameMode = GetWorld()->GetAuthGameMode<ABulletteGameMode>();
	if (IsPaused() || (GameMode && GameMode->IsRunOver()) || !PauseMenuWidgetClass)
	{
		return;
	}

	if (!PauseMenuWidget)
	{
		PauseMenuWidget = CreateWidget<UBullettePauseMenuWidget>(this, PauseMenuWidgetClass);
		PauseMenuWidget->OnResume.BindUObject(this, &ABullettePlayerController::ClosePauseMenu);
		PauseMenuWidget->SetPauseAction(PauseAction);
	}
	PauseMenuWidget->AddToViewport(5);
	BulletteAudio::PlaySound(this, PauseSound, true, 0.0f);

	SetPause(true);
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(PauseMenuWidget->GetDefaultFocusWidget()->TakeWidget());
	SetInputMode(InputMode);
	SetShowMouseCursor(true);
}

void ABullettePlayerController::ClosePauseMenu()
{
	PauseMenuWidget->RemoveFromParent();

	SetPause(false);
	SetInputMode(FInputModeGameOnly());
	SetShowMouseCursor(false);
}

void ABullettePlayerController::HandleLevelUp(int32 NewLevel)
{
	const ABulletteGameMode* GameMode = GetWorld()->GetAuthGameMode<ABulletteGameMode>();
	if (GameMode && GameMode->IsRunOver())
	{
		return;
	}

	++PendingLevelUps;
	if (!LevelUpWidget || !LevelUpWidget->IsInViewport())
	{
		ShowNextLevelUp();
	}
}

void ABullettePlayerController::ShowNextLevelUp()
{
	const bool bWeaponSlotsFull = CountWeapons() >= MaxWeapons;
	TArray<FBulletteUpgradeChoice> Eligible;
	for (const UBulletteUpgradeDefinition* Upgrade : UpgradePool)
	{
		const int32 Picks = TimesPicked.FindRef(Upgrade);
		const bool bNewWeaponBlocked = Upgrade && Upgrade->IsWeapon() && Picks == 0 && bWeaponSlotsFull;
		if (Upgrade && (Upgrade->MaxPicks == 0 || Picks < Upgrade->MaxPicks) && !bNewWeaponBlocked)
		{
			Eligible.Add({ Upgrade, Picks });
		}
	}

	if (Eligible.IsEmpty() || !ensureMsgf(LevelUpWidgetClass, TEXT("%s has no LevelUpWidgetClass assigned"), *GetName()))
	{
		PendingLevelUps = 0;
		CloseLevelUp();
		return;
	}

	// Partial Fisher-Yates: shuffle just the first ChoicesPerLevel entries into place.
	const int32 NumChoices = FMath::Min(ChoicesPerLevel, Eligible.Num());
	for (int32 Index = 0; Index < NumChoices; ++Index)
	{
		Eligible.Swap(Index, FMath::RandRange(Index, Eligible.Num() - 1));
	}

	if (!LevelUpWidget)
	{
		LevelUpWidget = CreateWidget<UBulletteLevelUpWidget>(this, LevelUpWidgetClass);
		LevelUpWidget->OnUpgradeChosen.BindUObject(this, &ABullettePlayerController::HandleUpgradeChosen);
	}

	++LevelBeingChosen;
	LevelUpWidget->ShowChoices(LevelBeingChosen, TConstArrayView<FBulletteUpgradeChoice>(Eligible.GetData(), NumChoices));
	if (!LevelUpWidget->IsInViewport())
	{
		LevelUpWidget->AddToViewport();
	}
	BulletteAudio::PlaySound(this, LevelUpSound, true, 0.0f);

	SetPause(true);
	FInputModeUIOnly InputMode;
	if (UWidget* FirstButton = LevelUpWidget->GetFirstOptionButton())
	{
		InputMode.SetWidgetToFocus(FirstButton->TakeWidget());
	}
	SetInputMode(InputMode);
	SetShowMouseCursor(true);
}

void ABullettePlayerController::HandleUpgradeChosen(const UBulletteUpgradeDefinition* Upgrade)
{
	ApplyUpgrade(Upgrade);
	BulletteAudio::PlaySound(this, UpgradeChosenSound, true, 0.0f);

	--PendingLevelUps;
	if (PendingLevelUps > 0)
	{
		ShowNextLevelUp();
	}
	else
	{
		CloseLevelUp();
	}
}

void ABullettePlayerController::CloseLevelUp()
{
	if (LevelUpWidget)
	{
		LevelUpWidget->RemoveFromParent();
	}

	SetPause(false);
	SetInputMode(FInputModeGameOnly());
	SetShowMouseCursor(false);
}

void ABullettePlayerController::ApplyUpgrade(const UBulletteUpgradeDefinition* Upgrade)
{
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetPawn());
	if (!Upgrade || !ASC)
	{
		return;
	}

	const int32 Picks = TimesPicked.FindRef(Upgrade) + 1;
	if (Upgrade->WeaponAbility)
	{
		// Weapon level = number of picks. Raising the level is a spec change, not a new grant.
		if (FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromClass(Upgrade->WeaponAbility))
		{
			Spec->Level = Picks;
			ASC->MarkAbilitySpecDirty(*Spec);
			if (UBulletteWeaponAbility* Weapon = Cast<UBulletteWeaponAbility>(Spec->GetPrimaryInstance()))
			{
				Weapon->OnWeaponLevelChanged();
			}
		}
		else
		{
			ASC->GiveAbility(FGameplayAbilitySpec(Upgrade->WeaponAbility, Picks));
		}
	}
	else if (Upgrade->Effect)
	{
		const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Upgrade->Effect, 1.0f, ASC->MakeEffectContext());
		ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
	}
	else
	{
		return;
	}

	TimesPicked.Add(Upgrade, Picks);
	UE_LOG(LogBullette, Display, TEXT("UpgradeChosen upgrade=%s picks=%d"), *Upgrade->GetName(), Picks);
	RebuildWeaponSummary();
}

void ABullettePlayerController::RebuildWeaponSummary()
{
	TArray<FText> Lines;
	for (const TPair<TObjectPtr<const UBulletteUpgradeDefinition>, int32>& Picked : TimesPicked)
	{
		if (Picked.Key && Picked.Key->IsWeapon())
		{
			Lines.Add(FText::Format(NSLOCTEXT("Bullette", "WeaponLine", "{0}  Lv {1}"), Picked.Key->DisplayName, FText::AsNumber(Picked.Value)));
		}
	}
	WeaponSummary = FText::Join(FText::FromString(TEXT("\n")), Lines);
}

bool ABullettePlayerController::HandleRunOver()
{
	if (MusicComponent)
	{
		MusicComponent->FadeOut(MusicFadeOutSeconds, 0.0f);
	}

	if (!GameOverWidgetClass)
	{
		return false;
	}

	GetWorldTimerManager().SetTimer(GameOverTimer, this, &ABullettePlayerController::ShowGameOver, FMath::Max(GameOverDelay, 0.01f), false);
	return true;
}

void ABullettePlayerController::ShowGameOver()
{
	const ABulletteGameMode* GameMode = GetWorld()->GetAuthGameMode<ABulletteGameMode>();
	const UBulletteLevelingComponent* Leveling = GetPawn() ? GetPawn()->FindComponentByClass<UBulletteLevelingComponent>() : nullptr;
	const float SurvivedSeconds = GameMode ? static_cast<float>(GameMode->GetRunTimeSeconds()) : 0.0f;
	const int32 Kills = GameMode ? GameMode->GetKills() : 0;
	const int32 Seconds = FMath::FloorToInt(SurvivedSeconds);

	FNumberFormattingOptions TwoDigits;
	TwoDigits.MinimumIntegralDigits = 2;
	auto FormatTime = [&TwoDigits](int32 TotalSeconds)
	{
		return FText::Format(NSLOCTEXT("Bullette", "RunTime", "{0}:{1}"), FText::AsNumber(TotalSeconds / 60), FText::AsNumber(TotalSeconds % 60, &TwoDigits));
	};

	// Recorded once per run: the game over screen only appears once before the level reloads.
	FText BestLine;
	if (UBulletteSettingsSubsystem* Settings = UBulletteSettingsSubsystem::Get(this))
	{
		BestLine = Settings->RecordRun(SurvivedSeconds, Kills)
			? NSLOCTEXT("Bullette", "NewBest", "New best!")
			: FText::Format(NSLOCTEXT("Bullette", "Best", "Best {0}"), FormatTime(FMath::FloorToInt(Settings->GetBestSurvivedSeconds())));
	}

	const FText Stats = FText::Format(NSLOCTEXT("Bullette", "GameOverStats", "Survived {0}\nLevel {1}\n{2} kills\n{3}"),
		FormatTime(Seconds), FText::AsNumber(Leveling ? Leveling->GetLevel() : 1), FText::AsNumber(Kills), BestLine);

	if (!GameOverWidget)
	{
		GameOverWidget = CreateWidget<UBulletteGameOverWidget>(this, GameOverWidgetClass);
		GameOverWidget->OnRestart.BindUObject(this, &ABullettePlayerController::HandleRestart);
	}
	GameOverWidget->ShowResults(Stats, WeaponSummary);
	GameOverWidget->AddToViewport(10);
	BulletteAudio::PlaySound(this, GameOverSound, true, 0.0f);

	SetPause(true);
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(GameOverWidget->GetRestartButton()->TakeWidget());
	SetInputMode(InputMode);
	SetShowMouseCursor(true);
}

void ABullettePlayerController::HandleRestart()
{
	if (ABulletteGameMode* GameMode = GetWorld()->GetAuthGameMode<ABulletteGameMode>())
	{
		GameMode->RestartRun();
	}
}

int32 ABullettePlayerController::CountWeapons() const
{
	int32 NumWeapons = 0;
	for (const TPair<TObjectPtr<const UBulletteUpgradeDefinition>, int32>& Picked : TimesPicked)
	{
		if (Picked.Key && Picked.Key->IsWeapon() && Picked.Value > 0)
		{
			++NumWeapons;
		}
	}
	return NumWeapons;
}
