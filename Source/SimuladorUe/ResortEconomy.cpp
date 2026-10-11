#include "ResortEconomy.h"
#include "ResortData.h"
#include "SimuladorUe.h"

namespace
{
	struct FMissionDef
	{
		const TCHAR* Text;
		int32 Kind; // 0 construcoes, 1 prestigio, 2 dinheiro, 3 hospedes atendidos, 4 nivel, 5 melhorias instaladas
		int32 Target;
		int32 Reward;
	};

	const FMissionDef Missions[] = {
		{ TEXT("Tenha 3 construções"), 0, 3, 150 },
		{ TEXT("Atenda 10 hóspedes"), 3, 10, 200 },
		{ TEXT("Instale 2 melhorias (olhe para uma construção e aperte 1-4)"), 5, 2, 300 },
		{ TEXT("Chegue ao nível 2: Barraca de Praia"), 4, 2, 500 },
		{ TEXT("Tenha 8 construções"), 0, 8, 600 },
		{ TEXT("Atenda 100 hóspedes"), 3, 100, 800 },
		{ TEXT("Junte R$ 5.000"), 2, 5000, 1000 },
		{ TEXT("Chegue ao nível 3: Beach Club"), 4, 3, 1500 },
		{ TEXT("Instale 10 melhorias"), 5, 10, 2000 },
		{ TEXT("Chegue ao nível 4: Pousada"), 4, 4, 4000 },
		{ TEXT("Atenda 1.000 hóspedes"), 3, 1000, 5000 },
		{ TEXT("Chegue ao nível 5: Resort"), 4, 5, 20000 },
	};
	constexpr int32 MissionCount = UE_ARRAY_COUNT(Missions);

	// Prestigio necessario para cada nivel (indice 0 = nivel 1).
	const float TierThreshold[5] = { 0.f, 60.f, 250.f, 800.f, 2000.f };
}

