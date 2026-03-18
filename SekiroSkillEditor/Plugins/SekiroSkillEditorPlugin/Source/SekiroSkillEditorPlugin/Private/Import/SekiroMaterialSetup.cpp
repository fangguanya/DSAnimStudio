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

	static void AutoBindTexturesToMaterial(
		UMaterialInstanceConstant* MatInstance,
		const FString& SlotName,
		const FString& TexturesPackagePath)
	{
		if (!MatInstance) return;

		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

		TArray<FAssetData> AllTextures;
		AssetRegistry.GetAssetsByPath(FName(*TexturesPackagePath), AllTextures, false);

		for (const FAssetData& TexData : AllTextures)
		{
			UTexture2D* Tex = Cast<UTexture2D>(TexData.GetAsset());
			if (!Tex) continue;

			const FString TexName = TexData.AssetName.ToString();
			const FString TexNameLower = TexName.ToLower();
			const FString SlotLower = SlotName.ToLower();

			bool bMatches = TexNameLower.Contains(SlotLower);

			if (!bMatches) continue;

			if (TexNameLower.EndsWith(TEXT("_a")) || TexNameLower.EndsWith(TEXT("_ao_a")))
			{
				MatInstance->SetTextureParameterValueEditorOnly(FName("BaseColor"), Tex);
			}
			else if (TexNameLower.EndsWith(TEXT("_n")))
			{
				MatInstance->SetTextureParameterValueEditorOnly(FName("Normal"), Tex);
			}
			else if (TexNameLower.EndsWith(TEXT("_m")) || TexNameLower.EndsWith(TEXT("_1m")))
			{
				MatInstance->SetTextureParameterValueEditorOnly(FName("Metallic"), Tex);
			}
			else if (TexNameLower.EndsWith(TEXT("_em")))
			{
				MatInstance->SetTextureParameterValueEditorOnly(FName("Emissive"), Tex);
			}
			else if (TexNameLower.EndsWith(TEXT("_d")))
			{
				MatInstance->SetTextureParameterValueEditorOnly(FName("Roughness"), Tex);
			}
			else if (TexNameLower.EndsWith(TEXT("_3m")))
			{
				MatInstance->SetTextureParameterValueEditorOnly(FName("Specular"), Tex);
			}
		}
	}
}

