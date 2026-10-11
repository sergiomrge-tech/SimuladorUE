#include "ResortEconomy.h"
#include "ResortData.h"
#include "SimuladorUe.h"

namespace
{
	// Prestigio necessario para cada nivel (indice 0 = nivel 1).
	const float TierThreshold[5] = { 0.f, 40.f, 120.f, 300.f, 700.f };
}

void UResortEconomy::Initialize()
{
	ResortData::LoadBuildings(BuildingDefs);
	ResortData::LoadGuests(GuestDefs);

	Buildings.Reset();
	Notifications.Reset();
	Money = 1000.f;
	Prestige = 0.f;
	TodayEarned = 0.f;
	Day = 1;
	GuestCount = 0;
	LastTier = 1;
	DayClock = 0.f;

	UE_LOG(LogResort, Log, TEXT("Economia iniciada: %d construcoes, %d tipos de hospede"), BuildingDefs.Num(), GuestDefs.Num());
}

void UResortEconomy::Tick(float DeltaSeconds)
{
	DayClock += DeltaSeconds;
	if (DayClock >= DayLengthSeconds)
	{
		DayClock -= DayLengthSeconds;
		EndDay();
	}

	for (int32 i = Notifications.Num() - 1; i >= 0; --i)
	{
		Notifications[i].TimeLeft -= DeltaSeconds;
		if (Notifications[i].TimeLeft <= 0.f)
		{
			Notifications.RemoveAt(i);
		}
	}

	const int32 NowTier = GetTier();
	if (NowTier > LastTier)
	{
		LastTier = NowTier;
		PushNotification(
			FString::Printf(TEXT("Novo nível: %s! Novas construções liberadas."), *GetTierName(NowTier)),
			FLinearColor(1.f, 0.85f, 0.2f));
	}
}

bool UResortEconomy::TrySpend(float Amount)
{
	if (Money < Amount)
	{
		return false;
	}
	Money -= Amount;
	return true;
}

void UResortEconomy::AddMoney(float Amount)
{
	Money += Amount;
	if (Amount > 0.f)
	{
		TodayEarned += Amount;
	}
}

void UResortEconomy::AddPrestige(float Amount)
{
	Prestige = FMath::Max(0.f, Prestige + Amount);
}

int32 UResortEconomy::GetTier() const
{
	int32 Tier = 1;
	for (int32 i = 1; i < 5; ++i)
	{
		if (Prestige >= TierThreshold[i])
		{
			Tier = i + 1;
		}
	}
	return Tier;
}

FString UResortEconomy::GetTierName(int32 Tier) const
{
	switch (Tier)
	{
	case 1: return TEXT("Quiosque");
	case 2: return TEXT("Barraca de Praia");
	case 3: return TEXT("Beach Club");
	case 4: return TEXT("Pousada");
	default: return TEXT("Resort");
	}
}

float UResortEconomy::GetTierStartPrestige() const
{
	return TierThreshold[FMath::Clamp(GetTier() - 1, 0, 4)];
}

float UResortEconomy::GetNextTierPrestige() const
{
	const int32 Tier = GetTier();
	return Tier >= GetMaxTier() ? -1.f : TierThreshold[Tier];
}

const FBuildingDef* UResortEconomy::FindBuildingDef(const FString& Id) const
{
	for (const FBuildingDef& D : BuildingDefs)
	{
		if (D.Id == Id)
		{
			return &D;
		}
	}
	return nullptr;
}

TArray<const FBuildingDef*> UResortEconomy::GetUnlockedBuildings() const
{
	TArray<const FBuildingDef*> Result;
	const int32 Tier = GetTier();
	for (const FBuildingDef& D : BuildingDefs)
	{
		if (D.Tier <= Tier)
		{
			Result.Add(&D);
		}
	}
	return Result;
}

void UResortEconomy::RegisterBuilding(ABuildingActor* Building)
{
	if (Building != nullptr)
	{
		Buildings.AddUnique(Building);
	}
}

void UResortEconomy::UnregisterBuilding(ABuildingActor* Building)
{
	Buildings.RemoveAll([Building](const TWeakObjectPtr<ABuildingActor>& P)
	{
		return !P.IsValid() || P.Get() == Building;
	});
}

void UResortEconomy::OnGuestLeft(float Satisfaction)
{
	// Satisfacao acima de 0.45 rende prestigio.
	AddPrestige(FMath::Max(0.f, Satisfaction - 0.45f) * 14.f);
}

void UResortEconomy::PushNotification(const FString& Text, const FLinearColor& Color)
{
	FResortNotification N;
	N.Text = Text;
	N.Color = Color;
	N.TimeLeft = 7.f;
	Notifications.Add(N);
	while (Notifications.Num() > 5)
	{
		Notifications.RemoveAt(0);
	}
}

void UResortEconomy::SetState(float InMoney, float InPrestige, int32 InDay, float InDayClock)
{
	Money = FMath::Max(0.f, InMoney);
	Prestige = FMath::Max(0.f, InPrestige);
	Day = FMath::Max(1, InDay);
	DayClock = FMath::Clamp(InDayClock, 0.f, DayLengthSeconds);
	TodayEarned = 0.f;
	LastTier = GetTier();
}

void UResortEconomy::EndDay()
{
	float Upkeep = 0.f;
	Buildings.RemoveAll([](const TWeakObjectPtr<ABuildingActor>& P) { return !P.IsValid(); });
	for (const TWeakObjectPtr<ABuildingActor>& P : Buildings)
	{
		Upkeep += P->GetDef().UpkeepPerDay;
	}

	const int32 FinishedDay = Day;
	++Day;

	if (Upkeep > 0.f)
	{
		if (Money >= Upkeep)
		{
			Money -= Upkeep;
			PushNotification(
				FString::Printf(TEXT("Dia %d encerrado: lucro R$ %d, manutenção -R$ %d"),
					FinishedDay, FMath::RoundToInt(TodayEarned), FMath::RoundToInt(Upkeep)),
				FLinearColor(0.8f, 0.9f, 1.f));
		}
		else
		{
			Money = 0.f;
			AddPrestige(-3.f);
			PushNotification(TEXT("Sem dinheiro para a manutenção! O prestígio caiu."), FLinearColor(1.f, 0.35f, 0.3f));
		}
	}
	TodayEarned = 0.f;
}
