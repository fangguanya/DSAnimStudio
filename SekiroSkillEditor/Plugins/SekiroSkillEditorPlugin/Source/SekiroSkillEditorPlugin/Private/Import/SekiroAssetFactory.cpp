// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

#include "Import/SekiroAssetFactory.h"

#include "Import/SekiroAssetImporter.h"
#include "Data/SekiroSkillDataAsset.h"

USekiroSkillConfigFactory::USekiroSkillConfigFactory()
{
	bCreateNew = false;
	bEditorImport = true;
	bEditAfterNew = true;
	bText = false;

	SupportedClass = USekiroSkillDataAsset::StaticClass();

	Formats.Add(TEXT("json;Sekiro Skill Config JSON"));
}

UObject* USekiroSkillConfigFactory::FactoryCreateFile(
	UClass* InClass,
	UObject* InParent,
	FName InName,
	EObjectFlags Flags,
	const FString& Filename,
	const TCHAR* Parms,
	FFeedbackContext* Warn,
	bool& bOutOperationCanceled)
{
	bOutOperationCanceled = false;

	// Derive the output package path from the parent object.
	FString OutputPackagePath;
	if (InParent)
	{
		OutputPackagePath = InParent->GetPathName();
	}
	else
	{
		OutputPackagePath = TEXT("/Game/Sekiro/Skills");
	}

	TArray<USekiroSkillDataAsset*> ImportedAssets =
		USekiroAssetImporter::ImportSkillConfig(Filename, OutputPackagePath);

	if (ImportedAssets.Num() == 0)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("SekiroSkillConfigFactory: No assets were created from '%s'."), *Filename);
		return nullptr;
	}

	// Return the first asset as the primary result; the rest are created as side-effects
	// and are already registered in the asset registry.
	return ImportedAssets[0];
}
