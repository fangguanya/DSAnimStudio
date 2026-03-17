// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Dom/JsonObject.h"
#include "Data/SekiroTaeEvent.h"
#include "Data/SekiroSkillDataAsset.h"
#include "SekiroAssetImporter.generated.h"

/**
 * Handles importing Sekiro skill configuration data from JSON files
 * exported by DSAnimStudio. Parses character animation data and TAE events
 * into USekiroSkillDataAsset instances.
 */
UCLASS()
class SEKIROSKILLEDITORPLUGIN_API USekiroAssetImporter : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Imports a skill configuration JSON file and creates data assets for each animation entry.
	 *
	 * @param JsonFilePath        Absolute path to the JSON configuration file.
	 * @param OutputPackagePath   Content browser path where assets will be created (e.g. "/Game/Sekiro/Skills").
	 * @return Array of newly created skill data assets. Empty if the import fails.
	 */
	UFUNCTION(BlueprintCallable, Category = "Sekiro|Import")
	static TArray<USekiroSkillDataAsset*> ImportSkillConfig(const FString& JsonFilePath, const FString& OutputPackagePath);

	/**
	 * Parses TAE events from a JSON object representing a single animation.
	 *
	 * @param AnimJson  JSON object containing an "events" array with TAE event data.
	 * @return Array of parsed TAE event structs.
	 */
	static TArray<FSekiroTaeEvent> ParseTaeEventsFromJson(const TSharedPtr<FJsonObject>& AnimJson);
};
