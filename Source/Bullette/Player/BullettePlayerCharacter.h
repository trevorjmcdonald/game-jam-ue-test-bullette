#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "BullettePlayerCharacter.generated.h"

class UAbilitySystemComponent;
class UAnimationAsset;
class UBulletteAttributeSet;
class UBulletteCombatSet;
class UBulletteHitFlashComponent;
class UCameraShakeBase;
class UBulletteLevelingComponent;
struct FOnAttributeChangeData;
class UCameraComponent;
class UInputAction;
class USoundBase;
class USpringArmComponent;
class UStaticMeshComponent;
struct FInputActionValue;

/**
 * Top-down player. Moves in world space (screen up = +X) and faces its movement direction.
 * The camera boom uses absolute rotation so the view never turns with the character.
 * Owns its ability system component (single-player, so no PlayerState). Weapons are abilities granted by the controller.
 */
UCLASS(Abstract)
class BULLETTE_API ABullettePlayerCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ABullettePlayerCharacter();

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
	virtual void PostInitializeComponents() override;

	void Move(const FInputActionValue& Value);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UBulletteAttributeSet> AttributeSet;

	UPROPERTY()
	TObjectPtr<UBulletteCombatSet> CombatSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Leveling")
	TObjectPtr<UBulletteLevelingComponent> Leveling;

	/** Flashes the character red when hurt. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Feedback")
	TObjectPtr<UBulletteHitFlashComponent> HitFlash;

	/** Played on the player camera when hurt (at most once per HurtSoundInterval) and on death. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Feedback")
	TSubclassOf<UCameraShakeBase> HurtCameraShake;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Feedback", meta = (ClampMin = "0"))
	float HurtShakeScale = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Feedback", meta = (ClampMin = "0"))
	float DeathShakeScale = 3.0f;

	/** Damage numbers for hits on the player appear this far above the actor origin. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Feedback", meta = (Units = "cm"))
	float DamageNumberHeight = 120.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "1"))
	float MaxHealth = 100.0f;

	// Starting values for UBulletteCombatSet. Upgrades modify the attributes, not these.
	// DamageScale and AttackSpeed always start at 1; weapon numbers live on the weapon abilities.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Starting Stats", meta = (ClampMin = "0", Units = "cm/s"))
	float StartingMoveSpeed = 500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Starting Stats", meta = (ClampMin = "0", Units = "cm"))
	float StartingPickupRadius = 300.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> TopDownCamera;

	/** Stand-in body until real character art is imported. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Placeholder")
	TObjectPtr<UStaticMeshComponent> PlaceholderBody;

	/** Small block on the front of the body so facing is visible. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Placeholder")
	TObjectPtr<UStaticMeshComponent> PlaceholderNose;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	// Played directly on the character mesh (single-node animation, no Anim Blueprint).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimationAsset> IdleAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimationAsset> RunAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimationAsset> DeathAnimation;

	/** Above this ground speed the run animation plays. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation", meta = (ClampMin = "0", Units = "cm/s"))
	float RunSpeedThreshold = 20.0f;

	/** Played when the player takes damage and survives, at most once per HurtSoundInterval. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundBase> HurtSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio", meta = (ClampMin = "0", Units = "s"))
	float HurtSoundInterval = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundBase> DeathSound;

public:
	virtual void Tick(float DeltaSeconds) override;

private:
	void HandleOutOfHealth(AActor* DamageInstigator);
	void HandleMoveSpeedChanged(const FOnAttributeChangeData& Data);
	void HandleHealthChanged(const FOnAttributeChangeData& Data);
	void PlayCameraShake(float Scale) const;

	const UAnimationAsset* PlayingAnimation = nullptr;
	double LastHurtSoundTime = -1.0e9;
	bool bDead = false;
};
