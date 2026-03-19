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
#include "Materials/Material.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
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

	static FString NormalizeMaterialSlotName(const FString& SlotName)
	{
		FString Normalized = SlotName;
		Normalized.ReplaceInline(TEXT(" "), TEXT("_"));
		Normalized.ReplaceInline(TEXT("|"), TEXT("_"));
		Normalized.ReplaceInline(TEXT("#"), TEXT("_"));
		return Normalized;
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

	static FString GetPackagePathFromObjectPath(const FString& ObjectPath)
	{
		FString PackagePath;
		if (ObjectPath.Split(TEXT("."), &PackagePath, nullptr))
		{
			return PackagePath;
		}

		return ObjectPath;
	}

	template <typename TExpression>
	static TExpression* AddExpression(UMaterial* Material, int32 X, int32 Y)
	{
		TExpression* Expression = NewObject<TExpression>(Material);
		Material->GetExpressionCollection().AddExpression(Expression);
		Expression->MaterialExpressionEditorX = X;
		Expression->MaterialExpressionEditorY = Y;
		return Expression;
	}

	static UMaterialExpressionTextureSampleParameter2D* AddTextureParameter(
		UMaterial* Material,
		const TCHAR* ParameterName,
		EMaterialSamplerType SamplerType,
		UTexture* DefaultTexture,
		int32 X,
		int32 Y)
	{
		auto* Expression = AddExpression<UMaterialExpressionTextureSampleParameter2D>(Material, X, Y);
		Expression->ParameterName = FName(ParameterName);
		Expression->SamplerType = SamplerType;
		Expression->Texture = DefaultTexture;
		return Expression;
	}

	static UMaterialExpressionScalarParameter* AddScalarParameter(
		UMaterial* Material,
		const TCHAR* ParameterName,
		float DefaultValue,
		int32 X,
		int32 Y)
	{
		auto* Expression = AddExpression<UMaterialExpressionScalarParameter>(Material, X, Y);
		Expression->ParameterName = FName(ParameterName);
		Expression->DefaultValue = DefaultValue;
		return Expression;
	}

	static UMaterialExpressionTextureCoordinate* AddTextureCoordinate(
		UMaterial* Material,
		int32 X,
		int32 Y)
	{
		auto* Expression = AddExpression<UMaterialExpressionTextureCoordinate>(Material, X, Y);
		Expression->CoordinateIndex = 0;
		Expression->UTiling = 1.0f;
		Expression->VTiling = 1.0f;
		return Expression;
	}

	static UMaterialExpressionMultiply* AddMultiply(UMaterial* Material, UMaterialExpression* A, UMaterialExpression* B, int32 X, int32 Y)
	{
		auto* Multiply = AddExpression<UMaterialExpressionMultiply>(Material, X, Y);
		Multiply->A.Expression = A;
		Multiply->B.Expression = B;
		return Multiply;
	}

	static void AttachScaledUv(
		UMaterial* Material,
		UMaterialExpressionTextureSampleParameter2D* TextureParameter,
		const TCHAR* ParameterName,
		int32 X,
		int32 Y)
	{
		if (!TextureParameter)
		{
			return;
		}

		auto* TexCoord = AddTextureCoordinate(Material, X, Y);
		auto* UScale = AddScalarParameter(Material, *FString::Printf(TEXT("%s_UScale"), ParameterName), 1.0f, X, Y + 140);
		auto* VScale = AddScalarParameter(Material, *FString::Printf(TEXT("%s_VScale"), ParameterName), 1.0f, X, Y + 260);
		auto* ScaleVector = AddExpression<UMaterialExpressionAppendVector>(Material, X + 220, Y + 200);
		ScaleVector->A.Expression = UScale;
		ScaleVector->B.Expression = VScale;
		auto* ScaledUv = AddMultiply(Material, TexCoord, ScaleVector, X + 460, Y + 80);
		TextureParameter->Coordinates.Expression = ScaledUv;
	}

	static UMaterialExpressionComponentMask* AddMaskR(UMaterial* Material, UMaterialExpression* Input, int32 X, int32 Y)
	{
		auto* Mask = AddExpression<UMaterialExpressionComponentMask>(Material, X, Y);
		Mask->Input.Expression = Input;
		Mask->R = true;
		return Mask;
	}

	static UMaterialExpressionLinearInterpolate* AddLerp(UMaterial* Material, UMaterialExpression* A, UMaterialExpression* B, UMaterialExpression* Alpha, int32 X, int32 Y)
	{
		auto* Lerp = AddExpression<UMaterialExpressionLinearInterpolate>(Material, X, Y);
		Lerp->A.Expression = A;
		Lerp->B.Expression = B;
		Lerp->Alpha.Expression = Alpha;
		return Lerp;
	}

	static UMaterialExpressionAdd* AddAdd(UMaterial* Material, UMaterialExpression* A, UMaterialExpression* B, int32 X, int32 Y)
	{
		auto* Add = AddExpression<UMaterialExpressionAdd>(Material, X, Y);
		Add->A.Expression = A;
		Add->B.Expression = B;
		return Add;
	}

	static UMaterial* EnsureFormalMasterMaterial(const FString& MaterialObjectPath, TArray<FString>& OutErrors)
	{
		UMaterial* Material = Cast<UMaterial>(StaticLoadObject(UMaterial::StaticClass(), nullptr, *MaterialObjectPath));
		const bool bCreatedNewMaterial = (Material == nullptr);

		const FString PackagePath = GetPackagePathFromObjectPath(MaterialObjectPath);
		const FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);
		UPackage* Package = CreatePackage(*PackagePath);
		if (!Package)
		{
			OutErrors.Add(FString::Printf(TEXT("Failed to create master material package: %s"), *PackagePath));
			return nullptr;
		}

		Package->FullyLoad();
		if (!Material)
		{
			Material = NewObject<UMaterial>(Package, *AssetName, RF_Public | RF_Standalone);
		}
		if (!Material)
		{
			OutErrors.Add(FString::Printf(TEXT("Failed to instantiate master material asset: %s"), *MaterialObjectPath));
			return nullptr;
		}

		Material->Modify();
		Material->PreEditChange(nullptr);
		Material->GetExpressionCollection().Empty();

		UMaterialEditorOnlyData* EditorOnlyData = Material->GetEditorOnlyData();
		if (!EditorOnlyData)
		{
			OutErrors.Add(FString::Printf(TEXT("Failed to access editor-only material data: %s"), *MaterialObjectPath));
			return nullptr;
		}

		UTexture* WhiteTexture = LoadObject<UTexture>(nullptr, TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
		UTexture* BlackTexture = LoadObject<UTexture>(nullptr, TEXT("/Engine/EngineResources/Black.Black"));
		UTexture* DefaultNormal = LoadObject<UTexture>(nullptr, TEXT("/Engine/EngineMaterials/DefaultNormal.DefaultNormal"));

		auto* BaseColor = AddTextureParameter(Material, TEXT("BaseColor"), SAMPLERTYPE_Color, WhiteTexture, -1600, -500);
		auto* BaseColor2 = AddTextureParameter(Material, TEXT("BaseColor2"), SAMPLERTYPE_Color, WhiteTexture, -1600, -320);
		auto* Normal = AddTextureParameter(Material, TEXT("Normal"), SAMPLERTYPE_Normal, DefaultNormal, -1600, -120);
		auto* Normal2 = AddTextureParameter(Material, TEXT("Normal2"), SAMPLERTYPE_Normal, DefaultNormal, -1600, 60);
		auto* Specular = AddTextureParameter(Material, TEXT("Specular"), SAMPLERTYPE_Color, WhiteTexture, -1600, 240);
		auto* Specular2 = AddTextureParameter(Material, TEXT("Specular2"), SAMPLERTYPE_Color, WhiteTexture, -1600, 420);
		auto* Roughness = AddTextureParameter(Material, TEXT("Roughness"), SAMPLERTYPE_Color, WhiteTexture, -1600, 600);
		auto* Roughness2 = AddTextureParameter(Material, TEXT("Roughness2"), SAMPLERTYPE_Color, WhiteTexture, -1600, 780);
		auto* Emissive = AddTextureParameter(Material, TEXT("Emissive"), SAMPLERTYPE_Color, BlackTexture, -1600, 980);
		auto* Emissive2 = AddTextureParameter(Material, TEXT("Emissive2"), SAMPLERTYPE_Color, BlackTexture, -1600, 1160);
		auto* BlendMask = AddTextureParameter(Material, TEXT("BlendMask"), SAMPLERTYPE_Color, BlackTexture, -1600, 1360);
		auto* BlendMask3 = AddTextureParameter(Material, TEXT("BlendMask3"), SAMPLERTYPE_Color, BlackTexture, -1600, 1540);

		AttachScaledUv(Material, BaseColor, TEXT("BaseColor"), -2200, -620);
		AttachScaledUv(Material, BaseColor2, TEXT("BaseColor2"), -2200, -360);
		AttachScaledUv(Material, Normal, TEXT("Normal"), -2200, -80);
		AttachScaledUv(Material, Normal2, TEXT("Normal2"), -2200, 180);
		AttachScaledUv(Material, Specular, TEXT("Specular"), -2200, 460);
		AttachScaledUv(Material, Specular2, TEXT("Specular2"), -2200, 720);
		AttachScaledUv(Material, Roughness, TEXT("Roughness"), -2200, 1000);
		AttachScaledUv(Material, Roughness2, TEXT("Roughness2"), -2200, 1260);
		AttachScaledUv(Material, Emissive, TEXT("Emissive"), -2200, 1540);
		AttachScaledUv(Material, Emissive2, TEXT("Emissive2"), -2200, 1800);
		AttachScaledUv(Material, BlendMask, TEXT("BlendMask"), -2200, 2080);
		AttachScaledUv(Material, BlendMask3, TEXT("BlendMask3"), -2200, 2340);

		auto* BlendMaskR = AddMaskR(Material, BlendMask, -1300, 1360);
		auto* BlendMask3R = AddMaskR(Material, BlendMask3, -1300, 1540);
		auto* SpecularR = AddMaskR(Material, Specular, -1300, 240);
		auto* Specular2R = AddMaskR(Material, Specular2, -1300, 420);
		auto* RoughnessR = AddMaskR(Material, Roughness, -1300, 600);
		auto* Roughness2R = AddMaskR(Material, Roughness2, -1300, 780);

		auto* BaseColorBlend = AddLerp(Material, BaseColor, BaseColor2, BlendMaskR, -1000, -420);
		auto* NormalBlend = AddLerp(Material, Normal, Normal2, BlendMaskR, -1000, -60);
		auto* SpecularBlend = AddLerp(Material, SpecularR, Specular2R, BlendMaskR, -1000, 320);
		auto* RoughnessBlend = AddLerp(Material, RoughnessR, Roughness2R, BlendMaskR, -1000, 680);
		auto* EmissiveBlend = AddLerp(Material, Emissive, Emissive2, BlendMaskR, -1000, 1060);
		auto* Zero = AddExpression<UMaterialExpressionConstant>(Material, -1000, 1540);
		Zero->R = 0.0f;
		auto* BlendMask3NoOp = AddMultiply(Material, BlendMask3R, Zero, -700, 1540);
		auto* EmissiveFinal = AddAdd(Material, EmissiveBlend, BlendMask3NoOp, -700, 1120);

		EditorOnlyData->BaseColor.Connect(0, BaseColorBlend);
		EditorOnlyData->Normal.Connect(0, NormalBlend);
		EditorOnlyData->Specular.Connect(0, SpecularBlend);
		EditorOnlyData->Roughness.Connect(0, RoughnessBlend);
		EditorOnlyData->EmissiveColor.Connect(0, EmissiveFinal);
		Material->bUsedWithSkeletalMesh = true;
		Material->TwoSided = false;
		Material->PostEditChange();
		Material->MarkPackageDirty();
		if (bCreatedNewMaterial)
		{
			FAssetRegistryModule::AssetCreated(Material);
		}

		const FString PackageFilename = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		UPackage::SavePackage(Package, Material, *PackageFilename, SaveArgs);
		return Material;
	}

	static void ApplyTextureImportSettings(const TSharedPtr<FJsonObject>& RootObject, const FString& OutputPackagePath, TArray<FString>& OutErrors)
	{
		const TArray<TSharedPtr<FJsonValue>>* MaterialsArray = nullptr;
		if (!RootObject.IsValid() || !RootObject->TryGetArrayField(TEXT("materials"), MaterialsArray) || !MaterialsArray)
		{
			return;
		}

		const FString TexturesPackagePath = OutputPackagePath / TEXT("Textures");
		TSet<FString> ProcessedTextures;

		for (const TSharedPtr<FJsonValue>& MaterialValue : *MaterialsArray)
		{
			const TSharedPtr<FJsonObject> MaterialObj = MaterialValue->AsObject();
			if (!MaterialObj.IsValid())
			{
				continue;
			}

			const TSharedPtr<FJsonObject>* TextureBindingsObj = nullptr;
			if (!MaterialObj->TryGetObjectField(TEXT("textureBindings"), TextureBindingsObj) || !TextureBindingsObj || !TextureBindingsObj->IsValid())
			{
				continue;
			}

			for (const auto& BindingPair : (*TextureBindingsObj)->Values)
			{
				const TSharedPtr<FJsonObject> BindingObj = BindingPair.Value->AsObject();
				if (!BindingObj.IsValid())
				{
					continue;
				}

				FString TextureFilename;
				if (!BindingObj->TryGetStringField(TEXT("exportedFileName"), TextureFilename) || TextureFilename.IsEmpty())
				{
					continue;
				}

				const FString TextureBaseName = FPaths::GetBaseFilename(TextureFilename);
				if (ProcessedTextures.Contains(TextureBaseName))
				{
					continue;
				}
				ProcessedTextures.Add(TextureBaseName);

				UTexture2D* Texture = FindTextureAsset(TextureBaseName, TexturesPackagePath);
				if (!Texture)
				{
					OutErrors.Add(FString::Printf(TEXT("Manifest texture '%s' was not imported as a Texture2D asset."), *TextureBaseName));
					continue;
				}

				FString SlotType;
				BindingObj->TryGetStringField(TEXT("slotType"), SlotType);
				bool bDirty = false;

				if (SlotType.StartsWith(TEXT("Normal")))
				{
					Texture->SRGB = false;
					Texture->CompressionSettings = TC_Normalmap;
					bDirty = true;
				}
				else if (SlotType.StartsWith(TEXT("Specular")) || SlotType.StartsWith(TEXT("Roughness")) || SlotType.StartsWith(TEXT("BlendMask")))
				{
					Texture->SRGB = false;
					Texture->CompressionSettings = TC_Masks;
					bDirty = true;
				}
				else if (SlotType.StartsWith(TEXT("BaseColor")) || SlotType.StartsWith(TEXT("Emissive")))
				{
					Texture->SRGB = true;
					Texture->CompressionSettings = TC_Default;
					bDirty = true;
				}

				if (bDirty)
				{
					Texture->PostEditChange();
					Texture->MarkPackageDirty();
					const FString TexturePackagePath = FString::Printf(TEXT("%s/%s"), *TexturesPackagePath, *TextureBaseName);
					const FString TexturePackageFilename = FPackageName::LongPackageNameToFilename(TexturePackagePath, FPackageName::GetAssetPackageExtension());
					FSavePackageArgs SaveArgs;
					SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
					UPackage::SavePackage(Texture->GetOutermost(), Texture, *TexturePackageFilename, SaveArgs);
				}
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
	UMaterialInterface* ParentMaterial = SekiroMaterialSetupInternal::EnsureFormalMasterMaterial(ParentMaterialPath, OutErrors);
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

	SekiroMaterialSetupInternal::ApplyTextureImportSettings(RootObject, OutputPackagePath, OutErrors);

	const FString TexturesPackagePath = OutputPackagePath / TEXT("Textures");
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
		const FString NormalizedSlotName = SekiroMaterialSetupInternal::NormalizeMaterialSlotName(SlotName);
		if (NormalizedSlotName.IsEmpty()) continue;

		const FString AssetName = SekiroMaterialSetupInternal::GetMaterialInstanceAssetName(MaterialObj);
		const FString AssetPath = FString::Printf(TEXT("%s/Materials/%s.%s"), *ChrContent, *AssetName, *AssetName);

		if (UMaterialInterface* Mat = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, *AssetPath)))
		{
			SlotToMaterial.Add(NormalizedSlotName, Mat);
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
		const FString NormalizedMatSlotName = SekiroMaterialSetupInternal::NormalizeMaterialSlotName(MatSlotName);
		if (UMaterialInterface** Found = SlotToMaterial.Find(NormalizedMatSlotName))
		{
			SkelMat.MaterialInterface = *Found;
			bChanged = true;
			BoundSlots.Add(NormalizedMatSlotName);
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
			UE_LOG(LogTemp, Warning, TEXT("SekiroMaterialSetup: Manifest slot '%s' was not found on the skeletal mesh and will be ignored."), *Pair.Key);
		}
	}

	if (bChanged)
	{
		SkelMesh->MarkPackageDirty();
	}

	return OutErrors.Num() == 0;
}
