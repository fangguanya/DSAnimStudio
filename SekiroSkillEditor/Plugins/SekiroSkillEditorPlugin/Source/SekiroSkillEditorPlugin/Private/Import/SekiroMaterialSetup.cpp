// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

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
#include "EditorAssetLibrary.h"

namespace SekiroMaterialSetupInternal
{
	/** Maps a manifest texture slot name to the UMaterial parameter name. */
	static const TMap<FString, FName>& GetTextureSlotMapping()
	{
		static const TMap<FString, FName> Mapping = {
			{ TEXT("BaseColor"),  FName(TEXT("BaseColor"))  },
			{ TEXT("Normal"),     FName(TEXT("Normal"))     },
			{ TEXT("Specular"),   FName(TEXT("Specular"))   },
			{ TEXT("Emissive"),   FName(TEXT("Emissive"))   },
		};
		return Mapping;
	}

	/**
	 * Attempts to find or import a texture asset for the given filename
	 * relative to the texture directory.
	 */
	static UTexture2D* ResolveTexture(
		const FString& TextureFilename,
		const FString& TextureDirectory,
		const FString& OutputPackagePath)
	{
		if (TextureFilename.IsEmpty())
		{
			return nullptr;
		}

		// Try to find an already-imported texture asset by name.
		const FString TextureBaseName = FPaths::GetBaseFilename(TextureFilename);
		const FString TextureAssetPath = FString::Printf(
			TEXT("%s/Textures/%s"), *OutputPackagePath, *TextureBaseName);

		UObject* ExistingAsset = UEditorAssetLibrary::LoadAsset(TextureAssetPath);
		if (UTexture2D* ExistingTexture = Cast<UTexture2D>(ExistingAsset))
		{
			return ExistingTexture;
		}

		// If the texture is not yet in the project, check if the source file exists on disk.
		const FString SourcePath = FPaths::Combine(TextureDirectory, TextureFilename);
		if (!FPaths::FileExists(SourcePath))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("SekiroMaterialSetup: Texture file not found: %s"), *SourcePath);
			return nullptr;
		}

		// Import via UEditorAssetLibrary.
		const FString DestinationPath = FString::Printf(
			TEXT("%s/Textures/%s"), *OutputPackagePath, *TextureBaseName);
		if (UEditorAssetLibrary::DoesAssetExist(DestinationPath))
		{
			ExistingAsset = UEditorAssetLibrary::LoadAsset(DestinationPath);
			return Cast<UTexture2D>(ExistingAsset);
		}

		UE_LOG(LogTemp, Warning,
			TEXT("SekiroMaterialSetup: Texture '%s' exists on disk but is not imported. "
			     "Import it into '%s/Textures/' first."),
			*TextureFilename, *OutputPackagePath);
		return nullptr;
	}
}

