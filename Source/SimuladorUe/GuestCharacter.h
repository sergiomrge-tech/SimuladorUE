#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ResortTypes.h"
#include "BuildingActor.h"
#include "GuestCharacter.generated.h"

class UStaticMeshComponent;

enum class EGuestState : uint8
{
	ToBuilding,
	Waiting,
	Using,
	Leaving
};

/**
 * Hospede da praia. Escolhe uma construcao, anda ate ela, usa, paga e repete.
 * Quando termina (ou fica sem dinheiro) vai embora e a satisfacao vira prestigio.
 * Anda em linha reta (sem navmesh) e ignora as construcoes na colisao.
 */
UCLASS()
class SIMULADORUE_API AGuestCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AGuestCharacter();

	void InitGuest(const FGuestDef& InDef);

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void PickNextTarget();
	void StartLeaving();
	void BeginUsing(ABuildingActor* Building);
	void FinishUsing();
	/** Anda em direcao ao destino no plano XY. Retorna true ao chegar. */
	void UpdateMood(bool bForce);
	void Complain(const FString& Reason);
	bool MoveToward(const FVector& Destination, float AcceptRadius);

	FGuestDef Def;
	EGuestState State = EGuestState::ToBuilding;
	TWeakObjectPtr<ABuildingActor> Target;
	TArray<FString> VisitedIds;

	bool bInitialized = false;
	bool bCounted = false;
	float Budget = 0.f;
	float Satisfaction = 0.5f;
	int32 VisitsLeft = 2;
	float Timer = 0.f;
	float WaitTime = 0.f;
	float Age = 0.f;
	int32 MoodBucket = -1;
	float MoodTimer = 0.f;
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> MoodBall;

	/** Tempo maximo na praia antes de ir embora (segundos). */
	static constexpr float MaxAge = 240.f;
};
