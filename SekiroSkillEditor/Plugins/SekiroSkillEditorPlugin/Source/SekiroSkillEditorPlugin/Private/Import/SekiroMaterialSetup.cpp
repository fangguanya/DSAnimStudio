#include "Import/SekiroMaterialSetup.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Texture2D.h"
#include "Engine/SkeletalMesh.h"
#include "EditorAssetLibrary.h"

namespace SekiroMaterialSetupInternal
{
	static UTexture2D* FindTextureAsset(const FString& BaseName, const FString& TexturesPackagePath)
	{
		const FString AssetPath = TexturesPackagePath / BaseName;
		UObject* Asset = StaticLoadObject(UTexture2D::StaticClass(), nullptr, *AssetPath);
		return Cast<UTexture2D>(Asset);
	}

	static FString GetMaterialInstanceAssetName(const TSharedPtr<FJsonObject>& MaterialObj)
	{
		FString MaterialKey;
		if (!MaterialObj->TryGetStringField(TEXT("materialInstanceKey"), MaterialKey) || MaterialKey.IsEmpty())
		{
			MaterialObj->TryGetStringField(TEXT("slotName"), MaterialKey);
		}
		if (MaterialKey.IsEmpty())
		{
			MaterialObj->TryGetStringField(TEXT("name"), MaterialKey);
		}
		MaterialKey.ReplaceInline(TEXT(" "), TEXT("_"));
		MaterialKey.ReplaceInline(TEXT("|"), TEXT("_"));
		MaterialKey.ReplaceInline(TEXT("#"), TEXT("_"));

		if (!MaterialKey.StartsWith(TEXT("MI_")))
		{
			return FString::Printf(TEXT("MI_%s"), *MaterialKey);
		}
		return MaterialKey;
	}

	static FString GetFormalMasterMaterialPath(const TSharedPtr<FJsonObject>& RootObject)
	{
		FString ManifestPath;
		if (RootObject.IsValid() && RootObject->TryGetStringField(TEXT("masterMaterialPath"), ManifestPath) && !ManifestPath.IsEmpty())
		{
			return ManifestPath;
		}

		return TEXT("/Game/SekiroAssets/Materials/M_SekiroMaster.M_SekiroMaster");
	}
}

TArray<UMaterialInstanceConstant*> USekiroMaterialSetup::SetupMaterialsFromManifest(
	const FString& ManifestJsonPath,
	const FString& TextureDirectory,
	const FString& OutputPackagePath)
{
	TArray<UMaterialInstanceConstant*> CreatedInstances;
	TArray<FString> Errors;
	SetupMaterialsFromManifestStrict(ManifestJsonPath, TextureDirectory, OutputPackagePath, CreatedInstances, Errors);
	for (const FString& Error : Errors)
	{
		UE_LOG(LogTemp, Error, TEXT("SekiroMaterialSetup: %s"), *Error);
	}
	return CreatedInstances;
}

