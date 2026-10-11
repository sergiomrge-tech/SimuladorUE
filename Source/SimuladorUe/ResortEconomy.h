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

	// ---- Nivel do resort (1 = Quiosque ... 5 = Resort) ----
	int32 GetTier() const;
	static int32 GetMaxTier() { return 5; }
	FString GetTierName(int32 Tier) const;
	/** Prestigio necessario para o nivel atual. */
	float GetTierStartPrestige() const;
	/** Prestigio necessario para o proximo nivel, ou -1 se ja esta no maximo. */
	float GetNextTierPrestige() const;

	// ---- Relogio ----
	int32 GetDay() const { return Day; }
	float GetDayFraction() const { return DayLengthSeconds > 0.f ? DayClock / DayLengthSeconds : 0.f; }
	float GetHourOfDay() const { return StartHour + GetDayFraction() * (EndHour - StartHour); }

	// ---- Dados ----
	const TArray<FBuildingDef>& GetBuildingDefs() const { return BuildingDefs; }
	const TArray<FGuestDef>& GetGuestDefs() const { return GuestDefs; }
	const FBuildingDef* FindBuildingDef(const FString& Id) const;
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

	TArray<FBuildingDef> BuildingDefs;
	TArray<FGuestDef> GuestDefs;
	TArray<TWeakObjectPtr<ABuildingActor>> Buildings;
	TArray<FResortNotification> Notifications;

	float Money = 1000.f;
	float Prestige = 0.f;
	float TodayEarned = 0.f;
	int32 Day = 1;
	int32 GuestCount = 0;
	int32 LastTier = 1;

	float DayClock = 0.f;
	/** Duracao de um dia de jogo em segundos reais. */
	float DayLengthSeconds = 240.f;
	/** O resort funciona das 8h as 20h. */
	float StartHour = 8.f;
	float EndHour = 20.f;
};
