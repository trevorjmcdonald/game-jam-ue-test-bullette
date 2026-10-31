#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Pawn.h"
#include "BulletteEnemy.generated.h"

class ABulletteXPGem;
class UAnimationAsset;
class USkeletalMeshComponent;
struct FOnAttributeChangeData;
class UAbilitySystemComponent;
class UBulletteAttributeSet;
class UGameplayEffect;
class UBulletteHitFlashComponent;
class UNiagaraSystem;
class USoundBase;
class USphereComponent;
class UStaticMeshComponent;

/**
 * Lightweight enemy: no CharacterMovement, no controller, no physics.
 * By default slides straight at the player on the XY plane in its own Tick (UpdateMovement), and damages the player
 * on contact. Subclasses can drive movement themselves (ABulletteCasterEnemy uses a StateTree).
 * Health and damage go through its own ability system component.
 */
UCLASS(Abstract)
class BULLETTE_API ABulletteEnemy : public APawn, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ABulletteEnemy();

	virtual void Tick(float DeltaSeconds) override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/** Distance from the actor origin to the ground, used by the spawner to place enemies on the floor. */
	float GetCollisionRadius() const;

	AActor* GetTarget() const { return Target.Get(); }
	bool IsDead() const { return bDead; }

	/** XY distance to the target, or a very large number if there is none. */
	double GetDistanceToTarget() const;

	/** Moves on the XY plane toward (or away from) the target at MoveSpeed, facing the way it moves. */
	void MoveRelativeToTarget(bool bAway, float DeltaSeconds);

	/** Turns to face the target without moving. */
	void FaceTarget();

	/** Plays Animation on the body mesh and holds it there for LockSeconds before idle/run switching resumes. */
	void PlayBodyAnimation(UAnimationAsset* Animation, bool bLooping, float LockSeconds);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Default: chase the target. */
	virtual void UpdateMovement(float DeltaSeconds);

	/** Called once when health runs out, before the death animation starts. */
	virtual void OnDied() {}

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision")
	TObjectPtr<USphereComponent> CollisionSphere;

	/** Stand-in body until real enemy art is imported. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Placeholder")
	TObjectPtr<UStaticMeshComponent> PlaceholderMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	/** Flashes the body mesh white when hit. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Feedback")
	TObjectPtr<UBulletteHitFlashComponent> HitFlash;

	UPROPERTY()
	TObjectPtr<UBulletteAttributeSet> AttributeSet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0", Units = "cm/s"))
	float MoveSpeed = 300.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "1"))
	float MaxHealth = 20.0f;

	/** Instant effect that adds SetByCaller.Damage to the target's Damage attribute. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Contact Damage")
	TSubclassOf<UGameplayEffect> ContactDamageEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Contact Damage", meta = (ClampMin = "0"))
	float ContactDamage = 5.0f;

	/** Center-to-center distance (XY) at which this enemy is touching the player. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Contact Damage", meta = (ClampMin = "0", Units = "cm"))
	float ContactRange = 80.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Contact Damage", meta = (ClampMin = "0.05", Units = "s"))
	float ContactInterval = 1.0f;

	/** Dropped where the enemy dies. Leave empty for no drop. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drops")
	TSubclassOf<ABulletteXPGem> XPGemClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drops", meta = (ClampMin = "0"))
	int32 XPValue = 1;

	/** Optional: looped on the body mesh while moving / standing still. Leave both empty to keep the Blueprint's animation. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Feedback")
	TObjectPtr<UAnimationAsset> RunAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Feedback")
	TObjectPtr<UAnimationAsset> IdleAnimation;

	/** Played on the Blueprint's skeletal mesh when the enemy dies. Without one, the enemy just vanishes. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Feedback")
	TObjectPtr<UAnimationAsset> DeathAnimation;

	/** How long the corpse stays (should cover the death animation). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Feedback", meta = (ClampMin = "0.1", Units = "s"))
	float DeathDuration = 1.0f;

	/** The mesh pops to this scale when hit, then eases back. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Feedback", meta = (ClampMin = "1"))
	float HitPopScale = 1.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Feedback", meta = (ClampMin = "0.01", Units = "s"))
	float HitPopDuration = 0.12f;

	/** Damage numbers appear this far above the actor origin. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Feedback", meta = (Units = "cm"))
	float DamageNumberHeight = 90.0f;

	/** One-shot Niagara burst where the enemy dies. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Feedback")
	TObjectPtr<UNiagaraSystem> DeathEffect;

	/** Played when the enemy takes damage and survives. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Feedback|Audio")
	TObjectPtr<USoundBase> HitSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Feedback|Audio")
	TObjectPtr<USoundBase> DeathSound;

private:
	void HandleOutOfHealth(AActor* DamageInstigator);
	void HandleHealthChanged(const FOnAttributeChangeData& Data);

	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> BodyMesh;

	void UpdateBodyAnimation();

	FVector BodyScale = FVector::OneVector;
	double LastHitTime = 0.0;
	/** When the enemy last moved. Movement can come from Tick or from a StateTree component, in either order. */
	double LastMoveTime = -1.0;
	const UAnimationAsset* PlayingAnimation = nullptr;
	double AnimationLockedUntil = 0.0;
	TWeakObjectPtr<AActor> Target;
	double NextContactTime = 0.0;
	bool bDead = false;
};
