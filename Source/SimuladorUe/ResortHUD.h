#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ResortHUD.generated.h"

/** HUD desenhado direto no Canvas (sem assets de UMG): dinheiro, nivel, relogio e menu de construcao. */
UCLASS()
class SIMULADORUE_API AResortHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
};