bool USekiroMaterialSetup::SetupMaterialsFromManifestStrict(
	const FString& ManifestJsonPath,
	const FString& TextureDirectory,
	const FString& OutputPackagePath,
	TArray<UMaterialInstanceConstant*>& OutCreatedInstances,
	TArray<FString>& OutErrors)
{
	OutCreatedInstances.Reset();
	OutErrors.Reset();

	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *ManifestJsonPath))
	{
		OutErrors.Add(FString::Printf(TEXT("Failed to read manifest: %s"), *ManifestJsonPath));
		return false;
	}

	TSharedPtr<FJsonObject> RootObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
	{
		OutErrors.Add(FString::Printf(TEXT("Failed to parse manifest JSON: %s"), *ManifestJsonPath));
		return false;
	}

	const FString ParentMaterialPath = SekiroMaterialSetupInternal::GetFormalMasterMaterialPath(RootObject);
	UMaterialInterface* ParentMaterial = Cast<UMaterialInterface>(
		StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, *ParentMaterialPath));
	if (!ParentMaterial)
	{
		OutErrors.Add(FString::Printf(TEXT("Formal master material is missing: %s"), *ParentMaterialPath));
		return false;
	}

	const TArray<TSharedPtr<FJsonValue>>* MaterialsArray = nullptr;
	if (!RootObject->TryGetArrayField(TEXT("materials"), MaterialsArray))
	{
		OutErrors.Add(TEXT("Manifest missing 'materials' array."));
		return false;
	}

	const FString TexturesPackagePath = OutputPackagePath / TEXT("Textures");
	TSet<FString> CreatedAssetNames;

	for (const TSharedPtr<FJsonValue>& MaterialValue : *MaterialsArray)
	{
		const TSharedPtr<FJsonObject>& MaterialObj = MaterialValue->AsObject();
		if (!MaterialObj.IsValid())
		{
			OutErrors.Add(TEXT("Encountered invalid material entry in manifest."));
			continue;
		}

		FString SlotName;
		MaterialObj->TryGetStringField(TEXT("slotName"), SlotName);
		if (SlotName.IsEmpty())
		{
			OutErrors.Add(TEXT("Manifest material entry is missing slotName."));
			continue;
		}

		const FString AssetName = SekiroMaterialSetupInternal::GetMaterialInstanceAssetName(MaterialObj);

		if (CreatedAssetNames.Contains(AssetName)) continue;
		CreatedAssetNames.Add(AssetName);

		const FString PackagePath = FString::Printf(TEXT("%s/Materials/%s"), *OutputPackagePath, *AssetName);

		UPackage* Package = CreatePackage(*PackagePath);
		if (!Package) continue;
		Package->FullyLoad();

		UMaterialInstanceConstant* MatInstance = FindObject<UMaterialInstanceConstant>(Package, *AssetName);
		if (!MatInstance)
		{
			MatInstance = NewObject<UMaterialInstanceConstant>(Package, *AssetName, RF_Public | RF_Standalone);
		}
		if (!MatInstance) continue;

		MatInstance->SetParentEditorOnly(ParentMaterial);
		const int32 ErrorCountBeforeMaterial = OutErrors.Num();

		const TSharedPtr<FJsonObject>* TextureBindingsObj = nullptr;
		if (MaterialObj->TryGetObjectField(TEXT("textureBindings"), TextureBindingsObj) && TextureBindingsObj && TextureBindingsObj->IsValid())
		{
			for (const auto& BindingPair : (*TextureBindingsObj)->Values)
			{
				const FString& ParameterName = BindingPair.Key;
				const TSharedPtr<FJsonObject> BindingObj = BindingPair.Value->AsObject();
				if (!BindingObj.IsValid())
				{
					OutErrors.Add(FString::Printf(TEXT("Material '%s' has invalid texture binding '%s'."), *AssetName, *ParameterName));
					continue;
				}

				FString TextureFilename;
				if (!BindingObj->TryGetStringField(TEXT("exportedFileName"), TextureFilename) || TextureFilename.IsEmpty())
				{
					OutErrors.Add(FString::Printf(TEXT("Material '%s' binding '%s' is missing exportedFileName."), *AssetName, *ParameterName));
					continue;
				}

				const FString TexBaseName = FPaths::GetBaseFilename(TextureFilename);
				if (UTexture2D* Tex = SekiroMaterialSetupInternal::FindTextureAsset(TexBaseName, TexturesPackagePath))
				{
					MatInstance->SetTextureParameterValueEditorOnly(FName(*ParameterName), Tex);
				}
				else
				{
					OutErrors.Add(FString::Printf(TEXT("Material '%s' binding '%s' references missing texture asset '%s'."), *AssetName, *ParameterName, *TexBaseName));
				}
			}
		}
		else
		{
			const TArray<TSharedPtr<FJsonValue>>* TexturesArray = nullptr;
			if (MaterialObj->TryGetArrayField(TEXT("textures"), TexturesArray) && TexturesArray && TexturesArray->Num() > 0)
			{
				OutErrors.Add(FString::Printf(TEXT("Material '%s' is missing formal textureBindings."), *AssetName));
			}
		}

		const TSharedPtr<FJsonObject>* ScalarsObj = nullptr;
		if (MaterialObj->TryGetObjectField(TEXT("scalarParameters"), ScalarsObj))
		{
			for (const auto& Param : (*ScalarsObj)->Values)
			{
				double Value = 0.0;
				if (Param.Value->TryGetNumber(Value))
				{
					MatInstance->SetScalarParameterValueEditorOnly(FName(*Param.Key), static_cast<float>(Value));
				}
			}
		}

		if (OutErrors.Num() > ErrorCountBeforeMaterial)
		{
			continue;
		}

		MatInstance->PostEditChange();
		MatInstance->MarkPackageDirty();
		FAssetRegistryModule::AssetCreated(MatInstance);

		const FString PackageFilename = FPackageName::LongPackageNameToFilename(
			PackagePath, FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		UPackage::SavePackage(Package, MatInstance, *PackageFilename, SaveArgs);

		OutCreatedInstances.Add(MatInstance);
	}

	UE_LOG(LogTemp, Display, TEXT("SekiroMaterialSetup: Created %d material instances."), OutCreatedInstances.Num());
	return OutErrors.Num() == 0;
}

