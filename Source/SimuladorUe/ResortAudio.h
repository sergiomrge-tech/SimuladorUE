#pragma once

#include "CoreMinimal.h"

class UObject;
class USoundWaveProcedural;

enum class EResortSound : uint8
{
	Coin,
	Build,
	Upgrade,
	Gull,
	Error
};

/**
 * Sons gerados por codigo (sem arquivos de audio): mar, moedas, construcao, melhoria, gaivota.
 * Servem como som provisorio; depois podem ser trocados por audio de verdade.
 */
namespace ResortAudio
{
	/** Toca um efeito curto (2D). */
	void Play(const UObject* WorldContext, EResortSound Sound, float Volume = 0.5f);

	/** Cria o som continuo do mar (procedural) e ja enfileira audio. */
	USoundWaveProcedural* CreateSeaAmbience();
	/** Enfileira mais audio do mar quando o buffer esta acabando. Chame de tempos em tempos. */
	void KeepSeaAmbienceFed(USoundWaveProcedural* Wave);
}
