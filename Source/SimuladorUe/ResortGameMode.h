#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ResortGameMode.generated.h"

class AStaticMeshActor;
class APlayerStart;
class UDirectionalLightComponent;
class UResortEconomy;
class AStaffCharacter;
class USoundWaveProcedural;
class UAudioComponent;

/**
 * Regras da partida. Como o prototipo nao usa assets de mapa, este GameMode
 * monta tudo por codigo em um nivel vazio: praia, mar, luz, ceu, quiosque
 * inicial, palmeiras e o fluxo de hospedes.
 */
UCLASS()
class SIMULADORUE_API AResortGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AResortGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void StartPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	/** Salva/carrega o resort em Saved/resort_save.json. */
	void SaveResort(bool bQuiet = false);
	bool LoadResort(bool bQuiet = false);
	/** Apaga o save e recomeca a praia do zero. */
	void NewResort();

	/** Meia-largura (cm) do terreno liberado em cada nivel do resort: o terreno cresce com o nivel. */
	static float PlotHalfYForTier(int32 Tier);
	static float GetPlotHalfY(const UObject* WorldContext);

	/** Acesso rapido a economia a partir de qualquer objeto do mundo. */
	static UResortEconomy* GetEconomy(const UObject* WorldContext);

private:
	void EnsureWorldBuilt();
	void BuildBeach();
	void BuildAtmosphere();
	void SpawnPalms();
	void SpawnStartingKiosk();
	void SpawnGuest();
	void UpdateSun();
	void BuildSandDetail();
	void SyncStaff();
	void UpdatePlot(bool bAnnounce);
	void UpdateWaves(float DeltaSeconds);

	AStaticMeshActor* SpawnBox(const FVector& Center, const FVector& SizeCm, const FLinearColor& Color,
		bool bVisible, bool bCollide);

	UPROPERTY()
	TObjectPtr<UResortEconomy> Economy;

	UPROPERTY()
	TObjectPtr<UDirectionalLightComponent> SunComponent;

	UPROPERTY()
	TObjectPtr<APlayerStart> PlayerStartActor;

	/** Peca do mar que se mexe com as ondas. */
	struct FWavePart
	{
		TWeakObjectPtr<AStaticMeshActor> Actor;
		FVector Base = FVector::ZeroVector;
		FVector BaseSize = FVector(100.f);
		float Phase = 0.f;
		int32 Kind = 0; // 0 = bobbing, 1 = espuma que vai e volta, 2 = crista que viaja, 3 = areia molhada que avanca
	};
	TArray<FWavePart> WaveParts;
	float WaveTime = 0.f;

	TArray<TWeakObjectPtr<AStaffCharacter>> WaiterActors;
	TArray<TWeakObjectPtr<AStaffCharacter>> CleanerActors;
	float StaffSyncTimer = 0.f;

	TWeakObjectPtr<AStaticMeshActor> WallLeft;
	TWeakObjectPtr<AStaticMeshActor> WallRight;
	TWeakObjectPtr<AStaticMeshActor> FenceLeft;
	TWeakObjectPtr<AStaticMeshActor> FenceRight;
	int32 PlotTier = 0;

	UPROPERTY()
	TObjectPtr<USoundWaveProcedural> SeaWave;
	UPROPERTY()
	TObjectPtr<UAudioComponent> SeaAudio;
	float AudioTimer = 0.f;
	float GullTimer = 8.f;

	bool bWorldBuilt = false;
	int32 LastAutosaveDay = 1;
	float GuestSpawnTimer = 0.f;
	float SunUpdateTimer = 0.f;
};