void UResortEconomy::Initialize()
{
	ResortData::LoadBuildings(BuildingDefs);
	ResortData::LoadGuests(GuestDefs);
	ResortData::LoadUpgrades(UpgradeDefs);
	ResortData::LoadStaffModels(StaffModels);

	Buildings.Reset();
	Notifications.Reset();
	Money = 1000.f;
	Prestige = 0.f;
	TodayEarned = 0.f;
	Day = 1;
	GuestCount = 0;
	Waiters = 0;
	Cleaners = 0;
	MissionIndex = 0;
	GuestsServed = 0;
	CurrentEvent = EResortEvent::None;
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

	MissionTimer -= DeltaSeconds;
	if (MissionTimer <= 0.f)
	{
		MissionTimer = 1.f;
		CheckMission();
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

const FUpgradeDef* UResortEconomy::FindUpgradeDef(const FString& Id) const
{
	for (const FUpgradeDef& U : UpgradeDefs)
	{
		if (U.Id == Id)
		{
			return &U;
		}
	}
	return nullptr;
}

TArray<const FUpgradeDef*> UResortEconomy::GetUpgradesFor(const FBuildingDef& Building) const
{
	TArray<const FUpgradeDef*> Out;
	for (const FUpgradeDef& U : UpgradeDefs)
	{
		if (U.AppliesTo(Building) && Out.Num() < 4)
		{
			Out.Add(&U);
		}
	}
	return Out;
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
	++GuestsServed;
	// Satisfacao acima de 0.45 rende prestigio.
	AddPrestige(FMath::Max(0.f, Satisfaction - 0.45f) * 1.f);
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

bool UResortEconomy::TryHire(bool bWaiter)
{
	int32& Count = bWaiter ? Waiters : Cleaners;
	if (Count >= MaxStaff)
	{
		PushNotification(TEXT("Equipe completa para essa função."), FLinearColor(1.f, 0.7f, 0.3f));
		return false;
	}
	if (!TrySpend(static_cast<float>(GetHireCost(bWaiter))))
	{
		PushNotification(TEXT("Dinheiro insuficiente para contratar."), FLinearColor(1.f, 0.5f, 0.4f));
		return false;
	}
	++Count;
	PushNotification(
		FString::Printf(TEXT("%s contratado (-R$ %d, salário R$ %d/dia)"),
			bWaiter ? TEXT("Garçom") : TEXT("Faxineiro"), GetHireCost(bWaiter), GetSalary(bWaiter)),
		FLinearColor(0.7f, 0.9f, 1.f));
	return true;
}

void UResortEconomy::SetStaff(int32 InWaiters, int32 InCleaners)
{
	Waiters = FMath::Clamp(InWaiters, 0, MaxStaff);
	Cleaners = FMath::Clamp(InCleaners, 0, MaxStaff);
}

float UResortEconomy::GetServiceMultiplier(const FBuildingDef& Building) const
{
	float Mult = Building.IsEnclosed() ? 1.f + 0.06f * Waiters : 1.f;
	if (CurrentEvent == EResortEvent::Heat && Building.Category == TEXT("food"))
	{
		Mult *= 1.2f; // calor: todo mundo quer bebida gelada
	}
	return Mult;
}

int32 UResortEconomy::CountUpgrades() const
{
	int32 Total = 0;
	for (const TWeakObjectPtr<ABuildingActor>& P : Buildings)
	{
		if (P.IsValid())
		{
			Total += P->GetUpgradeIds().Num();
		}
	}
	return Total;
}

bool UResortEconomy::HasMission() const
{
	return MissionIndex < MissionCount;
}

FString UResortEconomy::GetMissionText() const
{
	return HasMission() ? FString(Missions[MissionIndex].Text) : FString(TEXT("Todas as missões concluídas! Continue crescendo o seu resort."));
}

void UResortEconomy::GetMissionProgress(int32& Current, int32& Target, int32& Reward) const
{
	Current = 0;
	Target = 1;
	Reward = 0;
	if (!HasMission())
	{
		return;
	}
	const FMissionDef& M = Missions[MissionIndex];
	Target = M.Target;
	Reward = M.Reward;
	switch (M.Kind)
	{
	case 0: Current = GetBuildingCount(); break;
	case 1: Current = FMath::RoundToInt(Prestige); break;
	case 2: Current = FMath::RoundToInt(Money); break;
	case 3: Current = GuestsServed; break;
	case 4: Current = GetTier(); break;
	default: Current = CountUpgrades(); break;
	}
}

void UResortEconomy::CheckMission()
{
	if (!HasMission())
	{
		return;
	}
	int32 Current = 0, Target = 1, Reward = 0;
	GetMissionProgress(Current, Target, Reward);
	if (Current >= Target)
	{
		AddMoney(static_cast<float>(Reward));
		PushNotification(
			FString::Printf(TEXT("Missão cumprida: %s (+R$ %d)"), Missions[MissionIndex].Text, Reward),
			FLinearColor(1.f, 0.85f, 0.2f));
		++MissionIndex;
	}
}

FString UResortEconomy::GetEventName() const
{
	switch (CurrentEvent)
	{
	case EResortEvent::Rain: return TEXT("Dia de chuva: poucos hóspedes");
	case EResortEvent::Holiday: return TEXT("Feriado: praia lotada!");
	case EResortEvent::Heat: return TEXT("Onda de calor: bebidas vendem mais");
	case EResortEvent::Celebrity: return TEXT("Celebridade na praia: prestígio extra");
	default: return FString();
	}
}

float UResortEconomy::GetSpawnMultiplier() const
{
	switch (CurrentEvent)
	{
	case EResortEvent::Rain: return 0.5f;
	case EResortEvent::Holiday: return 1.8f;
	case EResortEvent::Celebrity: return 1.3f;
	default: return 1.f;
	}
}

void UResortEconomy::RollEvent()
{
	CurrentEvent = EResortEvent::None;
	if (Day < 3 || FMath::FRand() > 0.3f)
	{
		return;
	}
	CurrentEvent = static_cast<EResortEvent>(FMath::RandRange(1, 4));
	PushNotification(GetEventName(), FLinearColor(0.6f, 0.9f, 1.f));
	if (CurrentEvent == EResortEvent::Celebrity)
	{
		AddPrestige(8.f);
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
		Upkeep += P->GetUpkeep();
	}

	Upkeep += static_cast<float>(Waiters * GetSalary(true) + Cleaners * GetSalary(false));

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
			AddPrestige(-2.f);
			PushNotification(TEXT("Sem dinheiro para a manutenção! O prestígio caiu."), FLinearColor(1.f, 0.35f, 0.3f));
		}
	}
	TodayEarned = 0.f;
	RollEvent();
}
