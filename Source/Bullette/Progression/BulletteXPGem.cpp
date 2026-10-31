#include "Progression/BulletteXPGem.h"

#include "AbilitySystem/BulletteCombatSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Progression/BulletteLevelingComponent.h"
#include "UObject/ConstructorHelpers.h"

ABulletteXPGem::ABulletteXPGem()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> GemMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	PlaceholderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderMesh"));
	PlaceholderMesh->SetStaticMesh(GemMesh.Object);
	PlaceholderMesh->SetRelativeRotation(FRotator(45.0f, 0.0f, 45.0f));
	PlaceholderMesh->SetRelativeScale3D(FVector(0.2f));
	PlaceholderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlaceholderMesh->SetCanEverAffectNavigation(false);
	PlaceholderMesh->SetCastShadow(false);
	RootComponent = PlaceholderMesh;
}

void ABulletteXPGem::BeginPlay()
{
	Super::BeginPlay();

	Player = UGameplayStatics::GetPlayerPawn(this, 0);
	PlayerASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Player.Get());
}

void ABulletteXPGem::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const AActor* PlayerActor = Player.Get();
	const UAbilitySystemComponent* ASC = PlayerASC.Get();
	if (!PlayerActor || !ASC)
	{
		return;
	}

	const FVector Location = GetActorLocation();
	FVector ToPlayer = PlayerActor->GetActorLocation() - Location;
	ToPlayer.Z = 0.0;
	const double Distance = ToPlayer.Size();

	if (!bMagnetized)
	{
		if (Distance > ASC->GetNumericAttribute(UBulletteCombatSet::GetPickupRadiusAttribute()))
		{
			return;
		}
		bMagnetized = true;
	}

	if (Distance <= CollectDistance)
	{
		if (UBulletteLevelingComponent* Leveling = PlayerActor->FindComponentByClass<UBulletteLevelingComponent>())
		{
			Leveling->AddXP(XPValue);
		}
		Destroy();
		return;
	}

	const double Step = FMath::Min(MagnetSpeed * DeltaSeconds, Distance);
	SetActorLocation(Location + ToPlayer / Distance * Step);
}