void USekiroMaterialSetup::BindMaterialsToSkeletalMesh(
	USkeletalMesh* SkelMesh,
	const FString& ManifestJsonPath,
	const FString& ChrContent)
{
	TArray<FString> Errors;
	BindMaterialsToSkeletalMeshStrict(SkelMesh, ManifestJsonPath, ChrContent, Errors);
	for (const FString& Error : Errors)
	{
		UE_LOG(LogTemp, Error, TEXT("SekiroMaterialSetup: %s"), *Error);
	}
}

bool USekiroMaterialSetup::BindMaterialsToSkeletalMeshStrict(
	USkeletalMesh* SkelMesh,
	const FString& ManifestJsonPath,
	const FString& ChrContent,
	TArray<FString>& OutErrors)
{
	OutErrors.Reset();
	if (!SkelMesh)
	{
		OutErrors.Add(TEXT("Cannot bind materials to a null skeletal mesh."));
		return false;
	}

	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *ManifestJsonPath))
	{
		OutErrors.Add(FString::Printf(TEXT("Failed to read manifest for skeletal mesh binding: %s"), *ManifestJsonPath));
		return false;
	}

	TSharedPtr<FJsonObject> RootObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
	{
		OutErrors.Add(FString::Printf(TEXT("Failed to parse manifest JSON for skeletal mesh binding: %s"), *ManifestJsonPath));
		return false;
	}

	const TArray<TSharedPtr<FJsonValue>>* MaterialsArray = nullptr;
	if (!RootObject->TryGetArrayField(TEXT("materials"), MaterialsArray) || !MaterialsArray)
	{
		OutErrors.Add(TEXT("Manifest missing 'materials' array for skeletal mesh binding."));
		return false;
	}

	TMap<FString, UMaterialInterface*> SlotToMaterial;

	for (const TSharedPtr<FJsonValue>& MaterialValue : *MaterialsArray)
	{
		const TSharedPtr<FJsonObject>& MaterialObj = MaterialValue->AsObject();
		if (!MaterialObj.IsValid()) continue;

		FString SlotName;
		MaterialObj->TryGetStringField(TEXT("slotName"), SlotName);
		if (SlotName.IsEmpty() || SlotToMaterial.Contains(SlotName)) continue;

		const FString AssetName = SekiroMaterialSetupInternal::GetMaterialInstanceAssetName(MaterialObj);
		const FString AssetPath = FString::Printf(TEXT("%s/Materials/%s.%s"), *ChrContent, *AssetName, *AssetName);

		if (UMaterialInterface* Mat = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, *AssetPath)))
		{
			SlotToMaterial.Add(SlotName, Mat);
		}
		else
		{
			OutErrors.Add(FString::Printf(TEXT("Manifest slot '%s' expected material instance '%s' but it was not created."), *SlotName, *AssetName));
		}
	}

	if (SlotToMaterial.Num() == 0)
	{
		OutErrors.Add(TEXT("No formal material instances were available for skeletal mesh binding."));
		return false;
	}

	TArray<FSkeletalMaterial>& Materials = SkelMesh->GetMaterials();
	bool bChanged = false;
	TSet<FString> BoundSlots;

	for (FSkeletalMaterial& SkelMat : Materials)
	{
		const FString MatSlotName = SkelMat.MaterialSlotName.ToString();
		if (UMaterialInterface** Found = SlotToMaterial.Find(MatSlotName))
		{
			SkelMat.MaterialInterface = *Found;
			bChanged = true;
			BoundSlots.Add(MatSlotName);
			UE_LOG(LogTemp, Display, TEXT("  BindMat: slot '%s' -> %s"), *MatSlotName, *(*Found)->GetName());
		}
		else
		{
			OutErrors.Add(FString::Printf(TEXT("Skeletal mesh slot '%s' has no formal manifest binding."), *MatSlotName));
		}
	}

	for (const TPair<FString, UMaterialInterface*>& Pair : SlotToMaterial)
	{
		if (!BoundSlots.Contains(Pair.Key))
		{
			OutErrors.Add(FString::Printf(TEXT("Manifest slot '%s' was not found on the skeletal mesh."), *Pair.Key));
		}
	}

	if (bChanged)
	{
		SkelMesh->MarkPackageDirty();
	}

	return OutErrors.Num() == 0;
}
