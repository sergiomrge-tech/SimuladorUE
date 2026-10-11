#include "StaffCharacter.h"
#include "BuildingActor.h"
#include "ResortEconomy.h"
#include "ResortGameMode.h"
#include "ResortVisuals.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AStaffCharacter::AStaffCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Building, ECR_Ignore);

	bUseControllerRotationYaw = false;
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	MoveComp->bOrientRotationToMovement = true;
	MoveComp->RotationRate = FRotator(0.f, 540.f, 0.f);
	MoveComp->MaxWalkSpeed = 170.f;
	MoveComp->bRunPhysicsWithNoController = true;
}

void AStaffCharacter::InitStaff(EStaffRole InRole)
{
	Role = InRole;

	const TCHAR* ModelKey = Role == EStaffRole::Waiter ? TEXT("waiter") : TEXT("cleaner");
	bool bHasModel = false;
	if (const UResortEconomy* Economy = AResortGameMode::GetEconomy(this))
	{
		if (const FCharacterModel* Model = Economy->FindStaffModel(ModelKey))
		{
			bHasModel = ResortVisuals::ApplyCharacterModel(this, *Model);
		}
	}

	if (!bHasModel)
	{
		const FLinearColor Shirt = Role == EStaffRole::Waiter ? FLinearColor(0.97f, 0.97f, 0.97f) : FLinearColor(0.1f, 0.6f, 0.7f);
		const FLinearColor Pants = Role == EStaffRole::Waiter ? FLinearColor(0.05f, 0.05f, 0.08f) : FLinearColor(0.12f, 0.2f, 0.3f);
		const FLinearColor Skin(0.8f, 0.58f, 0.44f);
		USceneComponent* Root = GetCapsuleComponent();
		ResortVisuals::AddPart(this, Root, TEXT("Cylinder"), FVector(0.f, 0.f, -43.f), FVector(30.f, 30.f, 90.f), FRotator::ZeroRotator, Pants, false);
		ResortVisuals::AddPart(this, Root, TEXT("Cylinder"), FVector(0.f, 0.f, 32.f), FVector(44.f, 32.f, 60.f), FRotator::ZeroRotator, Shirt, false);
		ResortVisuals::AddPart(this, Root, TEXT("Cylinder"), FVector(0.f, 26.f, 30.f), FVector(10.f, 10.f, 55.f), FRotator::ZeroRotator, Skin, false);
		ResortVisuals::AddPart(this, Root, TEXT("Cylinder"), FVector(0.f, -26.f, 30.f), FVector(10.f, 10.f, 55.f), FRotator::ZeroRotator, Skin, false);
		ResortVisuals::AddPart(this, Root, TEXT("Sphere"), FVector(0.f, 0.f, 74.f), FVector(28.f, 28.f, 30.f), FRotator::ZeroRotator, Skin, false);
		if (Role == EStaffRole::Waiter)
		{
			// bandeja
			ResortVisuals::AddPart(this, Root, TEXT("Cylinder"), FVector(30.f, 0.f, 40.f), FVector(36.f, 36.f, 3.f), FRotator::ZeroRotator, FLinearColor(0.75f, 0.75f, 0.78f), false);
		}
		else
		{
			// vassoura
			ResortVisuals::AddPart(this, Root, TEXT("Cylinder"), FVector(30.f, 30.f, 5.f), FVector(4.f, 4.f, 120.f), FRotator::ZeroRotator, FLinearColor(0.5f, 0.35f, 0.2f), false);
		}
	}

	bInitialized = true;
	PickDestination();
}

void AStaffCharacter::PickDestination()
{
	bHasDestination = true;
	Destination = FVector(FMath::FRandRange(-2500.f, 300.f), FMath::FRandRange(-1800.f, 1800.f), GetActorLocation().Z);

	if (Role != EStaffRole::Waiter)
	{
		return;
	}
	const UResortEconomy* Economy = AResortGameMode::GetEconomy(this);
	if (Economy == nullptr)
	{
		return;
	}
	TArray<ABuildingActor*> Shops;
	for (const TWeakObjectPtr<ABuildingActor>& P : Economy->GetBuildings())
	{
		if (P.IsValid() && P->IsFoodShop())
		{
			Shops.Add(P.Get());
		}
	}
	if (Shops.Num() > 0)
	{
		const ABuildingActor* Shop = Shops[FMath::RandRange(0, Shops.Num() - 1)];
		const FVector Half = Shop->GetFootprintHalf();
		Destination = Shop->GetActorLocation() + FVector(0.0, Half.Y + FMath::FRandRange(40.f, 160.f), 0.0);
		Destination.Z = GetActorLocation().Z;
	}
}

void AStaffCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bInitialized)
	{
		return;
	}

	if (PauseTimer > 0.f)
	{
		PauseTimer -= DeltaSeconds;
		return;
	}

	FVector ToDest = Destination - GetActorLocation();
	ToDest.Z = 0.f;
	const float Dist = ToDest.Size();
	if (Dist <= 80.f)
	{
		PauseTimer = FMath::FRandRange(1.5f, 4.f);
		PickDestination();
		return;
	}
	AddMovementInput(ToDest / Dist, 1.f);
}
