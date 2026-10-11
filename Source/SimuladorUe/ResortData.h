#pragma once

#include "CoreMinimal.h"
#include "ResortTypes.h"

/** Leitura dos arquivos de dados em Content/Data. Se algo falhar, usa valores padrao. */
namespace ResortData
{
	void LoadBuildings(TArray<FBuildingDef>& Out);
	void LoadGuests(TArray<FGuestDef>& Out);
}
