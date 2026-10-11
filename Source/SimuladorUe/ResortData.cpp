#include "ResortData.h"
#include "SimuladorUe.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	bool LoadJsonArray(const FString& FileName, const FString& Field, TArray<TSharedPtr<FJsonValue>>& Out)
	{
		const FString Path = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data"), FileName);
		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *Path))
		{
			UE_LOG(LogResort, Warning, TEXT("Arquivo de dados nao encontrado: %s (usando valores padrao)"), *Path);
			return false;
		}

		TSharedPtr<FJsonObject> Root;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
		if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
		{
			UE_LOG(LogResort, Warning, TEXT("JSON invalido em %s (usando valores padrao)"), *Path);
			return false;
		}

		const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
		if (!Root->TryGetArrayField(Field, Arr) || Arr == nullptr)
		{
			UE_LOG(LogResort, Warning, TEXT("Campo '%s' ausente em %s (usando valores padrao)"), *Field, *Path);
			return false;
		}

		Out = *Arr;
		return true;
	}

	float GetNumber(const TSharedPtr<FJsonObject>& Obj, const FString& Key, float DefaultValue)
	{
		double Value = 0.0;
		return Obj->TryGetNumberField(Key, Value) ? static_cast<float>(Value) : DefaultValue;
	}

	FString GetString(const TSharedPtr<FJsonObject>& Obj, const FString& Key, const FString& DefaultValue)
	{
		FString Value;
		return Obj->TryGetStringField(Key, Value) ? Value : DefaultValue;
	}

	bool GetTriple(const TSharedPtr<FJsonObject>& Obj, const FString& Key, float& A, float& B, float& C)
	{
		const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
		if (!Obj->TryGetArrayField(Key, Arr) || Arr == nullptr || Arr->Num() < 3)
		{
			return false;
		}
		A = static_cast<float>((*Arr)[0]->AsNumber());
		B = static_cast<float>((*Arr)[1]->AsNumber());
		C = static_cast<float>((*Arr)[2]->AsNumber());
		return true;
	}
}

void ResortData::LoadBuildings(TArray<FBuildingDef>& Out)
{
	Out.Reset();

	TArray<TSharedPtr<FJsonValue>> Items;
	if (LoadJsonArray(TEXT("Buildings.json"), TEXT("Buildings"), Items))
	{
		for (const TSharedPtr<FJsonValue>& Item : Items)
		{
			const TSharedPtr<FJsonObject> Obj = Item.IsValid() ? Item->AsObject() : nullptr;
			if (!Obj.IsValid())
			{
				continue;
			}

			FBuildingDef D;
			D.Id = GetString(Obj, TEXT("Id"), TEXT(""));
			if (D.Id.IsEmpty())
			{
				continue;
			}
			D.DisplayName = GetString(Obj, TEXT("DisplayName"), D.Id);
			D.Category = GetString(Obj, TEXT("Category"), TEXT("shade"));
			D.Tier = FMath::Max(1, FMath::RoundToInt(GetNumber(Obj, TEXT("Tier"), 1.f)));
			D.Cost = FMath::Max(0, FMath::RoundToInt(GetNumber(Obj, TEXT("Cost"), 100.f)));
			D.IncomePerVisit = GetNumber(Obj, TEXT("IncomePerVisit"), 10.f);
			D.ServiceSeconds = FMath::Max(0.5f, GetNumber(Obj, TEXT("ServiceSeconds"), 5.f));
			D.Capacity = FMath::Max(1, FMath::RoundToInt(GetNumber(Obj, TEXT("Capacity"), 2.f)));
			D.Satisfaction = GetNumber(Obj, TEXT("Satisfaction"), 0.1f);
			D.UpkeepPerDay = GetNumber(Obj, TEXT("UpkeepPerDay"), 0.f);

			float X = 200.f, Y = 200.f, Z = 200.f;
			if (GetTriple(Obj, TEXT("Size"), X, Y, Z))
			{
				D.Size = FVector(X, Y, Z);
			}
			float R = 1.f, G = 1.f, B = 1.f;
			if (GetTriple(Obj, TEXT("Color"), R, G, B))
			{
				D.Color = FLinearColor(R, G, B);
			}

			D.MeshPath = GetString(Obj, TEXT("Mesh"), TEXT(""));
			D.MeshScale = GetNumber(Obj, TEXT("MeshScale"), 1.f);
			D.MeshYaw = GetNumber(Obj, TEXT("MeshYaw"), 0.f);
			float OX = 0.f, OY = 0.f, OZ = 0.f;
			if (GetTriple(Obj, TEXT("MeshOffset"), OX, OY, OZ))
			{
				D.MeshOffset = FVector(OX, OY, OZ);
			}
			Out.Add(D);
		}
	}

	if (Out.Num() == 0)
	{
		FBuildingDef Kiosk;
		Kiosk.Id = TEXT("kiosk");
		Kiosk.DisplayName = TEXT("Quiosque");
		Kiosk.Category = TEXT("food");
		Kiosk.Tier = 1;
		Kiosk.Cost = 700;
		Kiosk.IncomePerVisit = 14.f;
		Kiosk.ServiceSeconds = 4.f;
		Kiosk.Capacity = 3;
		Kiosk.Satisfaction = 0.12f;
		Kiosk.UpkeepPerDay = 8.f;
		Kiosk.Size = FVector(300.f, 260.f, 260.f);
		Kiosk.Color = FLinearColor(0.78f, 0.52f, 0.28f);
		Out.Add(Kiosk);

		FBuildingDef Umbrella;
		Umbrella.Id = TEXT("umbrella");
		Umbrella.DisplayName = TEXT("Guarda-sol");
		Umbrella.Category = TEXT("shade");
		Umbrella.Tier = 1;
		Umbrella.Cost = 120;
		Umbrella.IncomePerVisit = 4.f;
		Umbrella.ServiceSeconds = 14.f;
		Umbrella.Capacity = 4;
		Umbrella.Satisfaction = 0.10f;
		Umbrella.UpkeepPerDay = 1.f;
		Umbrella.Size = FVector(240.f, 240.f, 230.f);
		Umbrella.Color = FLinearColor(0.95f, 0.25f, 0.2f);
		Out.Add(Umbrella);
	}
}

