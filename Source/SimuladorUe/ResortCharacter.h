#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ResortTypes.h"
#include "ResortCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class ABuildingActor;
struct FInputActionValue;

/**
 * Jogador em primeira pessoa. As acoes de entrada (Enhanced Input) sao criadas
 * por codigo, entao o projeto nao precisa de nenhum asset de input.
 *
 * Controles: WASD mover, mouse olhar, Shift correr, Espaco pular,
 * B modo construcao, Q/E ou roda do mouse troca item, R gira,
 * clique esquerdo constroi, X demole (devolve 50%).
 */
UCLASS()
class SIMULADORUE_API AResortCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AResortCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PawnClientRestart() override;

	// Usado pelo HUD
	bool IsBuildMode() const { return bBuildMode; }
	const FBuildingDef* GetSelectedDef() const;
	int32 GetSelectedIndex() const { return SelectedIndex; }
	bool IsPlacementValid() const { return bPlacementValid; }
	const FString& GetStatusText() const { return StatusText; }

private:
	void EnsureInput();

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void DoJump();
	void DoStopJump();
	void SprintOn();
	void SprintOff();

	void ToggleBuildMode();
	void NextItem();
	void PrevItem();
	void RotateGhost();
	void PlaceBuilding();
	void DemolishBuilding();

	void CycleSelection(int32 Direction);
	void UpdateGhost();
	void DestroyGhost();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY()
	TObjectPtr<UInputMappingContext> InputContext;
	UPROPERTY()
	TObjectPtr<UInputAction> MoveAction;
	UPROPERTY()
	TObjectPtr<UInputAction> LookAction;
	UPROPERTY()
	TObjectPtr<UInputAction> JumpAction;
	UPROPERTY()
	TObjectPtr<UInputAction> SprintAction;
	UPROPERTY()
	TObjectPtr<UInputAction> BuildAction;
	UPROPERTY()
	TObjectPtr<UInputAction> NextAction;
	UPROPERTY()
	TObjectPtr<UInputAction> PrevAction;
	UPROPERTY()
	TObjectPtr<UInputAction> RotateAction;
	UPROPERTY()
	TObjectPtr<UInputAction> PlaceAction;
	UPROPERTY()
	TObjectPtr<UInputAction> DemolishAction;

	UPROPERTY()
	TObjectPtr<ABuildingActor> Ghost;

	FString GhostDefId;
	FString StatusText;
	bool bBuildMode = false;
	bool bPlacementValid = false;
	int32 SelectedIndex = 0;
	float GhostYaw = 0.f;

	static constexpr float WalkSpeed = 450.f;
	static constexpr float SprintSpeed = 750.f;
	static constexpr float BuildRange = 1800.f;
	static constexpr float GridSize = 50.f;
};
