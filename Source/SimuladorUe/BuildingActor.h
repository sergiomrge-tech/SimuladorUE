#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ResortTypes.h"
#include "BuildingActor.generated.h"

class UStaticMeshComponent;

enum class EBuildingMode : uint8
{
	/** Construcao de verdade: tem colisao, rende dinheiro, entra na economia. */
	Placed,
	/** Previa verde/vermelha do modo de construcao (sem colisao). */
	Ghost,
	/** Enfeite (palmeiras): colisao, mas fora da economia. */
	Decor
};

/**
 * Uma construcao do resort. O visual e montado em tempo de execucao a partir
 * de formas basicas (provisorio) conforme a definicao FBuildingDef.
 */
UCLASS()
class SIMULADORUE_API ABuildingActor : public AActor
{
	GENERATED_BODY()

public:
	ABuildingActor();

	void InitFromDef(const FBuildingDef& InDef, EBuildingMode InMode);

	const FBuildingDef& GetDef() const { return Def; }
	EBuildingMode GetMode() const { return Mode; }

	/** Pinta a previa de verde (pode construir) ou vermelho (nao pode). */
	void SetGhostValid(bool bValid);

	bool HasFreeSlot() const { return Occupants < Def.Capacity; }
	bool TryReserve();
	void Release();
	int32 GetOccupants() const { return Occupants; }

	/** Metade do tamanho ocupado no chao (inclui uma folga de beiral). */
	FVector GetFootprintHalf() const;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void BuildVisuals();
	UStaticMeshComponent* Part(const TCHAR* Shape, const FVector& Loc, const FVector& SizeCm,
		const FLinearColor& Color, bool bBlocking = true, const FRotator& Rot = FRotator::ZeroRotator);

	FBuildingDef Def;
	EBuildingMode Mode = EBuildingMode::Placed;
	int32 Occupants = 0;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Parts;
};