void ResortData::LoadGuests(TArray<FGuestDef>& Out)
{
	Out.Reset();

	TArray<TSharedPtr<FJsonValue>> Items;
	if (LoadJsonArray(TEXT("Guests.json"), TEXT("Guests"), Items))
	{
		for (const TSharedPtr<FJsonValue>& Item : Items)
		{
			const TSharedPtr<FJsonObject> Obj = Item.IsValid() ? Item->AsObject() : nullptr;
			if (!Obj.IsValid())
			{
				continue;
			}

			FGuestDef D;
			D.Id = GetString(Obj, TEXT("Id"), TEXT(""));
			if (D.Id.IsEmpty())
			{
				continue;
			}
			D.DisplayName = GetString(Obj, TEXT("DisplayName"), D.Id);
			D.MinTier = FMath::Max(1, FMath::RoundToInt(GetNumber(Obj, TEXT("MinTier"), 1.f)));
			D.BudgetMin = GetNumber(Obj, TEXT("BudgetMin"), 50.f);
			D.BudgetMax = FMath::Max(D.BudgetMin, GetNumber(Obj, TEXT("BudgetMax"), 100.f));
			D.WalkSpeed = GetNumber(Obj, TEXT("WalkSpeed"), 220.f);
			D.MinVisits = FMath::Max(1, FMath::RoundToInt(GetNumber(Obj, TEXT("MinVisits"), 2.f)));
			D.MaxVisits = FMath::Max(D.MinVisits, FMath::RoundToInt(GetNumber(Obj, TEXT("MaxVisits"), 3.f)));
			D.Weight = FMath::Max(0.01f, GetNumber(Obj, TEXT("Weight"), 1.f));
			D.PreferredCategory = GetString(Obj, TEXT("PreferredCategory"), TEXT(""));

			float R = 0.2f, G = 0.5f, B = 0.9f;
			if (GetTriple(Obj, TEXT("ShirtColor"), R, G, B))
			{
				D.ShirtColor = FLinearColor(R, G, B);
			}
			Out.Add(D);
		}
	}

	if (Out.Num() == 0)
	{
		FGuestDef Tourist;
		Tourist.Id = TEXT("turista");
		Tourist.DisplayName = TEXT("Turista");
		Tourist.MinTier = 1;
		Tourist.BudgetMin = 40.f;
		Tourist.BudgetMax = 110.f;
		Tourist.PreferredCategory = TEXT("shade");
		Out.Add(Tourist);
	}
}

void ResortData::LoadUpgrades(TArray<FUpgradeDef>& Out)
{
	Out.Reset();
	TArray<TSharedPtr<FJsonValue>> Items;
	if (!LoadJsonArray(TEXT("Upgrades.json"), TEXT("Upgrades"), Items))
	{
		return;
	}
	for (const TSharedPtr<FJsonValue>& Item : Items)
	{
		const TSharedPtr<FJsonObject> Obj = Item.IsValid() ? Item->AsObject() : nullptr;
		if (!Obj.IsValid())
		{
			continue;
		}
		FUpgradeDef U;
		U.Id = GetString(Obj, TEXT("Id"), TEXT(""));
		if (U.Id.IsEmpty())
		{
			continue;
		}
		U.Name = GetString(Obj, TEXT("Name"), U.Id);
		U.Category = GetString(Obj, TEXT("Category"), TEXT(""));
		U.BuildingId = GetString(Obj, TEXT("BuildingId"), TEXT(""));
		U.CostFactor = GetNumber(Obj, TEXT("CostFactor"), 0.25f);
		U.IncomeBonus = GetNumber(Obj, TEXT("IncomeBonus"), 0.15f);
		U.SatisfactionBonus = GetNumber(Obj, TEXT("SatisfactionBonus"), 0.02f);
		U.UpkeepFactor = GetNumber(Obj, TEXT("UpkeepFactor"), 0.1f);
		Out.Add(U);
	}
}
