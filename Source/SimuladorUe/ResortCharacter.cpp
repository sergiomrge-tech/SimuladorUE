#include "ResortCharacter.h"
#include "BuildingActor.h"
#include "ResortEconomy.h"
#include "ResortGameMode.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"

AResortCharacter::AResortCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(10.f, 0.f, 64.f));
	Camera->bUsePawnControlRotation = true;

	bUseControllerRotationYaw = true;

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	MoveComp->MaxWalkSpeed = WalkSpeed;
	MoveComp->JumpZVelocity = 420.f;
	MoveComp->BrakingDecelerationWalking = 2000.f;
}

// ---------------------------------------------------------------- Entrada

void AResortCharacter::EnsureInput()
{
	if (InputContext != nullptr)
	{
		return;
	}

	auto MakeAction = [this](const TCHAR* Name, EInputActionValueType Type)
	{
		UInputAction* Action = NewObject<UInputAction>(this, FName(Name));
		Action->ValueType = Type;
		return Action;
	};

	MoveAction = MakeAction(TEXT("IA_Move"), EInputActionValueType::Axis2D);
	LookAction = MakeAction(TEXT("IA_Look"), EInputActionValueType::Axis2D);
	JumpAction = MakeAction(TEXT("IA_Jump"), EInputActionValueType::Boolean);
	SprintAction = MakeAction(TEXT("IA_Sprint"), EInputActionValueType::Boolean);
	BuildAction = MakeAction(TEXT("IA_Build"), EInputActionValueType::Boolean);
	NextAction = MakeAction(TEXT("IA_NextItem"), EInputActionValueType::Boolean);
	PrevAction = MakeAction(TEXT("IA_PrevItem"), EInputActionValueType::Boolean);
	RotateAction = MakeAction(TEXT("IA_Rotate"), EInputActionValueType::Boolean);
	PlaceAction = MakeAction(TEXT("IA_Place"), EInputActionValueType::Boolean);
	DemolishAction = MakeAction(TEXT("IA_Demolish"), EInputActionValueType::Boolean);
	SaveAction = MakeAction(TEXT("IA_Save"), EInputActionValueType::Boolean);
	LoadAction = MakeAction(TEXT("IA_Load"), EInputActionValueType::Boolean);
	NewGameAction = MakeAction(TEXT("IA_NewGame"), EInputActionValueType::Boolean);

	InputContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Resort"));

	// Movimento: D = +X, A = -X, W = +Y, S = -Y
	InputContext->MapKey(MoveAction, EKeys::D);
	{
		FEnhancedActionKeyMapping& M = InputContext->MapKey(MoveAction, EKeys::A);
		M.Modifiers.Add(NewObject<UInputModifierNegate>(InputContext));
	}
	{
		FEnhancedActionKeyMapping& M = InputContext->MapKey(MoveAction, EKeys::W);
		UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(InputContext);
		Swizzle->Order = EInputAxisSwizzle::YXZ;
		M.Modifiers.Add(Swizzle);
	}
	{
		FEnhancedActionKeyMapping& M = InputContext->MapKey(MoveAction, EKeys::S);
		UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(InputContext);
		Swizzle->Order = EInputAxisSwizzle::YXZ;
		M.Modifiers.Add(Swizzle);
		M.Modifiers.Add(NewObject<UInputModifierNegate>(InputContext));
	}

	// Olhar: mouse (eixo Y invertido, como no template da Unreal)
	{
		FEnhancedActionKeyMapping& M = InputContext->MapKey(LookAction, EKeys::Mouse2D);
		UInputModifierNegate* Negate = NewObject<UInputModifierNegate>(InputContext);
		Negate->bX = false;
		Negate->bY = true;
		Negate->bZ = false;
		M.Modifiers.Add(Negate);
	}

	InputContext->MapKey(JumpAction, EKeys::SpaceBar);
	InputContext->MapKey(SprintAction, EKeys::LeftShift);
	InputContext->MapKey(BuildAction, EKeys::B);
	InputContext->MapKey(NextAction, EKeys::E);
	InputContext->MapKey(NextAction, EKeys::MouseScrollUp);
	InputContext->MapKey(PrevAction, EKeys::Q);
	InputContext->MapKey(PrevAction, EKeys::MouseScrollDown);
	InputContext->MapKey(RotateAction, EKeys::R);
	InputContext->MapKey(PlaceAction, EKeys::LeftMouseButton);
	InputContext->MapKey(DemolishAction, EKeys::X);
	InputContext->MapKey(SaveAction, EKeys::F5);
	InputContext->MapKey(LoadAction, EKeys::F9);
	InputContext->MapKey(NewGameAction, EKeys::F7);
}

void AResortCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	EnsureInput();

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (Input == nullptr)
	{
		return;
	}

	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AResortCharacter::Move);
	Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &AResortCharacter::Look);
	Input->BindAction(JumpAction, ETriggerEvent::Started, this, &AResortCharacter::DoJump);
	Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &AResortCharacter::DoStopJump);
	Input->BindAction(SprintAction, ETriggerEvent::Started, this, &AResortCharacter::SprintOn);
	Input->BindAction(SprintAction, ETriggerEvent::Completed, this, &AResortCharacter::SprintOff);
	Input->BindAction(BuildAction, ETriggerEvent::Started, this, &AResortCharacter::ToggleBuildMode);
	Input->BindAction(NextAction, ETriggerEvent::Started, this, &AResortCharacter::NextItem);
	Input->BindAction(PrevAction, ETriggerEvent::Started, this, &AResortCharacter::PrevItem);
	Input->BindAction(RotateAction, ETriggerEvent::Started, this, &AResortCharacter::RotateGhost);
	Input->BindAction(PlaceAction, ETriggerEvent::Started, this, &AResortCharacter::PlaceBuilding);
	Input->BindAction(DemolishAction, ETriggerEvent::Started, this, &AResortCharacter::DemolishBuilding);
	Input->BindAction(SaveAction, ETriggerEvent::Started, this, &AResortCharacter::SaveKey);
	Input->BindAction(LoadAction, ETriggerEvent::Started, this, &AResortCharacter::LoadKey);
	Input->BindAction(NewGameAction, ETriggerEvent::Started, this, &AResortCharacter::NewGameKey);
}

void AResortCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	EnsureInput();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
				LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				Subsystem->AddMappingContext(InputContext, 0);
			}
		}
	}
}

void AResortCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddMovementInput(GetActorForwardVector(), Axis.Y);
	AddMovementInput(GetActorRightVector(), Axis.X);
}

void AResortCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);
}

void AResortCharacter::DoJump() { Jump(); }
void AResortCharacter::DoStopJump() { StopJumping(); }
void AResortCharacter::SprintOn() { GetCharacterMovement()->MaxWalkSpeed = SprintSpeed; }
void AResortCharacter::SprintOff() { GetCharacterMovement()->MaxWalkSpeed = WalkSpeed; }

// ------------------------------------------------------------ Construcao

const FBuildingDef* AResortCharacter::GetSelectedDef() const
{
	UResortEconomy* Economy = AResortGameMode::GetEconomy(this);
	if (Economy == nullptr)
	{
		return nullptr;
	}
	const TArray<const FBuildingDef*> List = Economy->GetUnlockedBuildings();
	if (List.Num() == 0)
	{
		return nullptr;
	}
	return List[FMath::Clamp(SelectedIndex, 0, List.Num() - 1)];
}

void AResortCharacter::ToggleBuildMode()
{
	bBuildMode = !bBuildMode;
	if (!bBuildMode)
	{
		DestroyGhost();
		StatusText.Reset();
	}
}

void AResortCharacter::NextItem() { CycleSelection(+1); }
void AResortCharacter::PrevItem() { CycleSelection(-1); }

void AResortCharacter::CycleSelection(int32 Direction)
{
	if (!bBuildMode)
	{
		return;
	}
	UResortEconomy* Economy = AResortGameMode::GetEconomy(this);
	if (Economy == nullptr)
	{
		return;
	}
	const int32 Count = Economy->GetUnlockedBuildings().Num();
	if (Count == 0)
	{
		return;
	}
	SelectedIndex = (FMath::Clamp(SelectedIndex, 0, Count - 1) + Direction + Count) % Count;
}

void AResortCharacter::RotateGhost()
{
	if (bBuildMode)
	{
		GhostYaw = FMath::Fmod(GhostYaw + 90.f, 360.f);
	}
}

void AResortCharacter::DestroyGhost()
{
	if (Ghost != nullptr)
	{
		Ghost->Destroy();
		Ghost = nullptr;
	}
	GhostDefId.Reset();
	bPlacementValid = false;
}

void AResortCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bBuildMode)
	{
		UpdateGhost();
	}
}

