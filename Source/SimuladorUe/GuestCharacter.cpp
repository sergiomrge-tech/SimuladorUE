#include "GuestCharacter.h"
#include "ResortEconomy.h"
#include "ResortGameMode.h"
#include "ResortVisuals.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/CharacterMovementComponent.h"

AGuestCharacter::AGuestCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);
	// Hospedes atravessam uns aos outros, o jogador e as construcoes (andam em linha reta).
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Building, ECR_Ignore);

	bUseControllerRotationYaw = false;
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	MoveComp->bOrientRotationToMovement = true;
	MoveComp->RotationRate = FRotator(0.f, 540.f, 0.f);
	MoveComp->MaxWalkSpeed = 220.f;
	// Sem controlador de IA: a fisica precisa rodar mesmo assim.
	MoveComp->bRunPhysicsWithNoController = true;
}

void AGuestCharacter::InitGuest(const FGuestDef& InDef)
{
	Def = InDef;
	Budget = FMath::FRandRange(Def.BudgetMin, Def.BudgetMax);
	VisitsLeft = FMath::RandRange(Def.MinVisits, Def.MaxVisits);
	Satisfaction = 0.5f;
	GetCharacterMovement()->MaxWalkSpeed = Def.WalkSpeed * FMath::FRandRange(0.9f, 1.1f);

	// Modelo 3D final (se configurado em Guests.json); senao, boneco provisorio de formas basicas
	const bool bHasModel = ResortVisuals::ApplyCharacterModel(this, Def.Model);

	// Visual provisorio: pernas, camisa, bracos, cabeca e cabelo
	static const FLinearColor Skins[4] = {
		FLinearColor(0.85f, 0.62f, 0.48f), FLinearColor(0.6f, 0.4f, 0.28f),
		FLinearColor(0.4f, 0.26f, 0.18f), FLinearColor(0.95f, 0.78f, 0.65f) };
	static const FLinearColor Pants[4] = {
		FLinearColor(0.1f, 0.15f, 0.35f), FLinearColor(0.75f, 0.65f, 0.45f),
		FLinearColor(0.9f, 0.9f, 0.85f), FLinearColor(0.15f, 0.4f, 0.45f) };
	static const FLinearColor Hair[3] = {
		FLinearColor(0.05f, 0.03f, 0.02f), FLinearColor(0.35f, 0.2f, 0.08f), FLinearColor(0.8f, 0.65f, 0.25f) };

	const FLinearColor Skin = Skins[FMath::RandRange(0, 3)];
	USceneComponent* Root = GetCapsuleComponent();
	if (!bHasModel)
	{
		ResortVisuals::AddPart(this, Root, TEXT("Cylinder"), FVector(0.f, 0.f, -43.f), FVector(30.f, 30.f, 90.f), FRotator::ZeroRotator, Pants[FMath::RandRange(0, 3)], false);
		ResortVisuals::AddPart(this, Root, TEXT("Cylinder"), FVector(0.f, 0.f, 32.f), FVector(44.f, 32.f, 60.f), FRotator::ZeroRotator, Def.ShirtColor, false);
		ResortVisuals::AddPart(this, Root, TEXT("Cylinder"), FVector(0.f, 26.f, 30.f), FVector(10.f, 10.f, 55.f), FRotator::ZeroRotator, Skin, false);
		ResortVisuals::AddPart(this, Root, TEXT("Cylinder"), FVector(0.f, -26.f, 30.f), FVector(10.f, 10.f, 55.f), FRotator::ZeroRotator, Skin, false);
		ResortVisuals::AddPart(this, Root, TEXT("Sphere"), FVector(0.f, 0.f, 74.f), FVector(28.f, 28.f, 30.f), FRotator::ZeroRotator, Skin, false);
		ResortVisuals::AddPart(this, Root, TEXT("Sphere"), FVector(-1.f, 0.f, 82.f), FVector(27.f, 27.f, 15.f), FRotator::ZeroRotator, Hair[FMath::RandRange(0, 2)], false);
	}

	// Indicador de humor sobre a cabeca: verde feliz, amarelo ok, vermelho irritado
	MoodBall = ResortVisuals::AddPart(this, Root, TEXT("Sphere"), FVector(0.f, 0.f, 110.f), FVector(14.f, 14.f, 14.f), FRotator::ZeroRotator, FLinearColor(0.3f, 0.9f, 0.3f), false);

	if (UResortEconomy* Economy = AResortGameMode::GetEconomy(this))
	{
		Economy->OnGuestArrived();
		bCounted = true;
	}

	bInitialized = true;
	PickNextTarget();
}

void AGuestCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (State == EGuestState::Using && Target.IsValid())
	{
		Target->Release(RoomIndex);
	}
	if (bCounted)
	{
		if (UResortEconomy* Economy = AResortGameMode::GetEconomy(this))
		{
			Economy->OnGuestGone();
		}
		bCounted = false;
	}
	Super::EndPlay(EndPlayReason);
}

bool AGuestCharacter::MoveToward(const FVector& Destination, float AcceptRadius)
{
	FVector ToDest = Destination - GetActorLocation();
	ToDest.Z = 0.f;
	const float Dist = ToDest.Size();
	if (Dist <= AcceptRadius)
	{
		return true;
	}
	AddMovementInput(ToDest / Dist, 1.f);
	return false;
}

void AGuestCharacter::UpdateMood(bool bForce)
{
	const int32 Bucket = Satisfaction >= 0.6f ? 0 : (Satisfaction >= 0.3f ? 1 : 2);
	if (!bForce && Bucket == MoodBucket)
	{
		return;
	}
	MoodBucket = Bucket;
	if (MoodBall == nullptr)
	{
		return;
	}
	static const FLinearColor Colors[3] = {
		FLinearColor(0.2f, 0.9f, 0.25f), FLinearColor(0.95f, 0.8f, 0.15f), FLinearColor(0.95f, 0.15f, 0.1f) };
	if (UMaterialInstanceDynamic* Mid = Cast<UMaterialInstanceDynamic>(MoodBall->GetMaterial(0)))
	{
		Mid->SetVectorParameterValue(TEXT("Color"), Colors[Bucket]);
	}
}

void AGuestCharacter::Complain(const FString& Reason)
{
	// Nem toda reclamacao vira aviso, para nao poluir a tela
	if (FMath::FRand() > 0.5f)
	{
		return;
	}
	if (UResortEconomy* Economy = AResortGameMode::GetEconomy(this))
	{
		Economy->PushNotification(FString::Printf(TEXT("%s reclamou: %s"), *Def.DisplayName, *Reason),
			FLinearColor(1.f, 0.55f, 0.3f));
	}
}

void AGuestCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bInitialized)
	{
		return;
	}

	MoodTimer -= DeltaSeconds;
	if (MoodTimer <= 0.f)
	{
		MoodTimer = 0.5f;
		UpdateMood(false);
	}

	Age += DeltaSeconds;
	if (Age > MaxAge && State != EGuestState::Leaving)
	{
		StartLeaving();
	}

	switch (State)
	{
	case EGuestState::ToBuilding:
	{
		ABuildingActor* Building = Target.Get();
		if (Building == nullptr)
		{
			PickNextTarget();
			break;
		}
		const FVector Half = Building->GetFootprintHalf();
		const double Radius = Building->GetDef().IsEnclosed() ? FMath::Max(Half.X, Half.Y) + 40.0 : 90.0;
		if (MoveToward(Building->GetActorLocation(), Radius))
		{
			if (Building->TryReserveRoom(RoomIndex))
			{
				BeginUsing(Building);
			}
			else
			{
				State = EGuestState::Waiting;
				WaitTime = 0.f;
			}
		}
		break;
	}
	case EGuestState::Waiting:
	{
		ABuildingActor* Building = Target.Get();
		if (Building == nullptr)
		{
			PickNextTarget();
			break;
		}
		WaitTime += DeltaSeconds;
		if (Building->TryReserveRoom(RoomIndex))
		{
			BeginUsing(Building);
		}
		else if (WaitTime > 8.f)
		{
			// Cansou de esperar na fila
			Complain(FString::Printf(TEXT("fila muito longa em %s"), *Building->GetDef().DisplayName));
			Satisfaction = FMath::Max(0.f, Satisfaction - 0.15f);
			--VisitsLeft;
			PickNextTarget();
		}
		break;
	}
	case EGuestState::Using:
	{
		Timer -= DeltaSeconds;
		if (Timer <= 0.f)
		{
			FinishUsing();
		}
		break;
	}
	case EGuestState::Leaving:
	{
		const FVector Exit(-2950.f, GetActorLocation().Y, GetActorLocation().Z);
		if (MoveToward(Exit, 120.f))
		{
			if (UResortEconomy* Economy = AResortGameMode::GetEconomy(this))
			{
				Economy->OnGuestLeft(Satisfaction);
			}
			Destroy();
		}
		break;
	}
	}
}

