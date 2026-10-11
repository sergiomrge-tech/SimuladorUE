#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ResortGameMode.generated.h"

class AStaticMeshActor;
class APlayerStart;
class UDirectionalLightComponent;
class UResortEconomy;

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

	AStaticMeshActor* SpawnBox(const FVector& Center, const FVector& SizeCm, const FLinearColor& Color,
		bool bVisible, bool bCollide);

	UPROPERTY()
	TObjectPtr<UResortEconomy> Economy;

	UPROPERTY()
	TObjectPtr<UDirectionalLightComponent> SunComponent;

	UPROPERTY()
	TObjectPtr<APlayerStart> PlayerStartActor;

	bool bWorldBuilt = false;
	int32 LastAutosaveDay = 1;
	float GuestSpawnTimer = 0.f;
	float SunUpdateTimer = 0.f;
};
