#include "Enemies/BulletteEnemy.h"

#include "AbilitySystem/BulletteAttributeSet.h"
#include "AbilitySystem/BulletteDamage.h"
#include "AbilitySystemComponent.h"
#include "Audio/BulletteAudio.h"
#include "Bullette.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Enemies/BulletteEnemySubsystem.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Game/BulletteGameMode.h"
#include "GameplayEffect.h"
#include "Kismet/GameplayStatics.h"
#include "Progression/BulletteXPGem.h"
#include "UObject/ConstructorHelpers.h"

ABulletteEnemy::ABulletteEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	// Spawned enemies drive themselves; don't create an AI controller per enemy.
	AutoPossessAI = EAutoPossessAI::Disabled;
	AIControllerClass = nullptr;

	// Only projectile sweeps see enemies. Enemies pass through the player, the world and each other.
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(40.0f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ECC_BulletteEnemy);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_BulletteProjectile, ECR_Block);
	CollisionSphere->SetGenerateOverlapEvents(false);
	CollisionSphere->SetCanEverAffectNavigation(false);
	RootComponent = CollisionSphere;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	PlaceholderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderMesh"));
	PlaceholderMesh->SetupAttachment(CollisionSphere);
	PlaceholderMesh->SetStaticMesh(CubeMesh.Object);
	PlaceholderMesh->SetRelativeScale3D(FVector(0.6f));
	PlaceholderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlaceholderMesh->SetCanEverAffectNavigation(false);

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	// The ASC picks up attribute sets that are default subobjects of its owner.
	AttributeSet = CreateDefaultSubobject<UBulletteAttributeSet>(TEXT("AttributeSet"));
}

UAbilitySystemComponent* ABulletteEnemy::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

float ABulletteEnemy::GetCollisionRadius() const
{
	return CollisionSphere->GetScaledSphereRadius();
}

void ABulletteEnemy::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	AttributeSet->InitMaxHealth(MaxHealth);
	AttributeSet->InitHealth(MaxHealth);
	AttributeSet->OnOutOfHealth.AddUObject(this, &ABulletteEnemy::HandleOutOfHealth);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UBulletteAttributeSet::GetHealthAttribute())
		.AddUObject(this, &ABulletteEnemy::HandleHealthChanged);

	// The art mesh is added in the Blueprint, so find it rather than owning it in C++.
	BodyMesh = FindComponentByClass<USkeletalMeshComponent>();
	if (BodyMesh)
	{
		BodyScale = BodyMesh->GetRelativeScale3D();
	}

	Target = UGameplayStatics::GetPlayerPawn(this, 0);

	if (UBulletteEnemySubsystem* Enemies = GetWorld()->GetSubsystem<UBulletteEnemySubsystem>())
	{
		Enemies->RegisterEnemy(this);
	}
}

void ABulletteEnemy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UBulletteEnemySubsystem* Enemies = GetWorld()->GetSubsystem<UBulletteEnemySubsystem>())
	{
		Enemies->UnregisterEnemy(this);
	}

	Super::EndPlay(EndPlayReason);
}

void ABulletteEnemy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (BodyMesh && LastHitTime > 0.0)
	{
		// Scale pop on hit: jump to HitPopScale, ease back over HitPopDuration.
		const float Alpha = FMath::Clamp(static_cast<float>((GetWorld()->GetTimeSeconds() - LastHitTime) / HitPopDuration), 0.0f, 1.0f);
		BodyMesh->SetRelativeScale3D(BodyScale * FMath::Lerp(HitPopScale, 1.0f, Alpha));
		if (Alpha >= 1.0f)
		{
			LastHitTime = 0.0;
		}
	}

	AActor* TargetActor = Target.Get();
	if (bDead || !TargetActor)
	{
		return;
	}

	const FVector Location = GetActorLocation();
	FVector ToTarget = TargetActor->GetActorLocation() - Location;
	ToTarget.Z = 0.0;

	const double Distance = ToTarget.Size();
	if (Distance <= ContactRange)
	{
		const double Now = GetWorld()->GetTimeSeconds();
		if (Now >= NextContactTime)
		{
			NextContactTime = Now + ContactInterval;
			BulletteDamage::ApplyDamage(AbilitySystemComponent, TargetActor, ContactDamageEffect, ContactDamage);
		}
	}

	UpdateMovement(DeltaSeconds);
	UpdateBodyAnimation();
}