TArray<UMaterialInstanceConstant*> USekiroMaterialSetup::SetupMaterialsFromManifest(
	const FString& ManifestJsonPath,
	const FString& TextureDirectory,
	const FString& OutputPackagePath)
{
	TArray<UMaterialInstanceConstant*> CreatedInstances;

	// Read the manifest JSON.
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *ManifestJsonPath))
	{
		UE_LOG(LogTemp, Error,
			TEXT("SekiroMaterialSetup: Failed to read manifest: %s"), *ManifestJsonPath);
		return CreatedInstances;
	}

	TSharedPtr<FJsonObject> RootObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
	{
		UE_LOG(LogTemp, Error,
			TEXT("SekiroMaterialSetup: Failed to parse manifest JSON: %s"), *ManifestJsonPath);
		return CreatedInstances;
	}

	// Resolve the parent material. Use the engine default lit material as the base.
	static const FString DefaultLitMaterialPath =
		TEXT("/Engine/EngineMaterials/DefaultLitMaterial.DefaultLitMaterial");
	UMaterial* ParentMaterial = Cast<UMaterial>(
		StaticLoadObject(UMaterial::StaticClass(), nullptr, *DefaultLitMaterialPath));

	if (!ParentMaterial)
	{
		UE_LOG(LogTemp, Error,
			TEXT("SekiroMaterialSetup: Failed to load parent material: %s"), *DefaultLitMaterialPath);
		return CreatedInstances;
	}

	// Iterate "materials" array.
	const TArray<TSharedPtr<FJsonValue>>* MaterialsArray = nullptr;
	if (!RootObject->TryGetArrayField(TEXT("materials"), MaterialsArray))
	{
		UE_LOG(LogTemp, Error,
			TEXT("SekiroMaterialSetup: Manifest missing 'materials' array."));
		return CreatedInstances;
	}

	const TMap<FString, FName>& SlotMapping = SekiroMaterialSetupInternal::GetTextureSlotMapping();

	for (const TSharedPtr<FJsonValue>& MaterialValue : *MaterialsArray)
	{
		const TSharedPtr<FJsonObject>& MaterialObj = MaterialValue->AsObject();
		if (!MaterialObj.IsValid())
		{
			continue;
		}

		const FString MaterialName = MaterialObj->GetStringField(TEXT("name"));
		if (MaterialName.IsEmpty())
		{
			UE_LOG(LogTemp, Warning,
				TEXT("SekiroMaterialSetup: Skipping material entry with no 'name' field."));
			continue;
		}

		const FString AssetName = FString::Printf(TEXT("MI_%s"), *MaterialName);
		const FString PackagePath = FString::Printf(TEXT("%s/Materials/%s"), *OutputPackagePath, *AssetName);

		// Create the package.
		UPackage* Package = CreatePackage(*PackagePath);
		if (!Package)
		{
			UE_LOG(LogTemp, Error,
				TEXT("SekiroMaterialSetup: Failed to create package: %s"), *PackagePath);
			continue;
		}
		Package->FullyLoad();

		// Create the material instance constant.
		UMaterialInstanceConstant* MatInstance = NewObject<UMaterialInstanceConstant>(
			Package,
			*AssetName,
			RF_Public | RF_Standalone);

		if (!MatInstance)
		{
			UE_LOG(LogTemp, Error,
				TEXT("SekiroMaterialSetup: Failed to create material instance: %s"), *AssetName);
			continue;
		}

		MatInstance->SetParentEditorOnly(ParentMaterial);

		// Read the "textures" sub-object and assign texture parameters.
		const TSharedPtr<FJsonObject>* TexturesObj = nullptr;
		if (MaterialObj->TryGetObjectField(TEXT("textures"), TexturesObj))
		{
			for (const auto& SlotPair : SlotMapping)
			{
				FString TextureFilename;
				if ((*TexturesObj)->TryGetStringField(SlotPair.Key, TextureFilename) && !TextureFilename.IsEmpty())
				{
					UTexture2D* Texture = SekiroMaterialSetupInternal::ResolveTexture(
						TextureFilename, TextureDirectory, OutputPackagePath);

					if (Texture)
					{
						MatInstance->SetTextureParameterValueEditorOnly(
							SlotPair.Value, Texture);
					}
				}
			}
		}

		// Read optional scalar parameters.
		const TSharedPtr<FJsonObject>* ScalarsObj = nullptr;
		if (MaterialObj->TryGetObjectField(TEXT("scalarParameters"), ScalarsObj))
		{
			for (const auto& Param : (*ScalarsObj)->Values)
			{
				double Value = 0.0;
				if (Param.Value->TryGetNumber(Value))
				{
					MatInstance->SetScalarParameterValueEditorOnly(
						FName(*Param.Key), static_cast<float>(Value));
				}
			}
		}

		// Finalize.
		MatInstance->PostEditChange();
		MatInstance->MarkPackageDirty();

		FAssetRegistryModule::AssetCreated(MatInstance);

		const FString PackageFilename = FPackageName::LongPackageNameToFilename(
			PackagePath, FPackageName::GetAssetPackageExtension());

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		UPackage::SavePackage(Package, MatInstance, *PackageFilename, SaveArgs);

		CreatedInstances.Add(MatInstance);

		UE_LOG(LogTemp, Log,
			TEXT("SekiroMaterialSetup: Created material instance '%s'."), *AssetName);
	}

	UE_LOG(LogTemp, Log,
		TEXT("SekiroMaterialSetup: Setup complete. Created %d material instances."),
		CreatedInstances.Num());

	return CreatedInstances;
}
