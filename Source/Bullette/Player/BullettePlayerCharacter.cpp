#include "Player/BullettePlayerCharacter.h"

#include "AbilitySystem/BulletteAttributeSet.h"
#include "AbilitySystem/BulletteCombatSet.h"
#include "AbilitySystemComponent.h"
#include "Camera/CameraComponent.h"
#include "Animation/AnimationAsset.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Game/BulletteGameMode.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "Progression/BulletteLevelingComponent.h"
#include "UObject/ConstructorHelpers.h"

ABullettePlayerCharacter::ABullettePlayerCharacter()
{
	// Ticks only to switch between idle and run animations.
	PrimaryActorTick.bCanEverTick = true;

	// Face the movement direction, not the controller.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
	// The arena floor has open edges; stop at them instead of falling off the world.
	Movement->bCanWalkOffLedges = false;
	// MaxWalkSpeed is driven by the MoveSpeed attribute; see PostInitializeComponents.

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->SetRelativeRotation(FRotator(-60.0f, 0.0f, 0.0f));
	CameraBoom->TargetArmLength = 1600.0f;
	CameraBoom->bDoCollisionTest = false;

	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCamera->bUsePawnControlRotation = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	// Basic shapes are 100 units across, so scale the body to match the default capsule (radius 34, half-height 88).
	PlaceholderBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderBody"));
	PlaceholderBody->SetupAttachment(GetCapsuleComponent());
	PlaceholderBody->SetStaticMesh(CylinderMesh.Object);
	PlaceholderBody->SetRelativeScale3D(FVector(0.68f, 0.68f, 1.76f));
	PlaceholderBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PlaceholderNose = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderNose"));
	PlaceholderNose->SetupAttachment(GetCapsuleComponent());
	PlaceholderNose->SetStaticMesh(CubeMesh.Object);
	PlaceholderNose->SetRelativeLocation(FVector(40.0f, 0.0f, 40.0f));
	PlaceholderNose->SetRelativeScale3D(FVector(0.3f, 0.2f, 0.2f));
	PlaceholderNose->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

	// The ASC picks up attribute sets that are default subobjects of its owner.
	AttributeSet = CreateDefaultSubobject<UBulletteAttributeSet>(TEXT("AttributeSet"));
	CombatSet = CreateDefaultSubobject<UBulletteCombatSet>(TEXT("CombatSet"));

	Leveling = CreateDefaultSubobject<UBulletteLevelingComponent>(TEXT("Leveling"));
}

UAbilitySystemComponent* ABullettePlayerCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ABullettePlayerCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Set up attributes at spawn, before the controller possesses this pawn and grants the starting weapon,
	// which activates straight away and reads AttackSpeed.
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	AttributeSet->InitMaxHealth(MaxHealth);
	AttributeSet->InitHealth(MaxHealth);
	AttributeSet->OnOutOfHealth.AddUObject(this, &ABullettePlayerCharacter::HandleOutOfHealth);

	CombatSet->InitDamageScale(1.0f);
	CombatSet->InitAttackSpeed(1.0f);
	CombatSet->InitMoveSpeed(StartingMoveSpeed);
	CombatSet->InitPickupRadius(StartingPickupRadius);
	GetCharacterMovement()->MaxWalkSpeed = StartingMoveSpeed;
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UBulletteCombatSet::GetMoveSpeedAttribute())
		.AddUObject(this, &ABullettePlayerCharacter::HandleMoveSpeedChanged);
}

void ABullettePlayerCharacter::HandleMoveSpeedChanged(const FOnAttributeChangeData& Data)
{
	GetCharacterMovement()->MaxWalkSpeed = Data.NewValue;
}

void ABullettePlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bDead)
	{
		return;
	}

	const UAnimationAsset* Wanted = GetVelocity().Size2D() > RunSpeedThreshold ? RunAnimation : IdleAnimation;
	if (Wanted && Wanted != PlayingAnimation)
	{
		GetMesh()->PlayAnimation(const_cast<UAnimationAsset*>(Wanted), true);
		PlayingAnimation = Wanted;
	}
}

void ABullettePlayerCharacter::HandleOutOfHealth(AActor* DamageInstigator)
{
	bDead = true;
	if (DeathAnimation)
	{
		GetMesh()->PlayAnimation(DeathAnimation, false);
	}

	// Ends every weapon (their attack loops, and the orbiting blades).
	AbilitySystemComponent->CancelAllAbilities();
	GetCharacterMovement()->DisableMovement();
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		DisableInput(PC);
	}

	if (ABulletteGameMode* GameMode = GetWorld()->GetAuthGameMode<ABulletteGameMode>())
	{
		GameMode->HandlePlayerDied(this);
	}
}

void ABullettePlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	if (ensureMsgf(MoveAction, TEXT("%s has no MoveAction assigned"), *GetName()))
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABullettePlayerCharacter::Move);
	}
}

void ABullettePlayerCharacter::Move(const FInputActionValue& Value)
{
	// The camera looks down +X with no yaw, so input maps straight onto world axes.
	const FVector2D Axis = Value.Get<FVector2D>();
	AddMovementInput(FVector::ForwardVector, Axis.Y);
	AddMovementInput(FVector::RightVector, Axis.X);
}
