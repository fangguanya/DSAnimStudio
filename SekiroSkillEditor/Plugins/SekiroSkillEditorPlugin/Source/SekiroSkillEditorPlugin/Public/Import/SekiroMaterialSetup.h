// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Engine/SkeletalMesh.h"
#include "SekiroMaterialSetup.generated.h"

UCLASS()
class SEKIROSKILLEDITORPLUGIN_API USekiroMaterialSetup : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Sekiro|Import")
	static TArray<UMaterialInstanceConstant*> SetupMaterialsFromManifest(
		const FString& ManifestJsonPath,
		const FString& TextureDirectory,
		const FString& OutputPackagePath
	);

	static void BindMaterialsToSkeletalMesh(
		USkeletalMesh* SkelMesh,
		const FString& ManifestJsonPath,
		const FString& ChrContent
	);
};
