#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"

/** Canal de colisao dos predios (definido em Config/DefaultEngine.ini como GameTraceChannel1). */
#define ECC_Building ECC_GameTraceChannel1

/** Definicao de uma construcao (carregada de Content/Data/Buildings.json). */
struct FBuildingDef
{
	FString Id;
	FString DisplayName;
	/** food, shade, entertainment, lodging, wellness */
	FString Category;
	/** Nivel do resort necessario para liberar (1 = quiosque ... 5 = resort). */
	int32 Tier = 1;
	int32 Cost = 100;
	float IncomePerVisit = 10.f;
	float ServiceSeconds = 5.f;
	int32 Capacity = 2;
	/** Quanto cada visita aumenta a satisfacao do hospede (0..1). */
	float Satisfaction = 0.1f;
	float UpkeepPerDay = 0.f;
	/** Tamanho aproximado em cm (X, Y, Z). */
	FVector Size = FVector(200.f, 200.f, 200.f);
	FLinearColor Color = FLinearColor::White;

	/**
	 * Modelo 3D final opcional (ex.: "/Game/Fab/Praia/SM_Quiosque.SM_Quiosque").
	 * Se vazio ou nao encontrado, usa as formas basicas provisorias.
	 * O tamanho (Size) continua valendo para colisao e espaco ocupado.
	 */
	FString MeshPath;
	float MeshScale = 1.f;
	float MeshYaw = 0.f;
	FVector MeshOffset = FVector(0.f, 0.f, 0.f);

	/** Construcoes fechadas: o hospede "entra" e some enquanto usa. */
	bool IsEnclosed() const
	{
		return Category == TEXT("food") || Category == TEXT("lodging") || Category == TEXT("wellness");
	}
};

/** Melhoria comprada por construcao (Content/Data/Upgrades.json): cama, armario, cardapio... */
struct FUpgradeDef
{
	FString Id;
	FString Name;
	/** Categoria da construcao que aceita a melhoria. */
	FString Category;
	/** Se preenchido, vale so para essa construcao (ex.: "cabin"). */
	FString BuildingId;
	/** Custo = custo da construcao x este fator. */
	float CostFactor = 0.25f;
	/** Aumento da renda por visita (0.2 = +20%). */
	float IncomeBonus = 0.15f;
	float SatisfactionBonus = 0.02f;
	/** Aumento da manutencao diaria, em fracao da manutencao base. */
	float UpkeepFactor = 0.1f;

	int32 CostFor(const FBuildingDef& B) const
	{
		return FMath::Max(10, FMath::RoundToInt(B.Cost * CostFactor / 10.f) * 10);
	}
	bool AppliesTo(const FBuildingDef& B) const
	{
		return Category == B.Category && (BuildingId.IsEmpty() || BuildingId == B.Id);
	}
};

/**
 * Modelo 3D de uma pessoa (hospede ou funcionario). Tudo opcional: se Mesh estiver vazio ou
 * nao for achado, usa o boneco provisorio de formas basicas.
 * Mesh: SkeletalMesh (ex.: "/Game/Fab/Pessoas/SK_Homem.SK_Homem")
 * AnimClass: Animation Blueprint (ex.: "/Game/Fab/Pessoas/ABP_Homem.ABP_Homem_C")
 */
struct FCharacterModel
{
	FString Mesh;
	FString AnimClass;
	float Scale = 1.f;
	/** Muitos modelos olham para o eixo Y; -90 alinha com a frente do personagem. */
	float Yaw = -90.f;
	/** Ajuste de altura (cm) se os pes ficarem enterrados ou flutuando. */
	float OffsetZ = 0.f;

	bool IsSet() const { return !Mesh.IsEmpty(); }
};

/** Definicao de um tipo de hospede (carregada de Content/Data/Guests.json). */
struct FGuestDef
{
	FString Id;
	FString DisplayName;
	int32 MinTier = 1;
	float BudgetMin = 50.f;
	float BudgetMax = 100.f;
	float WalkSpeed = 220.f;
	int32 MinVisits = 2;
	int32 MaxVisits = 3;
	/** Peso no sorteio de quem chega a praia. */
	float Weight = 1.f;
	FString PreferredCategory;
	FLinearColor ShirtColor = FLinearColor(0.2f, 0.5f, 0.9f);
	FCharacterModel Model;
};
