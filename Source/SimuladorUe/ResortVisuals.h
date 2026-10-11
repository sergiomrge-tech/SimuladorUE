#pragma once

#include "CoreMinimal.h"

class AActor;
class USceneComponent;
class UStaticMeshComponent;

/**
 * Ajudante para montar visuais provisorios com as formas basicas da Unreal
 * (/Engine/BasicShapes). Serve para o prototipo de jogabilidade; a arte final
 * (Fab/Quixel/Nanite) substitui isso depois.
 */
class ACharacter;
struct FCharacterModel;

namespace ResortVisuals
{
	/**
	 * Aplica um modelo 3D (SkeletalMesh + Animation Blueprint opcional) ao mesh do personagem.
	 * Retorna false se nao houver modelo ou ele nao for encontrado (use o boneco provisorio).
	 */
	bool ApplyCharacterModel(ACharacter* Character, const FCharacterModel& Model);

	/** Cria um modelo parado (sem personagem), usado nos atendentes das lojas. */
	bool AddStaticCharacter(AActor* Owner, USceneComponent* Parent, const FCharacterModel& Model,
		const FVector& LocalLocation, float LocalYaw);

	/**
	 * Cria e registra um componente de malha ligado a Parent.
	 * @param Shape     Cube, Sphere, Cylinder ou Cone
	 * @param SizeCm    tamanho final em cm (as formas basicas tem 100 cm)
	 * @param bBlocking se true, bloqueia o jogador e entra no canal ECC_Building
	 */
	UStaticMeshComponent* AddPart(
		AActor* Owner,
		USceneComponent* Parent,
		const TCHAR* Shape,
		const FVector& LocalLocation,
		const FVector& SizeCm,
		const FRotator& LocalRotation,
		const FLinearColor& Color,
		bool bBlocking);
}
