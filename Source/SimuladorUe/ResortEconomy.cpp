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
		{ TEXT("Chegue ao nível 3"), 6, 3, 400 },
		{ TEXT("Tenha 6 construções"), 0, 6, 500 },
		{ TEXT("Atenda 50 hóspedes"), 3, 50, 600 },
		{ TEXT("Chegue ao nível 5: Barraca de Praia"), 6, 5, 1000 },
		{ TEXT("Junte R$ 5.000"), 2, 5000, 1000 },
		{ TEXT("Tenha 10 construções"), 0, 10, 1500 },
		{ TEXT("Instale 8 melhorias"), 5, 8, 2000 },
		{ TEXT("Atenda 250 hóspedes"), 3, 250, 2500 },
		{ TEXT("Chegue ao nível 7"), 6, 7, 3000 },
		{ TEXT("Chegue ao nível 9: Beach Club"), 6, 9, 6000 },
		{ TEXT("Tenha 16 construções"), 0, 16, 6000 },
		{ TEXT("Junte R$ 50.000"), 2, 50000, 8000 },
		{ TEXT("Atenda 1.000 hóspedes"), 3, 1000, 10000 },
		{ TEXT("Instale 20 melhorias"), 5, 20, 12000 },
		{ TEXT("Chegue ao nível 11"), 6, 11, 15000 },
		{ TEXT("Chegue ao nível 13: Pousada"), 6, 13, 25000 },
		{ TEXT("Tenha 24 construções"), 0, 24, 25000 },
		{ TEXT("Atenda 3.000 hóspedes"), 3, 3000, 30000 },
		{ TEXT("Instale 40 melhorias"), 5, 40, 40000 },
		{ TEXT("Chegue ao nível 15"), 6, 15, 50000 },
		{ TEXT("Junte R$ 500.000"), 2, 500000, 60000 },
		{ TEXT("Chegue ao nível 17: Resort"), 6, 17, 100000 },
		{ TEXT("Tenha 34 construções"), 0, 34, 100000 },
		{ TEXT("Atenda 8.000 hóspedes"), 3, 8000, 120000 },
		{ TEXT("Instale 70 melhorias"), 5, 70, 150000 },
		{ TEXT("Chegue ao nível 19"), 6, 19, 250000 },
		{ TEXT("Atenda 15.000 hóspedes"), 3, 15000, 300000 },
		{ TEXT("Chegue ao nível 20: Resort Lendário"), 6, 20, 1000000 },
	};
	constexpr int32 MissionCount = UE_ARRAY_COUNT(Missions);

	// Prestigio necessario para cada nivel (indice 0 = nivel 1).
	// Prestigio para cada um dos 20 niveis (indice 0 = nivel 1). Os niveis 5, 9, 13 e 17 abrem novas
	// fases (construcoes e hospedes); os demais dao premios em dinheiro.
	const float LevelThreshold[20] = {
		0.f, 25.f, 96.f, 243.f, 435.f,
		725.f, 1050.f, 1395.f, 1757.f, 2176.f,
		2602.f, 3076.f, 3526.f, 4029.f, 4532.f,
		5059.f, 5588.f, 6141.f, 6730.f, 7317.f };

	const TCHAR* LevelNames[20] = {
		TEXT("Quiosque Simples"), TEXT("Quiosque Movimentado"), TEXT("Quiosque Popular"), TEXT("Ponto da Praia"),
		TEXT("Barraca de Praia"), TEXT("Barraca Animada"), TEXT("Barraca Premium"), TEXT("Bar da Praia"),
		TEXT("Beach Club"), TEXT("Beach Club Animado"), TEXT("Clube de Praia"), TEXT("Clube Exclusivo"),
		TEXT("Pousada"), TEXT("Pousada Charmosa"), TEXT("Pousada Boutique"), TEXT("Hotel Boutique"),
		TEXT("Resort"), TEXT("Resort Cinco Estrelas"), TEXT("Resort de Luxo"), TEXT("Resort Lendário") };
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
	LastLevel = 1;
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

	const int32 NowLevel = GetLevel();
	if (NowLevel > LastLevel)
	{
		LastLevel = NowLevel;
		const int32 Bonus = NowLevel * NowLevel * 50;
		AddMoney(static_cast<float>(Bonus));
		PushNotification(
			FString::Printf(TEXT("Nível %d: %s! (+R$ %d)"), NowLevel, *GetLevelName(NowLevel), Bonus),
			FLinearColor(1.f, 0.85f, 0.2f));
		for (const FBuildingDef& D : BuildingDefs)
		{
			if (D.MinLevel == NowLevel)
			{
				PushNotification(FString::Printf(TEXT("Desbloqueado: %s"), *D.DisplayName), FLinearColor(0.6f, 1.f, 0.9f));
			}
		}
		for (const FGuestDef& G : GuestDefs)
		{
			if (G.MinLevel == NowLevel)
			{
				PushNotification(FString::Printf(TEXT("Novo tipo de hóspede: %s"), *G.DisplayName), FLinearColor(0.9f, 0.8f, 1.f));
			}
		}
	}
	const int32 NowTier = GetTier();
	if (NowTier > LastTier)
	{
		LastTier = NowTier;
		PushNotification(
			FString::Printf(TEXT("Nova fase: %s! Novas construções e hóspedes liberados."), *GetTierName(NowTier)),
			FLinearColor(0.6f, 1.f, 0.9f));
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

int32 UResortEconomy::GetLevel() const
{
	int32 Level = 1;
	for (int32 i = 1; i < MaxLevel; ++i)
	{
		if (Prestige >= LevelThreshold[i])
		{
			Level = i + 1;
		}
	}
	return Level;
}

FString UResortEconomy::GetLevelName(int32 Level) const
{
	return LevelNames[FMath::Clamp(Level, 1, MaxLevel) - 1];
}

int32 UResortEconomy::GetTier() const
{
	return FMath::Clamp((GetLevel() - 1) / 4 + 1, 1, 5);
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
	return LevelThreshold[GetLevel() - 1];
}

float UResortEconomy::GetNextTierPrestige() const
{
	const int32 Level = GetLevel();
	return Level >= MaxLevel ? -1.f : LevelThreshold[Level];
}

float UResortEconomy::GetNextUnlockPrestige() const
{
	const int32 Tier = GetTier();
	return Tier >= GetMaxTier() ? -1.f : LevelThreshold[Tier * 4];
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
	const int32 Level = GetLevel();
	for (const FBuildingDef& D : BuildingDefs)
	{
		if (D.MinLevel <= Level)
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
	AddPrestige(FMath::Max(0.f, Satisfaction - 0.45f) * 0.25f);
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
	case 6: Current = GetLevel(); break;
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
	LastLevel = GetLevel();
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
