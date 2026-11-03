#include "Player/JamKitPlayerController.h"

#include "Audio/JamKitAudio.h"
#include "Components/AudioComponent.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "JamKit.h"
#include "Settings/JamKitSettingsSubsystem.h"
#include "UI/JamKitDamageNumbersWidget.h"
#include "UI/JamKitFadeWidget.h"
#include "UI/JamKitPauseMenuWidget.h"
#include "Verify/JamKitTrace.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

void AJamKitPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}
	JamKitTrace::Event(this, TEXT("level"), UWorld::RemovePIEPrefix(GetWorld()->GetMapName()), FJamKitTraceFields().Add(TEXT("pawn"), GetPawn() ? GetPawn()->GetActorLocation() : FVector::ZeroVector));

	// The viewport keeps the last input mode across level loads; coming from a menu it would still be UI-only.
	SetInputMode(FInputModeGameOnly());
	SetShowMouseCursor(!bHideCursorInGame);

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			// Registers the context's player-mappable keys, so rebinding in the options screen applies to it.
			FModifyContextOptions Options;
			Options.bNotifyUserSettings = true;
			Subsystem->AddMappingContext(DefaultMappingContext, 0, Options);
		}
		else
		{
			UE_LOG(LogJamKit, Warning, TEXT("%s has no DefaultMappingContext"), *GetName());
		}
	}

	if (DamageNumbersWidgetClass)
	{
		DamageNumbersWidget = CreateWidget<UJamKitDamageNumbersWidget>(this, DamageNumbersWidgetClass);
		DamageNumbersWidget->AddToViewport(-1);
	}

	if (const UJamKitSettingsSubsystem* Settings = UJamKitSettingsSubsystem::Get(this))
	{
		Settings->ApplyVolumes();
	}
	MusicComponent = JamKitAudio::PlayMusic(this, LevelMusic);

	UJamKitFadeWidget::FadeIn(this, /*bWaitForShaders*/ true);
}

void AJamKitPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (PauseAction)
		{
			EnhancedInput->BindAction(PauseAction, ETriggerEvent::Started, this, &AJamKitPlayerController::OpenPauseMenu);
		}
	}
}

bool AJamKitPlayerController::CanOpenPauseMenu() const
{
	return !IsPaused();
}

void AJamKitPlayerController::OpenPauseMenu()
{
	if (!PauseMenuWidgetClass || !CanOpenPauseMenu())
	{
		return;
	}

	if (!PauseMenuWidget)
	{
		PauseMenuWidget = CreateWidget<UJamKitPauseMenuWidget>(this, PauseMenuWidgetClass);
		PauseMenuWidget->OnResume.BindUObject(this, &AJamKitPlayerController::ClosePauseMenu);
		PauseMenuWidget->SetPauseAction(PauseAction);
	}
	PauseMenuWidget->AddToViewport(5);
	JamKitTrace::Event(this, TEXT("pause"), TEXT("open"));
	JamKitAudio::PlaySound(this, PauseSound, true, 0.0f);

	SetPause(true);
	FInputModeUIOnly InputMode;
	if (UWidget* Focus = PauseMenuWidget->GetDefaultFocusWidget())
	{
		InputMode.SetWidgetToFocus(Focus->TakeWidget());
	}
	SetInputMode(InputMode);
	SetShowMouseCursor(true);
}

void AJamKitPlayerController::ClosePauseMenu()
{
	PauseMenuWidget->RemoveFromParent();
	JamKitTrace::Event(this, TEXT("pause"), TEXT("close"));

	SetPause(false);
	SetInputMode(FInputModeGameOnly());
	SetShowMouseCursor(!bHideCursorInGame);
}

void AJamKitPlayerController::StopLevelMusic(float FadeOutSeconds)
{
	if (MusicComponent)
	{
		MusicComponent->FadeOut(FadeOutSeconds, 0.0f);
		MusicComponent = nullptr;
	}
}
