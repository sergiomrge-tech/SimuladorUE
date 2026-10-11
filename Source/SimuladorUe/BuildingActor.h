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

	bool HasFreeSlot() const { return Occupants < GetCapacity(); }

	// ---- Atendentes e caixa (so quiosque, bar e restaurante) ----
	bool IsFoodShop() const { return Def.Category == TEXT("food"); }
	int32 GetAttendants() const { return Attendants; }
	bool HasRegister() const { return bRegister; }
	/** Quantos clientes sao atendidos ao mesmo tempo. */
	int32 GetCapacity() const { return IsFoodShop() ? FMath::Min(Def.Capacity, 1 + Attendants) : Def.Capacity; }
	/** Tempo de atendimento: o caixa agiliza em 30%. */
	float GetServiceSeconds() const { return Def.ServiceSeconds * (bRegister ? 0.7f : 1.f); }
	int32 GetAttendantCost() const { return 150; }
	int32 GetRegisterCost() const { return FMath::Max(10, FMath::RoundToInt(Def.Cost * 0.3f / 10.f) * 10); }
	bool CanHireAttendant() const { return IsFoodShop() && Attendants < Def.Capacity; }
	/** Define o numero de atendentes (sem cobrar) e cria os bonecos que faltam. */
	void SetAttendants(int32 Count);
	void InstallRegister();
	bool TryReserve();
	void Release();
	int32 GetOccupants() const { return Occupants; }

	// ---- Melhorias (cama, armario, cardapio...) ----
	bool HasUpgrade(const FString& UpgradeId) const { return UpgradeIds.Contains(UpgradeId); }
	const TArray<FString>& GetUpgradeIds() const { return UpgradeIds; }
	/** Aplica a melhoria (sem cobrar). Retorna false se ja tinha. */
	bool ApplyUpgrade(const FUpgradeDef& Upgrade);
	/** Renda por visita ja com as melhorias (para quartos, a diaria). */
float GetIncome() const;
	float GetSatisfaction() const;
	float GetUpkeep() const { return Def.UpkeepPerDay * (1.f + UpkeepBonus) + (IsFoodShop() ? 15.f * FMath::Max(0, Attendants - 1) : 0.f); }
	/** Estrelas (1 a 5): sobem a cada melhoria instalada e atraem hospedes mais ricos. */
	int32 GetStars() const { return FMath::Clamp(1 + UpgradeIds.Num(), 1, 5); }
	/** Total gasto em melhorias nesta construcao. */
	int32 GetInvested() const { return Invested; }

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
	TArray<FString> UpgradeIds;
	float IncomeBonus = 0.f;
	float SatisfactionBonus = 0.f;
	float UpkeepBonus = 0.f;
	int32 Invested = 0;
	int32 Attendants = 0;
	int32 AttendantsShown = 0;
	bool bRegister = false;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Parts;
};