TArray<UMaterialInstanceConstant*> USekiroMaterialSetup::SetupMaterialsFromManifest(
	const FString& ManifestJsonPath,
	const FString& TextureDirectory,
	const FString& OutputPackagePath)
{
	TArray<UMaterialInstanceConstant*> CreatedInstances;

	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *ManifestJsonPath))
	{
		UE_LOG(LogTemp, Error, TEXT("SekiroMaterialSetup: Failed to read manifest: %s"), *ManifestJsonPath);
		return CreatedInstances;
	}

	TSharedPtr<FJsonObject> RootObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("SekiroMaterialSetup: Failed to parse manifest JSON: %s"), *ManifestJsonPath);
		return CreatedInstances;
	}

	UMaterial* ParentMaterial = Cast<UMaterial>(
		StaticLoadObject(UMaterial::StaticClass(), nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial")));
	if (!ParentMaterial)
	{
		UE_LOG(LogTemp, Error, TEXT("SekiroMaterialSetup: Failed to load parent material."));
		return CreatedInstances;
	}

	const TArray<TSharedPtr<FJsonValue>>* MaterialsArray = nullptr;
	if (!RootObject->TryGetArrayField(TEXT("materials"), MaterialsArray))
	{
		UE_LOG(LogTemp, Error, TEXT("SekiroMaterialSetup: Manifest missing 'materials' array."));
		return CreatedInstances;
	}

	const FString TexturesPackagePath = OutputPackagePath / TEXT("Textures");
	TSet<FString> CreatedAssetNames;

	for (const TSharedPtr<FJsonValue>& MaterialValue : *MaterialsArray)
	{
		const TSharedPtr<FJsonObject>& MaterialObj = MaterialValue->AsObject();
		if (!MaterialObj.IsValid()) continue;

		FString SlotName;
		MaterialObj->TryGetStringField(TEXT("slotName"), SlotName);
		if (SlotName.IsEmpty()) continue;

		const FString AssetName = SekiroMaterialSetupInternal::GetMaterialInstanceAssetName(MaterialObj);

		if (CreatedAssetNames.Contains(AssetName)) continue;
		CreatedAssetNames.Add(AssetName);

		const FString PackagePath = FString::Printf(TEXT("%s/Materials/%s"), *OutputPackagePath, *AssetName);

		UPackage* Package = CreatePackage(*PackagePath);
		if (!Package) continue;
		Package->FullyLoad();

		UMaterialInstanceConstant* MatInstance = NewObject<UMaterialInstanceConstant>(
			Package, *AssetName, RF_Public | RF_Standalone);
		if (!MatInstance) continue;

		MatInstance->SetParentEditorOnly(ParentMaterial);

		const TSharedPtr<FJsonObject>* TextureBindingsObj = nullptr;
		if (MaterialObj->TryGetObjectField(TEXT("textureBindings"), TextureBindingsObj))
		{
			for (const auto& BindingPair : (*TextureBindingsObj)->Values)
			{
				const FString& ParameterName = BindingPair.Key;
				const TSharedPtr<FJsonObject> BindingObj = BindingPair.Value->AsObject();
				if (!BindingObj.IsValid()) continue;

				FString TextureFilename;
				if (!BindingObj->TryGetStringField(TEXT("exportedFileName"), TextureFilename) || TextureFilename.IsEmpty()) continue;

				const FString TexBaseName = FPaths::GetBaseFilename(TextureFilename);
				if (UTexture2D* Tex = SekiroMaterialSetupInternal::FindTextureAsset(TexBaseName, TexturesPackagePath))
				{
					MatInstance->SetTextureParameterValueEditorOnly(FName(*ParameterName), Tex);
				}
			}
		}

		SekiroMaterialSetupInternal::AutoBindTexturesToMaterial(MatInstance, SlotName, TexturesPackagePath);

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

		MatInstance->PostEditChange();
		MatInstance->MarkPackageDirty();
		FAssetRegistryModule::AssetCreated(MatInstance);

		const FString PackageFilename = FPackageName::LongPackageNameToFilename(
			PackagePath, FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		UPackage::SavePackage(Package, MatInstance, *PackageFilename, SaveArgs);

		CreatedInstances.Add(MatInstance);
	}

	UE_LOG(LogTemp, Display, TEXT("SekiroMaterialSetup: Created %d material instances."), CreatedInstances.Num());
	return CreatedInstances;
}

void USekiroMaterialSetup::BindMaterialsToSkeletalMesh(
	USkeletalMesh* SkelMesh,
	const FString& ManifestJsonPath,
	const FString& ChrContent)
{
	if (!SkelMesh) return;

	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *ManifestJsonPath)) return;

	TSharedPtr<FJsonObject> RootObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid()) return;

	const TArray<TSharedPtr<FJsonValue>>* MaterialsArray = nullptr;
	if (!RootObject->TryGetArrayField(TEXT("materials"), MaterialsArray)) return;

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
	}

	if (SlotToMaterial.Num() == 0) return;

	TArray<FSkeletalMaterial>& Materials = SkelMesh->GetMaterials();
	bool bChanged = false;

	for (FSkeletalMaterial& SkelMat : Materials)
	{
		const FString MatSlotName = SkelMat.MaterialSlotName.ToString();
		if (UMaterialInterface** Found = SlotToMaterial.Find(MatSlotName))
		{
			SkelMat.MaterialInterface = *Found;
			bChanged = true;
			UE_LOG(LogTemp, Display, TEXT("  BindMat: slot '%s' -> %s"), *MatSlotName, *(*Found)->GetName());
		}
	}

	if (bChanged)
	{
		SkelMesh->MarkPackageDirty();
	}
}
