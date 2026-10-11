#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ResortTypes.h"
#include "StaffCharacter.generated.h"

class UStaticMeshComponent;

/** Funcao do funcionario que anda pela praia. */
enum class EStaffRole : uint8
{
	Waiter,
	Cleaner
};

/**
 * Funcionario que circula pela praia: o garcom passa pelas lojas de comida e o
 * faxineiro anda pela areia. Usa o modelo 3D de Content/Data/Staff.json se existir.
 */
UCLASS()
class SIMULADORUE_API AStaffCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AStaffCharacter();

	void InitStaff(EStaffRole InRole);
	EStaffRole GetRole() const { return Role; }

	virtual void Tick(float DeltaSeconds) override;

private:
	void PickDestination();

	EStaffRole Role = EStaffRole::Waiter;
	FVector Destination = FVector::ZeroVector;
	bool bHasDestination = false;
	float PauseTimer = 0.f;
	bool bInitialized = false;
};
