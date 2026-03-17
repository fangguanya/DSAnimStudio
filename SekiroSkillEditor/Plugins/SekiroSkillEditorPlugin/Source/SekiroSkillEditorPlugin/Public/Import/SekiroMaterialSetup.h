// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Materials/MaterialInstanceConstant.h"
#include "SekiroMaterialSetup.generated.h"

/**
 * Utility class for creating UMaterialInstanceConstant assets from a
 * material manifest JSON exported by the Sekiro asset pipeline.
 * Maps texture slots (BaseColor, Normal, Specular, Emissive) to the
 * corresponding material parameter names.
 */
UCLASS()
class SEKIROSKILLEDITORPLUGIN_API USekiroMaterialSetup : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Reads a material manifest JSON and creates material instances for each entry.
	 *
	 * @param ManifestJsonPath   Absolute path to material_manifest.json.
	 * @param TextureDirectory   Absolute path to the directory containing referenced textures.
	 * @param OutputPackagePath  Content browser path where material instances will be saved.
	 * @return Array of newly created material instance constants.
	 */
	UFUNCTION(BlueprintCallable, Category = "Sekiro|Import")
	static TArray<UMaterialInstanceConstant*> SetupMaterialsFromManifest(
		const FString& ManifestJsonPath,
		const FString& TextureDirectory,
		const FString& OutputPackagePath
	);
};
