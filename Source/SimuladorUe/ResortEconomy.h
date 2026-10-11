#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ResortTypes.h"
#include "BuildingActor.h"
#include "ResortEconomy.generated.h"

struct FResortNotification
{
	FString Text;
	FLinearColor Color = FLinearColor::White;
	float TimeLeft = 6.f;
};

/**
 * Estado do resort: dinheiro, prestigio, nivel, relogio do dia e lista de
 * construcoes. Vive dentro do GameMode (um por partida).
 */
/** Evento do dia (sorteado no fim de cada dia, a partir do dia 3). */
enum class EResortEvent : uint8
{
	None,
	Rain,
	Holiday,
	Heat,
	Celebrity
};

UCLASS()
class SIMULADORUE_API UResortEconomy : public UObject
{
	GENERATED_BODY()

public:
	/** Carrega os dados (JSON) e zera o estado. */
	void Initialize();
	void Tick(float DeltaSeconds);

	// ---- Dinheiro e prestigio ----
	float GetMoney() const { return Money; }
	bool CanAfford(float Amount) const { return Money >= Amount; }
	bool TrySpend(float Amount);
	void AddMoney(float Amount);
	float GetPrestige() const { return Prestige; }
	void AddPrestige(float Amount);
	float GetTodayEarned() const { return TodayEarned; }

	// ---- 20 niveis de evolucao (1 = Quiosque Simples ... 20 = Resort Lendario) ----
	static constexpr int32 MaxLevel = 20;
	int32 GetLevel() const;
	FString GetLevelName(int32 Level) const;
	/** Prestigio necessario para o proximo grupo de construcoes (proxima fase), ou -1 no maximo. */
	float GetNextUnlockPrestige() const;

	// ---- Fase do resort (1 = Quiosque ... 5 = Resort): libera construcoes e hospedes; muda a cada 4 niveis ----
	int32 GetTier() const;
	static int32 GetMaxTier() { return 5; }
	FString GetTierName(int32 Tier) const;
	/** Prestigio necessario para o nivel atual. */
	float GetTierStartPrestige() const;
	/** Prestigio necessario para o proximo nivel, ou -1 se ja esta no maximo. */
	float GetNextTierPrestige() const;

	// ---- Missoes ----
	bool HasMission() const;
	FString GetMissionText() const;
	/** Progresso atual e meta da missao, e a recompensa em dinheiro. */
	void GetMissionProgress(int32& Current, int32& Target, int32& Reward) const;
	int32 GetMissionIndex() const { return MissionIndex; }
	void SetMissionState(int32 InIndex, int32 InGuestsServed) { MissionIndex = FMath::Max(0, InIndex); GuestsServed = FMath::Max(0, InGuestsServed); }
	int32 GetGuestsServed() const { return GuestsServed; }

	// ---- Eventos do dia ----
	EResortEvent GetEvent() const { return CurrentEvent; }
	FString GetEventName() const;
	/** Multiplicador da chegada de hospedes (chuva diminui, feriado aumenta). */
	float GetSpawnMultiplier() const;

	// ---- Equipe ----
	static constexpr int32 MaxStaff = 5;
	int32 GetWaiters() const { return Waiters; }
	int32 GetCleaners() const { return Cleaners; }
	int32 GetHireCost(bool bWaiter) const { return bWaiter ? 300 : 250; }
	int32 GetSalary(bool bWaiter) const { return bWaiter ? 25 : 20; }
	bool TryHire(bool bWaiter);
	void SetStaff(int32 InWaiters, int32 InCleaners);
	/** Garcons aumentam a renda de comida, hospedagem e spa. */
	float GetServiceMultiplier(const FBuildingDef& Building) const;
	/** Faxineiros aumentam a satisfacao de cada visita. */
	float GetCleanBonus() const { return 0.012f * Cleaners; }

	// ---- Relogio ----
	int32 GetDay() const { return Day; }
	float GetDayFraction() const { return DayLengthSeconds > 0.f ? DayClock / DayLengthSeconds : 0.f; }
	float GetHourOfDay() const { return StartHour + GetDayFraction() * (EndHour - StartHour); }

	/** Usado pelo salvar/carregar. */
	float GetDayClock() const { return DayClock; }
	void SetState(float InMoney, float InPrestige, int32 InDay, float InDayClock);

	// ---- Dados ----
	const TArray<FBuildingDef>& GetBuildingDefs() const { return BuildingDefs; }
	const TArray<FGuestDef>& GetGuestDefs() const { return GuestDefs; }
	const FBuildingDef* FindBuildingDef(const FString& Id) const;
	/** Modelo 3D da funcao ("attendant", "waiter", "cleaner") ou nullptr para usar o boneco provisorio. */
	const FCharacterModel* FindStaffModel(const FString& Role) const { return StaffModels.Find(Role); }
	const FUpgradeDef* FindUpgradeDef(const FString& Id) const;
	/** Melhorias disponiveis para essa construcao (maximo 4). */
	TArray<const FUpgradeDef*> GetUpgradesFor(const FBuildingDef& Building) const;
	/** Construcoes liberadas no nivel atual. */
	TArray<const FBuildingDef*> GetUnlockedBuildings() const;

	// ---- Construcoes em jogo ----
	void RegisterBuilding(ABuildingActor* Building);
	void UnregisterBuilding(ABuildingActor* Building);
	const TArray<TWeakObjectPtr<ABuildingActor>>& GetBuildings() const { return Buildings; }
	int32 GetBuildingCount() const { return Buildings.Num(); }

	// ---- Hospedes ----
	int32 GetGuestCount() const { return GuestCount; }
	void OnGuestArrived() { ++GuestCount; }
	void OnGuestGone() { GuestCount = FMath::Max(0, GuestCount - 1); }
	/** Chamado quando um hospede vai embora: satisfacao vira prestigio. */
	void OnGuestLeft(float Satisfaction);

	// ---- Avisos na tela ----
	void PushNotification(const FString& Text, const FLinearColor& Color = FLinearColor::White);
	const TArray<FResortNotification>& GetNotifications() const { return Notifications; }

private:
	void EndDay();
	void CheckMission();
	void RollEvent();
	int32 CountUpgrades() const;

	TArray<FBuildingDef> BuildingDefs;
	TArray<FGuestDef> GuestDefs;
	TArray<FUpgradeDef> UpgradeDefs;
	TMap<FString, FCharacterModel> StaffModels;
	TArray<TWeakObjectPtr<ABuildingActor>> Buildings;
	TArray<FResortNotification> Notifications;

	float Money = 1000.f;
	float Prestige = 0.f;
	float TodayEarned = 0.f;
	int32 Day = 1;
	int32 GuestCount = 0;
	int32 MissionIndex = 0;
	int32 GuestsServed = 0;
	EResortEvent CurrentEvent = EResortEvent::None;
	float MissionTimer = 0.f;
	int32 Waiters = 0;
	int32 Cleaners = 0;
	int32 LastTier = 1;
	int32 LastLevel = 1;

	float DayClock = 0.f;
	/** Duracao de um dia de jogo em segundos reais. */
	float DayLengthSeconds = 240.f;
	/** O resort funciona das 8h as 20h. */
	float StartHour = 8.f;
	float EndHour = 20.f;
};