void AResortCharacter::UpdateGhost()
{
	UWorld* World = GetWorld();
	UResortEconomy* Economy = AResortGameMode::GetEconomy(this);
	APlayerController* PC = Cast<APlayerController>(GetController());
	const FBuildingDef* Def = GetSelectedDef();
	if (World == nullptr || Economy == nullptr || PC == nullptr || Def == nullptr)
	{
		return;
	}

	// Troca de item: recria a previa
	if (Ghost == nullptr || GhostDefId != Def->Id)
	{
		DestroyGhost();
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Ghost = World->SpawnActor<ABuildingActor>(ABuildingActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
		if (Ghost == nullptr)
		{
			return;
		}
		Ghost->InitFromDef(*Def, EBuildingMode::Ghost);
		GhostDefId = Def->Id;
	}

	// Onde o olhar encontra o chao (plano Z = 0)
	FVector CamLoc;
	FRotator CamRot;
	PC->GetPlayerViewPoint(CamLoc, CamRot);
	const FVector Dir = CamRot.Vector();

	FVector Hit = GetActorLocation();
	if (Dir.Z < -0.02)
	{
		const double T = (0.0 - CamLoc.Z) / Dir.Z;
		Hit = CamLoc + Dir * T;
	}
	else
	{
		// Olhando para o horizonte: coloca na frente do jogador
		Hit = GetActorLocation() + GetActorForwardVector() * 600.f;
	}

	// Limita a distancia ao jogador e encaixa na grade
	const FVector Origin = GetActorLocation();
	FVector Offset = Hit - Origin;
	Offset.Z = 0.f;
	Offset = Offset.GetClampedToMaxSize2D(BuildRange);
	FVector Spot(Origin.X + Offset.X, Origin.Y + Offset.Y, 0.f);
	const double Grid = static_cast<double>(GridSize);
	Spot.X = FMath::GridSnap(Spot.X, Grid);
	Spot.Y = FMath::GridSnap(Spot.Y, Grid);

	const FRotator Rot(0.f, GhostYaw, 0.f);
	Ghost->SetActorLocationAndRotation(Spot, Rot);

	// Validade: dinheiro e espaco livre
	bool bValid = true;
	StatusText.Reset();

	if (!Economy->CanAfford(Def->Cost))
	{
		bValid = false;
		StatusText = TEXT("Dinheiro insuficiente");
	}
	else
	{
		const FVector Half = Ghost->GetFootprintHalf();
		const FVector TestHalf(FMath::Max(1.0, Half.X - 8.0), FMath::Max(1.0, Half.Y - 8.0), Half.Z);
		FCollisionQueryParams Query(SCENE_QUERY_STAT(ResortPlacement), false, this);
		Query.AddIgnoredActor(Ghost);
		const bool bBlocked = World->OverlapBlockingTestByChannel(
			Spot + FVector(0.f, 0.f, Half.Z + 6.f),
			FQuat(Rot),
			ECC_Building,
			FCollisionShape::MakeBox(TestHalf),
			Query);
		if (bBlocked)
		{
			bValid = false;
			StatusText = TEXT("Local ocupado ou fora da praia");
		}
	}

	bPlacementValid = bValid;
	Ghost->SetGhostValid(bValid);
}

void AResortCharacter::PlaceBuilding()
{
	if (!bBuildMode || Ghost == nullptr || !bPlacementValid)
	{
		return;
	}
	UWorld* World = GetWorld();
	UResortEconomy* Economy = AResortGameMode::GetEconomy(this);
	const FBuildingDef* Def = GetSelectedDef();
	if (World == nullptr || Economy == nullptr || Def == nullptr)
	{
		return;
	}
	if (!Economy->TrySpend(static_cast<float>(Def->Cost)))
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ABuildingActor* Building = World->SpawnActor<ABuildingActor>(
		ABuildingActor::StaticClass(), Ghost->GetActorLocation(), Ghost->GetActorRotation(), Params);
	if (Building == nullptr)
	{
		Economy->AddMoney(static_cast<float>(Def->Cost)); // devolve
		return;
	}

	Building->InitFromDef(*Def, EBuildingMode::Placed);
	Economy->PushNotification(
		FString::Printf(TEXT("%s construído (-R$ %d)"), *Def->DisplayName, Def->Cost),
		FLinearColor(0.7f, 0.9f, 1.f));
}

void AResortCharacter::SaveKey()
{
	if (AResortGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AResortGameMode>() : nullptr)
	{
		GM->SaveResort();
	}
}

void AResortCharacter::LoadKey()
{
	if (AResortGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AResortGameMode>() : nullptr)
	{
		GM->LoadResort();
	}
}

void AResortCharacter::NewGameKey()
{
	if (AResortGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AResortGameMode>() : nullptr)
	{
		GM->NewResort();
	}
}

void AResortCharacter::DemolishBuilding()
{
	if (!bBuildMode)
	{
		return;
	}
	UWorld* World = GetWorld();
	UResortEconomy* Economy = AResortGameMode::GetEconomy(this);
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (World == nullptr || Economy == nullptr || PC == nullptr)
	{
		return;
	}

	FVector CamLoc;
	FRotator CamRot;
	PC->GetPlayerViewPoint(CamLoc, CamRot);

	FCollisionQueryParams Query(SCENE_QUERY_STAT(ResortDemolish), false, this);
	if (Ghost != nullptr)
	{
		Query.AddIgnoredActor(Ghost);
	}

	FHitResult Hit;
	if (!World->LineTraceSingleByChannel(Hit, CamLoc, CamLoc + CamRot.Vector() * 1500.f, ECC_Visibility, Query))
	{
		return;
	}

	ABuildingActor* Building = Cast<ABuildingActor>(Hit.GetActor());
	if (Building == nullptr || Building->GetMode() != EBuildingMode::Placed)
	{
		return;
	}

	const FBuildingDef Def = Building->GetDef();
	const int32 Refund = Def.Cost / 2;
	Economy->AddMoney(static_cast<float>(Refund));
	Building->Destroy();
	Economy->PushNotification(
		FString::Printf(TEXT("%s demolido (+R$ %d)"), *Def.DisplayName, Refund),
		FLinearColor(1.f, 0.7f, 0.5f));
}