void AGuestCharacter::PickNextTarget()
{
	if (VisitsLeft <= 0)
	{
		StartLeaving();
		return;
	}

	UResortEconomy* Economy = AResortGameMode::GetEconomy(this);
	if (Economy == nullptr)
	{
		StartLeaving();
		return;
	}

	TArray<ABuildingActor*> Candidates;
	TArray<float> Weights;
	float Total = 0.f;
	for (const TWeakObjectPtr<ABuildingActor>& Ptr : Economy->GetBuildings())
	{
		ABuildingActor* Building = Ptr.Get();
		if (Building == nullptr)
		{
			continue;
		}
		const FBuildingDef& D = Building->GetDef();
		if (Building->GetIncome() > Budget + 0.01f)
		{
			continue; // nao pode pagar
		}

		float Weight = 1.f;
		if (!Def.PreferredCategory.IsEmpty() && D.Category == Def.PreferredCategory)
		{
			Weight *= 2.5f;
		}
		Weight *= 1.f + 0.15f * (Building->GetStars() - 1) * (Def.BudgetMax >= 400.f ? 2.f : 1.f);
		if (VisitedIds.Contains(D.Id))
		{
			Weight *= 0.25f;
		}
		if (!Building->HasFreeSlot())
		{
			Weight *= 0.4f;
		}
		Candidates.Add(Building);
		Weights.Add(Weight);
		Total += Weight;
	}

	if (Candidates.Num() == 0)
	{
		if (VisitedIds.Num() == 0)
		{
			Satisfaction = FMath::Max(0.f, Satisfaction - 0.2f); // nao achou nada para fazer
			Complain(TEXT("não há nada para fazer aqui"));
		}
		StartLeaving();
		return;
	}

	float Roll = FMath::FRandRange(0.f, Total);
	Target = Candidates.Last();
	for (int32 i = 0; i < Candidates.Num(); ++i)
	{
		Roll -= Weights[i];
		if (Roll <= 0.f)
		{
			Target = Candidates[i];
			break;
		}
	}
	State = EGuestState::ToBuilding;
}

void AGuestCharacter::StartLeaving()
{
	State = EGuestState::Leaving;
	SetActorHiddenInGame(false);
}

void AGuestCharacter::BeginUsing(ABuildingActor* Building)
{
	State = EGuestState::Using;
	Timer = Building->GetServiceSeconds();
	if (Building->GetDef().IsEnclosed())
	{
		SetActorHiddenInGame(true); // "entrou" na construcao
	}
}

void AGuestCharacter::FinishUsing()
{
	SetActorHiddenInGame(false);
	ABuildingActor* Building = Target.Get();
	UResortEconomy* Economy = AResortGameMode::GetEconomy(this);

	if (Building != nullptr && Economy != nullptr)
	{
		const FBuildingDef& D = Building->GetDef();
		const float Pay = FMath::Min(Building->GetIncome(RoomIndex), Budget);
		Budget -= Pay;
		// Lojas de comida gastam 25% da venda em produtos
		Economy->AddMoney(Building->IsFoodShop() ? Pay * 0.75f : Pay);
		Satisfaction = FMath::Min(1.5f, Satisfaction + Building->GetSatisfaction());
		VisitedIds.Add(D.Id);
		Building->Release(RoomIndex);
		RoomIndex = -1;

		if (Pay >= 100.f)
		{
			Economy->PushNotification(
				FString::Printf(TEXT("%s gastou R$ %d em %s"), *Def.DisplayName, FMath::RoundToInt(Pay), *D.DisplayName),
				FLinearColor(0.6f, 1.f, 0.6f));
		}
	}
	else
	{
		// A construcao sumiu enquanto o hospede usava
		Satisfaction = FMath::Max(0.f, Satisfaction - 0.1f);
		Complain(TEXT("o lugar fechou enquanto usava"));
	}

	Target.Reset();
	--VisitsLeft;
	PickNextTarget();
}
