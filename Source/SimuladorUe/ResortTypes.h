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

	/** Construcoes fechadas: o hospede "entra" e some enquanto usa. */
	bool IsEnclosed() const
	{
		return Category == TEXT("food") || Category == TEXT("lodging") || Category == TEXT("wellness");
	}
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
};
