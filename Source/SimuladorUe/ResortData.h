#pragma once

#include "CoreMinimal.h"
#include "ResortTypes.h"
#include "Containers/Map.h"

/** Leitura dos arquivos de dados em Content/Data. Se algo falhar, usa valores padrao. */
namespace ResortData
{
	void LoadBuildings(TArray<FBuildingDef>& Out);
	void LoadGuests(TArray<FGuestDef>& Out);
	void LoadUpgrades(TArray<FUpgradeDef>& Out);
	/** Modelos dos funcionarios por funcao: attendant, waiter, cleaner (Content/Data/Staff.json). */
	void LoadStaffModels(TMap<FString, FCharacterModel>& Out);
}