void ABulletteEnemy::UpdateMovement(float DeltaSeconds)
{
	MoveRelativeToTarget(false, DeltaSeconds);
}

double ABulletteEnemy::GetDistanceToTarget() const
{
	const AActor* TargetActor = Target.Get();
	return TargetActor ? FVector::Dist2D(GetActorLocation(), TargetActor->GetActorLocation()) : TNumericLimits<double>::Max();
}

void ABulletteEnemy::MoveRelativeToTarget(bool bAway, float DeltaSeconds)
{
	const AActor* TargetActor = Target.Get();
	if (!TargetActor)
	{
		return;
	}

	const FVector Location = GetActorLocation();
	FVector ToTarget = TargetActor->GetActorLocation() - Location;
	ToTarget.Z = 0.0;
	const double Distance = ToTarget.Size();
	if (Distance < UE_KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FVector Direction = bAway ? -ToTarget / Distance : ToTarget / Distance;
	const double Step = bAway ? MoveSpeed * DeltaSeconds : FMath::Min(MoveSpeed * DeltaSeconds, Distance);
	SetActorLocationAndRotation(Location + Direction * Step, Direction.Rotation());
	LastMoveTime = GetWorld()->GetTimeSeconds();
}

void ABulletteEnemy::FaceTarget()
{
	if (const AActor* TargetActor = Target.Get())
	{
		FVector ToTarget = TargetActor->GetActorLocation() - GetActorLocation();
		ToTarget.Z = 0.0;
		if (!ToTarget.IsNearlyZero())
		{
			SetActorRotation(ToTarget.Rotation());
		}
	}
}

void ABulletteEnemy::PlayBodyAnimation(UAnimationAsset* Animation, bool bLooping, float LockSeconds)
{
	if (BodyMesh && Animation)
	{
		BodyMesh->PlayAnimation(Animation, bLooping);
		PlayingAnimation = Animation;
		AnimationLockedUntil = GetWorld()->GetTimeSeconds() + LockSeconds;
	}
}

void ABulletteEnemy::UpdateBodyAnimation()
{
	if (!BodyMesh || (!RunAnimation && !IdleAnimation) || GetWorld()->GetTimeSeconds() < AnimationLockedUntil)
	{
		return;
	}

	const bool bMoving = GetWorld()->GetTimeSeconds() - LastMoveTime < 0.1;
	const UAnimationAsset* Wanted = bMoving ? RunAnimation : IdleAnimation;
	if (Wanted && Wanted != PlayingAnimation)
	{
		BodyMesh->PlayAnimation(const_cast<UAnimationAsset*>(Wanted), true);
		PlayingAnimation = Wanted;
	}
}

void ABulletteEnemy::HandleOutOfHealth(AActor* DamageInstigator)
{
	if (bDead)
	{
		return;
	}
	bDead = true;
	OnDied();
	BulletteAudio::PlaySound(this, DeathSound, false, 0.1f);

	if (UBulletteEnemySubsystem* Enemies = GetWorld()->GetSubsystem<UBulletteEnemySubsystem>())
	{
		Enemies->UnregisterEnemy(this);
	}
	if (ABulletteGameMode* GameMode = GetWorld()->GetAuthGameMode<ABulletteGameMode>())
	{
		GameMode->HandleEnemyKilled(this);
	}

	if (XPGemClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ABulletteXPGem* Gem = GetWorld()->SpawnActor<ABulletteXPGem>(XPGemClass, GetActorLocation(), FRotator::ZeroRotator, SpawnParams))
		{
			Gem->SetXPValue(XPValue);
		}
	}

	// This runs inside the damage effect's execution, so don't destroy the ASC's owner synchronously.
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
	if (BodyMesh && DeathAnimation)
	{
		BodyMesh->SetRelativeScale3D(BodyScale);
		BodyMesh->PlayAnimation(DeathAnimation, false);
		SetLifeSpan(DeathDuration);
	}
	else
	{
		SetActorHiddenInGame(true);
		SetLifeSpan(0.1f);
	}
}

void ABulletteEnemy::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	if (!bDead && Data.NewValue < Data.OldValue)
	{
		LastHitTime = GetWorld()->GetTimeSeconds();
		// A killing hit plays DeathSound instead.
		if (Data.NewValue > 0.0f)
		{
			BulletteAudio::PlaySound(this, HitSound, false, 0.1f);
		}
	}
}
