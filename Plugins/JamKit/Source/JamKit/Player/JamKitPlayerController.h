#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "JamKitPlayerController.generated.h"

class UAudioComponent;
class UInputAction;
class UInputMappingContext;
class UJamKitDamageNumbersWidget;
class UJamKitPauseMenuWidget;
class USoundBase;

/**
 * Base for the game's in-level player controller. On BeginPlay it fades in, resets input to game-only, adds
 * DefaultMappingContext (registered with the rebinding settings), applies the saved volumes, starts LevelMusic and,
 * if set, creates the damage numbers layer. PauseAction opens the pause menu (UI-only input, game paused) unless
 * CanOpenPauseMenu says no. Subclass it for game-specific flow and call Super in the overrides.
 */
UCLASS()
class JAMKIT_API AJamKitPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/** Fades the level's music out (for game over and similar). */
	void StopLevelMusic(float FadeOutSeconds = 1.0f);

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	/** False blocks the pause menu. Default: not while the game is already paused by another screen. */
	virtual bool CanOpenPauseMenu() const;

	void OpenPauseMenu();
	void ClosePauseMenu();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> PauseAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pause")
	TSubclassOf<UJamKitPauseMenuWidget> PauseMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pause")
	TObjectPtr<USoundBase> PauseSound;

	/** Full-screen layer that draws damage numbers (UJamKitFeedbackSubsystem). Leave empty for none. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UJamKitDamageNumbersWidget> DamageNumbersWidgetClass;

	/** Plays from the start of the level. Set it to loop on the asset. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundBase> LevelMusic;

	/** Hides the mouse cursor during play (shown again while the pause menu is open). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	bool bHideCursorInGame = true;

	UPROPERTY()
	TObjectPtr<UJamKitPauseMenuWidget> PauseMenuWidget;

private:
	UPROPERTY()
	TObjectPtr<UJamKitDamageNumbersWidget> DamageNumbersWidget;

	UPROPERTY()
	TObjectPtr<UAudioComponent> MusicComponent;
};
