#include "Import/SekiroImportCommandlet.h"
#include "Factories/FbxFactory.h"
#include "Factories/FbxImportUI.h"
#include "Factories/TextureFactory.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimSequence.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetImportTask.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "InterchangeManager.h"
#include "InterchangeSourceData.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "ReferenceSkeleton.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/SavePackage.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "Misc/FileHelper.h"
#include "Import/SekiroAssetImporter.h"

namespace
{
	struct FGltfPayload
	{
		FString JsonString;
		TArray<uint8> BinaryChunk;
	};

	constexpr double GltfToUeLengthScale = 100.0;

	static bool LoadGltfPayload(const FString& FilePath, FGltfPayload& OutPayload)
	{
		const FString Extension = FPaths::GetExtension(FilePath).ToLower();
		if (Extension == TEXT("gltf"))
		{
			return FFileHelper::LoadFileToString(OutPayload.JsonString, *FilePath);
		}

		TArray<uint8> FileData;
		if (!FFileHelper::LoadFileToArray(FileData, *FilePath) || FileData.Num() < 20)
		{
			return false;
		}

		auto ReadUInt32 = [&FileData](int32 Offset) -> uint32
		{
			return static_cast<uint32>(FileData[Offset])
				| (static_cast<uint32>(FileData[Offset + 1]) << 8)
				| (static_cast<uint32>(FileData[Offset + 2]) << 16)
				| (static_cast<uint32>(FileData[Offset + 3]) << 24);
		};

		const uint32 Magic = ReadUInt32(0);
		const uint32 Version = ReadUInt32(4);
		if (Magic != 0x46546C67 || Version != 2)
		{
			return false;
		}

		int32 Offset = 12;
		while (Offset + 8 <= FileData.Num())
		{
			const uint32 ChunkLength = ReadUInt32(Offset);
			const uint32 ChunkType = ReadUInt32(Offset + 4);
			Offset += 8;

			if (Offset + static_cast<int32>(ChunkLength) > FileData.Num())
			{
				return false;
			}

			if (ChunkType == 0x4E4F534A)
			{
				OutPayload.JsonString = FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(FileData.GetData() + Offset))).Left(static_cast<int32>(ChunkLength));
			}
			else if (ChunkType == 0x004E4942)
			{
				OutPayload.BinaryChunk.SetNumUninitialized(static_cast<int32>(ChunkLength));
				FMemory::Memcpy(OutPayload.BinaryChunk.GetData(), FileData.GetData() + Offset, ChunkLength);
			}

			Offset += static_cast<int32>(ChunkLength);
		}

		return !OutPayload.JsonString.IsEmpty();
	}

	static FTransform GetImportedRootOrientationCorrection()
	{
		return FTransform(FQuat(FVector::YAxisVector, PI), FVector::ZeroVector, FVector(-1.0, 1.0, 1.0));
	}

	static FMatrix MakeGltfToUeBasisMatrix()
	{
		FMatrix Basis = FMatrix::Identity;
		const FVector XAxis(0.0, 0.0, -1.0);
		const FVector YAxis(-1.0, 0.0, 0.0);
		const FVector ZAxis(0.0, -1.0, 0.0);
		const FVector Origin = FVector::ZeroVector;
		Basis.SetAxes(&XAxis, &YAxis, &ZAxis, &Origin);
		return Basis;
	}

	static FVector MapGltfVectorToUeBasis(const FVector& InVector)
	{
		return FVector(-InVector.Y, -InVector.Z, -InVector.X);
	}

	static FTransform ConvertGltfLocalTransformToUeBasis(const FTransform& InTransform)
	{
		static const FMatrix GltfToUeBasis = MakeGltfToUeBasisMatrix();
		static const FMatrix UeToGltfBasis = GltfToUeBasis.GetTransposed();

		const FMatrix InMatrix = InTransform.ToMatrixWithScale();
		FMatrix OutMatrix = GltfToUeBasis * InMatrix * UeToGltfBasis;
		OutMatrix.SetOrigin(OutMatrix.GetOrigin() * GltfToUeLengthScale);
		return FTransform(OutMatrix);
	}

	static void ApplyImportedRootOrientationCorrection(USkeleton* Skeleton, USkeletalMesh* SkeletalMesh)
	{
		const FTransform RootCorrection = GetImportedRootOrientationCorrection();

		if (Skeleton)
		{
			FReferenceSkeleton& SkeletonRef = const_cast<FReferenceSkeleton&>(Skeleton->GetReferenceSkeleton());
			if (SkeletonRef.GetNum() > 0)
			{
				FReferenceSkeletonModifier Modifier(SkeletonRef, Skeleton);
				const FTransform RootPose = SkeletonRef.GetRefBonePose()[0];
				Modifier.UpdateRefPoseTransform(0, RootPose * RootCorrection);
				Skeleton->MarkPackageDirty();
			}
		}

		if (SkeletalMesh)
		{
			FReferenceSkeleton& MeshRef = const_cast<FReferenceSkeleton&>(SkeletalMesh->GetRefSkeleton());
			if (MeshRef.GetNum() > 0)
			{
				FReferenceSkeletonModifier Modifier(MeshRef, Skeleton);
				const FTransform RootPose = MeshRef.GetRefBonePose()[0];
				Modifier.UpdateRefPoseTransform(0, RootPose * RootCorrection);
				SkeletalMesh->CalculateInvRefMatrices();
				SkeletalMesh->MarkPackageDirty();
			}
		}
	}

	static FTransform ParseGltfNodeLocalTransform(const TSharedPtr<FJsonObject>& NodeObject)
	{
		if (!NodeObject.IsValid())
			return FTransform::Identity;

		const TArray<TSharedPtr<FJsonValue>>* MatrixArray = nullptr;
		if (NodeObject->TryGetArrayField(TEXT("matrix"), MatrixArray) && MatrixArray && MatrixArray->Num() == 16)
		{
			const FVector XAxis((*MatrixArray)[0]->AsNumber(), (*MatrixArray)[1]->AsNumber(), (*MatrixArray)[2]->AsNumber());
			const FVector YAxis((*MatrixArray)[4]->AsNumber(), (*MatrixArray)[5]->AsNumber(), (*MatrixArray)[6]->AsNumber());
			const FVector ZAxis((*MatrixArray)[8]->AsNumber(), (*MatrixArray)[9]->AsNumber(), (*MatrixArray)[10]->AsNumber());
			const FVector Origin((*MatrixArray)[12]->AsNumber(), (*MatrixArray)[13]->AsNumber(), (*MatrixArray)[14]->AsNumber());

			FMatrix Matrix = FMatrix::Identity;
			Matrix.SetAxes(&XAxis, &YAxis, &ZAxis, &Origin);
			return FTransform(Matrix);
		}

		FVector Translation = FVector::ZeroVector;
		const TArray<TSharedPtr<FJsonValue>>* TranslationArray = nullptr;
		if (NodeObject->TryGetArrayField(TEXT("translation"), TranslationArray) && TranslationArray && TranslationArray->Num() == 3)
		{
			Translation = FVector((*TranslationArray)[0]->AsNumber(), (*TranslationArray)[1]->AsNumber(), (*TranslationArray)[2]->AsNumber());
		}

		FQuat Rotation = FQuat::Identity;
		const TArray<TSharedPtr<FJsonValue>>* RotationArray = nullptr;
		if (NodeObject->TryGetArrayField(TEXT("rotation"), RotationArray) && RotationArray && RotationArray->Num() == 4)
		{
			Rotation = FQuat((*RotationArray)[0]->AsNumber(), (*RotationArray)[1]->AsNumber(), (*RotationArray)[2]->AsNumber(), (*RotationArray)[3]->AsNumber());
			Rotation.Normalize();
		}

		FVector Scale = FVector::OneVector;
		const TArray<TSharedPtr<FJsonValue>>* ScaleArray = nullptr;
		if (NodeObject->TryGetArrayField(TEXT("scale"), ScaleArray) && ScaleArray && ScaleArray->Num() == 3)
		{
			Scale = FVector((*ScaleArray)[0]->AsNumber(), (*ScaleArray)[1]->AsNumber(), (*ScaleArray)[2]->AsNumber());
		}

		return FTransform(Rotation, Translation, Scale);
	}
}

USekiroImportCommandlet::USekiroImportCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 USekiroImportCommandlet::Main(const FString& Params)
{
	FString ExportDir;
	FString ChrFilterStr;
	FString ContentBase = TEXT("/Game/SekiroAssets/Characters");

	TArray<FString> Tokens;
	TArray<FString> Switches;
	TMap<FString, FString> ParamsMap;
	ParseCommandLine(*Params, Tokens, Switches, ParamsMap);

	if (ParamsMap.Contains(TEXT("ExportDir")))
		ExportDir = ParamsMap[TEXT("ExportDir")];
	else
		ExportDir = TEXT("E:/Sekiro/Export");

	if (ParamsMap.Contains(TEXT("ChrFilter")))
		ChrFilterStr = ParamsMap[TEXT("ChrFilter")];

	if (ParamsMap.Contains(TEXT("ContentBase")))
		ContentBase = ParamsMap[TEXT("ContentBase")];

	UE_LOG(LogTemp, Display, TEXT("=== Sekiro Import Commandlet ==="));
	UE_LOG(LogTemp, Display, TEXT("ExportDir: %s"), *ExportDir);

	TArray<FString> ChrDirs;
	IFileManager::Get().FindFiles(ChrDirs, *(ExportDir / TEXT("*")), false, true);

	TArray<FString> ChrFilter;
	if (!ChrFilterStr.IsEmpty())
		ChrFilterStr.ParseIntoArray(ChrFilter, TEXT(","));

	TArray<FString> ValidChrs;
	for (const FString& Dir : ChrDirs)
	{
		if (Dir.StartsWith(TEXT("c")) && Dir.Len() >= 4)
		{
			if (ChrFilter.Num() == 0 || ChrFilter.Contains(Dir))
				ValidChrs.Add(Dir);
		}
	}
	ValidChrs.Sort();

	UE_LOG(LogTemp, Display, TEXT("Found %d character(s) to import"), ValidChrs.Num());

	for (int32 i = 0; i < ValidChrs.Num(); i++)
	{
		const FString& ChrId = ValidChrs[i];
		UE_LOG(LogTemp, Display, TEXT("[%d/%d] Importing %s..."), i + 1, ValidChrs.Num(), *ChrId);
		ImportCharacter(ChrId, ExportDir / ChrId, ContentBase);

		if ((i + 1) % 5 == 0)
			CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	}

	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	UE_LOG(LogTemp, Display, TEXT("=== Import Complete ==="));
	return 0;
}

void USekiroImportCommandlet::ImportCharacter(const FString& ChrId, const FString& ExportDir, const FString& ContentBase)
{
	FString ChrContent = ContentBase / ChrId;
	int32 TexCount = 0, AnimCount = 0;

	// 1. Import textures (PNG only)
	FString TexDir = ExportDir / TEXT("Textures");
	TArray<FString> TexFiles;
	IFileManager::Get().FindFiles(TexFiles, *(TexDir / TEXT("*.png")), true, false);
	TArray<FString> TgaFiles;
	IFileManager::Get().FindFiles(TgaFiles, *(TexDir / TEXT("*.tga")), true, false);
	TexFiles.Append(TgaFiles);

	for (const FString& TexFile : TexFiles)
	{
		FString AbsPath = FPaths::ConvertRelativePathToFull(TexDir / TexFile);
		UObject* Result = ImportTexture(AbsPath, ChrContent / TEXT("Textures"));
		if (Result) TexCount++;
	}

	if (TexCount > 0)
		UE_LOG(LogTemp, Display, TEXT("  Textures: %d"), TexCount);

	// 2. Import model - find best formal format: FBX > glTF > GLB
	FString ModelDir = ExportDir / TEXT("Model");
	FString ModelFile = FindBestModelFile(ModelDir, ChrId);

	USkeleton* Skeleton = nullptr;

	if (!ModelFile.IsEmpty())
	{
		FString Extension = FPaths::GetExtension(ModelFile).ToLower();
		UE_LOG(LogTemp, Display, TEXT("  Importing model: %s (format: %s)"), *FPaths::GetCleanFilename(ModelFile), *Extension);

		UObject* Result = nullptr;

		if (Extension == TEXT("gltf") || Extension == TEXT("glb"))
		{
			// Use AssetImportTask for glTF - routes through Interchange framework
			// which properly handles skeleton/skin data
			Result = ImportViaAssetTask(ModelFile, ChrContent / TEXT("Mesh"));
		}
		else
		{
			// FBX/DAE - use FBX factory
			Result = ImportMeshViaFbxFactory(ModelFile, ChrContent / TEXT("Mesh"));
		}

		if (Result)
		{
			USkeletalMesh* SkelMesh = Cast<USkeletalMesh>(Result);
			if (SkelMesh)
			{
				UE_LOG(LogTemp, Display, TEXT("  Model: OK (SkeletalMesh)"));
				Skeleton = SkelMesh->GetSkeleton();
				ApplyImportedRootOrientationCorrection(Skeleton, SkelMesh);
				SavePackage(SkelMesh);
				if (Skeleton)
					SavePackage(Skeleton);
				if (Skeleton)
					UE_LOG(LogTemp, Display, TEXT("  Skeleton: %s"), *Skeleton->GetPathName());
			}
			else
			{
				// Interchange might return a different object type, check for skeleton in package
				UE_LOG(LogTemp, Display, TEXT("  Model: OK (%s) - checking for skeleton..."),
					*Result->GetClass()->GetName());

				// Try to find skeleton asset that was created alongside the mesh
				Skeleton = FindSkeletonInPackage(ChrContent / TEXT("Mesh"));
				if (Skeleton)
				{
					ApplyImportedRootOrientationCorrection(Skeleton, nullptr);
					SavePackage(Skeleton);
				}
				if (Skeleton)
					UE_LOG(LogTemp, Display, TEXT("  Skeleton found: %s"), *Skeleton->GetPathName());
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("  Model: FAILED (%s)"), *FPaths::GetCleanFilename(ModelFile));
		}
	}

	// 3. Import animations
	// NOTE: For animations, prefer FBX/DAE over glTF because:
	//   - FBX factory can specify a target skeleton (ImportUI->Skeleton)
	//   - Interchange (glTF) cannot bind animation to an existing skeleton
	//   - Animation files are skeleton-only (no mesh), so FBX bone limit is not an issue
	if (Skeleton)
	{
		FString AnimDir = ExportDir / TEXT("Animations");
		TArray<FString> AllAnimFiles;

		// Collect animation files, deduplicated by base name (prefer glTF > FBX)
		// glTF is preferred because we parse it directly into AnimSequence with skeleton binding
		TMap<FString, FString> BestAnimFiles; // BaseName -> FullPath
		TArray<TPair<FString, int32>> FormatPriority = {
			{TEXT("*.gltf"), 0}, {TEXT("*.glb"), 1}, {TEXT("*.fbx"), 2}
		};
		for (const auto& Fmt : FormatPriority)
		{
			TArray<FString> TempFiles;
			IFileManager::Get().FindFiles(TempFiles, *(AnimDir / Fmt.Key), true, false);
			for (const FString& F : TempFiles)
			{
				FString BaseName = FPaths::GetBaseFilename(F);
				if (!BestAnimFiles.Contains(BaseName))
					BestAnimFiles.Add(BaseName, FPaths::ConvertRelativePathToFull(AnimDir / F));
			}
		}
		BestAnimFiles.GenerateValueArray(AllAnimFiles);
		AllAnimFiles.Sort();

		for (const FString& AnimFile : AllAnimFiles)
		{
			FString Ext = FPaths::GetExtension(AnimFile).ToLower();
			UObject* Result = nullptr;

			if (Ext == TEXT("gltf") || Ext == TEXT("glb"))
			{
				// Parse glTF directly and create AnimSequence programmatically
				Result = ImportAnimationFromGltf(AnimFile, ChrContent / TEXT("Animations"), Skeleton);
			}
			else
			{
				Result = ImportAnimationViaFbxFactory(AnimFile, ChrContent / TEXT("Animations"), Skeleton);
			}

			if (Result) AnimCount++;
		}

		if (AnimCount > 0)
			UE_LOG(LogTemp, Display, TEXT("  Animations: %d"), AnimCount);

		if (AnimCount > 30)
			CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("  Animations: SKIPPED (no skeleton)"));
	}

	// 4. Import skill configs
	FString SkillDir = ExportDir / TEXT("Skills");
	FString SkillSrc = SkillDir / TEXT("skill_config.json");
	if (FPaths::FileExists(SkillSrc))
	{
		FString SkillDst = FPaths::ProjectContentDir() / TEXT("SekiroAssets/Characters") / ChrId / TEXT("Skills/skill_config.json");
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(SkillDst), true);
		IFileManager::Get().Copy(*SkillDst, *SkillSrc, true);
		TArray<USekiroSkillDataAsset*> ImportedSkillAssets = USekiroAssetImporter::ImportSkillConfig(SkillSrc, ChrContent / TEXT("Skills"));
		UE_LOG(LogTemp, Display, TEXT("  Skills: imported %d skill assets"), ImportedSkillAssets.Num());
	}
}

FString USekiroImportCommandlet::FindBestModelFile(const FString& ModelDir, const FString& ChrId)
{
	// Priority: FBX > glTF > GLB
	TArray<TPair<FString, FString>> Extensions = {
		{TEXT("*.fbx"), TEXT("")},
		{TEXT("*.gltf"), TEXT("")},
		{TEXT("*.glb"), TEXT("")}
	};

	for (const auto& Pair : Extensions)
	{
		TArray<FString> Files;
		IFileManager::Get().FindFiles(Files, *(ModelDir / Pair.Key), true, false);
		if (Files.Num() > 0)
			return FPaths::ConvertRelativePathToFull(ModelDir / Files[0]);
	}

	return TEXT("");
}

UObject* USekiroImportCommandlet::ImportViaAssetTask(const FString& FilePath, const FString& DestPackagePath)
{
	// Use UInterchangeManager directly to avoid AssetTools/ContentBrowser notification
	// that crashes in commandlet mode (Slate not initialized)
	UInterchangeManager& InterchangeManager = UInterchangeManager::GetInterchangeManager();

	UInterchangeSourceData* SourceData = UInterchangeManager::CreateSourceData(FilePath);
	if (!SourceData)
	{
		UE_LOG(LogTemp, Warning, TEXT("    Failed to create source data for: %s"), *FilePath);
		return nullptr;
	}

	FImportAssetParameters ImportParams;
	ImportParams.bIsAutomated = true;
	ImportParams.bReplaceExisting = true;
	ImportParams.DestinationName = FPaths::GetBaseFilename(FilePath);

	TArray<UObject*> ImportedObjects;
	bool bSuccess = InterchangeManager.ImportAsset(DestPackagePath, SourceData, ImportParams, ImportedObjects);

	UObject* Result = nullptr;
	if (bSuccess && ImportedObjects.Num() > 0)
	{
		Result = ImportedObjects[0];
		UE_LOG(LogTemp, Display, TEXT("    Interchange import OK: %s -> %s (%d objects)"),
			*FPaths::GetCleanFilename(FilePath), *Result->GetClass()->GetName(), ImportedObjects.Num());

		// Save all directly returned imported objects first.
		for (UObject* Obj : ImportedObjects)
		{
			if (Obj) SavePackage(Obj);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("    Interchange import returned %s with %d objects for: %s"),
			bSuccess ? TEXT("true") : TEXT("false"), ImportedObjects.Num(), *FPaths::GetCleanFilename(FilePath));

		// Even if bSuccess is false, check if assets were created on disk
		// (Interchange sometimes reports failure but creates assets)
		if (ImportedObjects.Num() > 0)
		{
			Result = ImportedObjects[0];
			for (UObject* Obj : ImportedObjects)
			{
				if (Obj) SavePackage(Obj);
			}
		}
	}

	// Interchange can create related assets (Skeleton, SkeletalMesh, Materials, Textures)
	// without returning all of them in ImportedObjects. Save everything created under the
	// destination package path so later commandlet runs see the same on-disk assets.
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	TArray<FAssetData> AssetsInPath;
	AssetRegistry.GetAssetsByPath(FName(*DestPackagePath), AssetsInPath, true);
	for (const FAssetData& AssetData : AssetsInPath)
	{
		if (UObject* Asset = AssetData.GetAsset())
		{
			SavePackage(Asset);
		}
	}

	return Result;
}

UObject* USekiroImportCommandlet::ImportMeshViaFbxFactory(const FString& FilePath, const FString& DestPackagePath)
{
	FString FileName = FPaths::GetBaseFilename(FilePath);
	FString PackagePath = DestPackagePath / FileName;

	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package) return nullptr;
	Package->FullyLoad();

	UFbxFactory* Factory = NewObject<UFbxFactory>();
	Factory->AddToRoot();

	UFbxImportUI* ImportUI = NewObject<UFbxImportUI>();
	ImportUI->bImportMesh = true;
	ImportUI->bImportAsSkeletal = true;
	ImportUI->bImportAnimations = false;
	ImportUI->bImportMaterials = false;
	ImportUI->bImportTextures = false;
	ImportUI->bCreatePhysicsAsset = true;
	ImportUI->bIsReimport = false;
	ImportUI->bAutomatedImportShouldDetectType = false;
	Factory->SetDetectImportTypeOnImport(false);
	Factory->ImportUI = ImportUI;

	bool bCancelled = false;

	UObject* Result = UFactory::StaticImportObject(
		USkeletalMesh::StaticClass(),
		Package,
		FName(*FileName),
		RF_Public | RF_Standalone,
		bCancelled,
		*FilePath,
		nullptr,
		Factory);

	if (Result)
	{
		FAssetRegistryModule::AssetCreated(Result);
		SavePackage(Result);
	}

	Factory->RemoveFromRoot();
	return Result;
}

UObject* USekiroImportCommandlet::ImportAnimationViaFbxFactory(const FString& FilePath, const FString& DestPackagePath, USkeleton* Skeleton)
{
	if (!Skeleton) return nullptr;

	FString FileName = FPaths::GetBaseFilename(FilePath);
	FString PackagePath = DestPackagePath / FileName;

	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package) return nullptr;
	Package->FullyLoad();

	UFbxFactory* Factory = NewObject<UFbxFactory>();
	Factory->AddToRoot();

	UFbxImportUI* ImportUI = NewObject<UFbxImportUI>();
	ImportUI->bImportMesh = false;
	ImportUI->bImportAsSkeletal = true;
	ImportUI->bImportAnimations = true;
	ImportUI->Skeleton = Skeleton;
	ImportUI->bIsReimport = false;
	ImportUI->bAutomatedImportShouldDetectType = false;
	Factory->SetDetectImportTypeOnImport(false);
	Factory->ImportUI = ImportUI;

	bool bCancelled = false;

	UObject* Result = UFactory::StaticImportObject(
		UAnimSequence::StaticClass(),
		Package,
		FName(*FileName),
		RF_Public | RF_Standalone,
		bCancelled,
		*FilePath,
		nullptr,
		Factory);

	if (Result)
	{
		FAssetRegistryModule::AssetCreated(Result);
		SavePackage(Result);
	}

	Factory->RemoveFromRoot();
	return Result;
}

UObject* USekiroImportCommandlet::ImportTexture(const FString& FilePath, const FString& DestPackagePath)
{
	FString FileName = FPaths::GetBaseFilename(FilePath);
	FString PackagePath = DestPackagePath / FileName;

	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package) return nullptr;
	Package->FullyLoad();

	UTextureFactory* Factory = NewObject<UTextureFactory>();
	Factory->AddToRoot();

	bool bCancelled = false;
	UObject* Result = Factory->FactoryCreateFile(
		UTexture2D::StaticClass(), Package, FName(*FileName),
		RF_Public | RF_Standalone, FilePath, nullptr, GWarn, bCancelled);

	if (Result)
	{
		FAssetRegistryModule::AssetCreated(Result);
		SavePackage(Result);
	}

	Factory->RemoveFromRoot();
	return Result;
}

USkeleton* USekiroImportCommandlet::FindSkeletonInPackage(const FString& PackagePath)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> Assets;
	AssetRegistry.GetAssetsByPath(FName(*PackagePath), Assets, true);

	for (const FAssetData& Asset : Assets)
	{
		if (Asset.GetClass() == USkeleton::StaticClass())
		{
			return Cast<USkeleton>(Asset.GetAsset());
		}
	}

	// Also check for SkeletalMesh and get its skeleton
	for (const FAssetData& Asset : Assets)
	{
		UObject* LoadedAsset = Asset.GetAsset();
		USkeletalMesh* SkelMesh = Cast<USkeletalMesh>(LoadedAsset);
		if (SkelMesh && SkelMesh->GetSkeleton())
		{
			return SkelMesh->GetSkeleton();
		}
	}

	return nullptr;
}

UObject* USekiroImportCommandlet::ImportAnimationFromGltf(const FString& FilePath, const FString& DestPackagePath, USkeleton* Skeleton)
{
	if (!Skeleton) return nullptr;

	// 1. Load and parse glTF/GLB payload
	FGltfPayload Payload;
	if (!LoadGltfPayload(FilePath, Payload))
	{
		UE_LOG(LogTemp, Warning, TEXT("    Failed to read glTF/GLB payload: %s"), *FPaths::GetCleanFilename(FilePath));
		return nullptr;
	}

	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Payload.JsonString);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("    Failed to parse glTF JSON: %s"), *FPaths::GetCleanFilename(FilePath));
		return nullptr;
	}

	TArray<uint8> BinData;
	if (FPaths::GetExtension(FilePath).Equals(TEXT("glb"), ESearchCase::IgnoreCase))
	{
		BinData = Payload.BinaryChunk;
	}
	else
	{
		const TArray<TSharedPtr<FJsonValue>>* BuffersArray;
		if (!Root->TryGetArrayField(TEXT("buffers"), BuffersArray) || BuffersArray->Num() == 0)
			return nullptr;

		FString BinUri = (*BuffersArray)[0]->AsObject()->GetStringField(TEXT("uri"));
		FString BinPath = FPaths::GetPath(FilePath) / BinUri;
		if (!FFileHelper::LoadFileToArray(BinData, *BinPath))
		{
			UE_LOG(LogTemp, Warning, TEXT("    Failed to read binary: %s"), *BinUri);
			return nullptr;
		}
	}

	// 3. Parse node names for bone mapping
	const TArray<TSharedPtr<FJsonValue>>* NodesArray;
	if (!Root->TryGetArrayField(TEXT("nodes"), NodesArray))
		return nullptr;

	TArray<FString> NodeNames;
	NodeNames.SetNum(NodesArray->Num());
	TMap<FName, FTransform> GltfBindPoseByBone;
	for (int32 i = 0; i < NodesArray->Num(); i++)
	{
		TSharedPtr<FJsonObject> Node = (*NodesArray)[i]->AsObject();
		NodeNames[i] = Node->HasField(TEXT("name")) ? Node->GetStringField(TEXT("name")) : FString::Printf(TEXT("node_%d"), i);
		GltfBindPoseByBone.Add(FName(*NodeNames[i]), ParseGltfNodeLocalTransform(Node));
	}

	// 4. Parse accessors and bufferViews
	const TArray<TSharedPtr<FJsonValue>>* AccessorsArray;
	const TArray<TSharedPtr<FJsonValue>>* BufferViewsArray;
	if (!Root->TryGetArrayField(TEXT("accessors"), AccessorsArray) ||
		!Root->TryGetArrayField(TEXT("bufferViews"), BufferViewsArray))
		return nullptr;

	// Helper lambda: read accessor data as float array
	auto ReadAccessorFloats = [&](int32 AccessorIdx, int32 ExpectedComponents) -> TArray<float>
	{
		TArray<float> Result;
		if (AccessorIdx < 0 || AccessorIdx >= AccessorsArray->Num()) return Result;

		TSharedPtr<FJsonObject> Acc = (*AccessorsArray)[AccessorIdx]->AsObject();
		int32 Count = Acc->GetIntegerField(TEXT("count"));
		int32 BvIdx = Acc->GetIntegerField(TEXT("bufferView"));
		int32 AccOffset = Acc->HasField(TEXT("byteOffset")) ? Acc->GetIntegerField(TEXT("byteOffset")) : 0;

		if (BvIdx < 0 || BvIdx >= BufferViewsArray->Num()) return Result;
		TSharedPtr<FJsonObject> Bv = (*BufferViewsArray)[BvIdx]->AsObject();
		int32 BvOffset = Bv->HasField(TEXT("byteOffset")) ? Bv->GetIntegerField(TEXT("byteOffset")) : 0;

		int32 TotalOffset = BvOffset + AccOffset;
		int32 TotalFloats = Count * ExpectedComponents;
		Result.SetNum(TotalFloats);

		if (TotalOffset + TotalFloats * sizeof(float) <= (uint32)BinData.Num())
		{
			FMemory::Memcpy(Result.GetData(), BinData.GetData() + TotalOffset, TotalFloats * sizeof(float));
		}

		return Result;
	};

	// 5. Parse animations
	const TArray<TSharedPtr<FJsonValue>>* AnimationsArray;
	if (!Root->TryGetArrayField(TEXT("animations"), AnimationsArray) || AnimationsArray->Num() == 0)
		return nullptr;

	TSharedPtr<FJsonObject> Anim = (*AnimationsArray)[0]->AsObject();
	const TArray<TSharedPtr<FJsonValue>>& Channels = Anim->GetArrayField(TEXT("channels"));
	const TArray<TSharedPtr<FJsonValue>>& Samplers = Anim->GetArrayField(TEXT("samplers"));

	// 6. Collect keyframe data per bone
	struct FBoneAnimData
	{
		TArray<float> TimesT, TimesR, TimesS;
		TArray<FVector3f> Positions;
		TArray<FQuat4f> Rotations;
		TArray<FVector3f> Scales;
	};
	TMap<FName, FBoneAnimData> BoneData;

	float MaxTime = 0.0f;

	for (int32 i = 0; i < Channels.Num(); i++)
	{
		TSharedPtr<FJsonObject> Channel = Channels[i]->AsObject();
		int32 SamplerIdx = Channel->GetIntegerField(TEXT("sampler"));
		TSharedPtr<FJsonObject> Target = Channel->GetObjectField(TEXT("target"));
		int32 NodeIdx = Target->GetIntegerField(TEXT("node"));
		FString Path = Target->GetStringField(TEXT("path"));

		if (NodeIdx < 0 || NodeIdx >= NodeNames.Num() || SamplerIdx < 0 || SamplerIdx >= Samplers.Num())
			continue;

		FName BoneName(*NodeNames[NodeIdx]);

		TSharedPtr<FJsonObject> Sampler = Samplers[SamplerIdx]->AsObject();
		int32 InputAccessor = Sampler->GetIntegerField(TEXT("input"));
		int32 OutputAccessor = Sampler->GetIntegerField(TEXT("output"));

		TArray<float> Times = ReadAccessorFloats(InputAccessor, 1);
		for (float T : Times)
			MaxTime = FMath::Max(MaxTime, T);

		FBoneAnimData& Data = BoneData.FindOrAdd(BoneName);

		if (Path == TEXT("translation"))
		{
			Data.TimesT = Times;
			TArray<float> Values = ReadAccessorFloats(OutputAccessor, 3);
			Data.Positions.SetNum(Values.Num() / 3);
			for (int32 j = 0; j < Data.Positions.Num(); j++)
				Data.Positions[j] = FVector3f(Values[j * 3], Values[j * 3 + 1], Values[j * 3 + 2]);
		}
		else if (Path == TEXT("rotation"))
		{
			Data.TimesR = Times;
			TArray<float> Values = ReadAccessorFloats(OutputAccessor, 4);
			Data.Rotations.SetNum(Values.Num() / 4);
			for (int32 j = 0; j < Data.Rotations.Num(); j++)
				Data.Rotations[j] = FQuat4f(Values[j * 4], Values[j * 4 + 1], Values[j * 4 + 2], Values[j * 4 + 3]);
		}
		else if (Path == TEXT("scale"))
		{
			Data.TimesS = Times;
			TArray<float> Values = ReadAccessorFloats(OutputAccessor, 3);
			Data.Scales.SetNum(Values.Num() / 3);
			for (int32 j = 0; j < Data.Scales.Num(); j++)
				Data.Scales[j] = FVector3f(Values[j * 3], Values[j * 3 + 1], Values[j * 3 + 2]);
		}
	}

	if (BoneData.Num() == 0 || MaxTime <= 0.0f)
		return nullptr;

	// 7. Create AnimSequence
	FString FileName = FPaths::GetBaseFilename(FilePath);
	FString PackagePath = DestPackagePath / FileName;

	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package) return nullptr;
	Package->FullyLoad();

	UAnimSequence* AnimSeq = NewObject<UAnimSequence>(Package, FName(*FileName), RF_Public | RF_Standalone);
	AnimSeq->SetSkeleton(Skeleton);

	const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();
	UE_LOG(LogTemp, Display, TEXT("    glTF basis for %s: applying fixed glTF-to-UE local transform conversion"), *FileName);

	// 8. Populate bone tracks using IAnimationDataController
	IAnimationDataController& Controller = AnimSeq->GetController();
	Controller.InitializeModel();
	Controller.OpenBracket(FText::FromString(TEXT("Importing glTF animation")), false);

	// Determine frame rate and frame count from keyframe times
	// Use 30 fps as default, calculate frames from max time
	FFrameRate FrameRate(30, 1);
	Controller.SetFrameRate(FrameRate, false);
	int32 NumFrames = FMath::Max(1, FMath::RoundToInt32(MaxTime * 30.0f));
	Controller.SetNumberOfFrames(FFrameNumber(NumFrames), false);

	// Create a full track set using the imported skeleton reference pose as the
	// default local transform. If a parent bone is omitted here, UE falls back
	// to identity and the hierarchy collapses during evaluation.
	for (int32 BoneIdx = 0; BoneIdx < RefSkeleton.GetNum(); BoneIdx++)
	{
		const FName BoneName = RefSkeleton.GetBoneName(BoneIdx);
		const FBoneAnimData* Data = BoneData.Find(BoneName);

		// Resample all channels to uniform frame count
		// Each channel may have different sample counts, so resample to NumFrames+1 keys
		int32 NumKeys = NumFrames + 1;

		// Helper: resample a channel from arbitrary timestamps to uniform keys
		auto ResampleVec3 = [&](const TArray<float>& Times, const TArray<FVector3f>& Values, FVector3f Default) -> TArray<FVector3f>
		{
			TArray<FVector3f> Result;
			Result.SetNum(NumKeys);
			if (Times.Num() == 0 || Values.Num() == 0)
			{
				for (int32 k = 0; k < NumKeys; k++) Result[k] = Default;
				return Result;
			}
			for (int32 k = 0; k < NumKeys; k++)
			{
				float T = (float)k / 30.0f;
				// Find surrounding keyframes
				int32 Idx = 0;
				while (Idx < Times.Num() - 1 && Times[Idx + 1] <= T) Idx++;
				if (Idx >= Values.Num() - 1)
					Result[k] = Values.Last();
				else if (Times[Idx + 1] == Times[Idx])
					Result[k] = Values[Idx];
				else
				{
					float Alpha = (T - Times[Idx]) / (Times[Idx + 1] - Times[Idx]);
					Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
					Result[k] = FMath::Lerp(Values[Idx], Values[Idx + 1], Alpha);
				}
			}
			return Result;
		};

		auto ResampleQuat = [&](const TArray<float>& Times, const TArray<FQuat4f>& Values, FQuat4f Default) -> TArray<FQuat4f>
		{
			TArray<FQuat4f> Result;
			Result.SetNum(NumKeys);
			if (Times.Num() == 0 || Values.Num() == 0)
			{
				for (int32 k = 0; k < NumKeys; k++) Result[k] = Default;
				return Result;
			}
			for (int32 k = 0; k < NumKeys; k++)
			{
				float T = (float)k / 30.0f;
				int32 Idx = 0;
				while (Idx < Times.Num() - 1 && Times[Idx + 1] <= T) Idx++;
				if (Idx >= Values.Num() - 1)
					Result[k] = Values.Last();
				else if (Times[Idx + 1] == Times[Idx])
					Result[k] = Values[Idx];
				else
				{
					float Alpha = (T - Times[Idx]) / (Times[Idx + 1] - Times[Idx]);
					Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
					Result[k] = FQuat4f::Slerp(Values[Idx], Values[Idx + 1], Alpha);
				}
			}
			return Result;
		};

		const FTransform DefaultUePose = RefSkeleton.GetRefBonePose()[BoneIdx];
		const FTransform GltfBindLocal = GltfBindPoseByBone.FindRef(BoneName);
		const FTransform GltfBindPose = ConvertGltfLocalTransformToUeBasis(GltfBindLocal);

		TArray<FVector3f> GltfPosKeys = ResampleVec3(
			Data ? Data->TimesT : TArray<float>(),
			Data ? Data->Positions : TArray<FVector3f>(),
			FVector3f::ZeroVector);
		TArray<FQuat4f> GltfRotKeys = ResampleQuat(
			Data ? Data->TimesR : TArray<float>(),
			Data ? Data->Rotations : TArray<FQuat4f>(),
			FQuat4f::Identity);
		TArray<FVector3f> GltfScaleKeys = ResampleVec3(
			Data ? Data->TimesS : TArray<float>(),
			Data ? Data->Scales : TArray<FVector3f>(),
			FVector3f(1.0f, 1.0f, 1.0f));

		TArray<FVector3f> PosKeys;
		TArray<FQuat4f> RotKeys;
		TArray<FVector3f> ScaleKeys;
		PosKeys.SetNum(NumKeys);
		RotKeys.SetNum(NumKeys);
		ScaleKeys.SetNum(NumKeys);

		for (int32 KeyIndex = 0; KeyIndex < NumKeys; KeyIndex++)
		{
			FQuat KeyRotation = FQuat(GltfRotKeys[KeyIndex]);
			KeyRotation.Normalize();

			const FTransform GltfKeyTransform(
				KeyRotation,
				FVector(GltfPosKeys[KeyIndex]),
				FVector(GltfScaleKeys[KeyIndex]));
			const FTransform UeKeyTransformRaw = ConvertGltfLocalTransformToUeBasis(GltfKeyTransform);
			const FTransform UeKeyDeltaFromBind = UeKeyTransformRaw.GetRelativeTransform(GltfBindPose);
			FTransform UeKeyTransform = DefaultUePose * UeKeyDeltaFromBind;
			if (BoneIdx == 0)
			{
				UeKeyTransform = UeKeyTransform * GetImportedRootOrientationCorrection();
			}

			PosKeys[KeyIndex] = FVector3f(UeKeyTransform.GetTranslation());
			RotKeys[KeyIndex] = FQuat4f(UeKeyTransform.GetRotation());
			ScaleKeys[KeyIndex] = FVector3f(UeKeyTransform.GetScale3D());
		}

		if (!Data || Data->TimesT.Num() == 0)
		{
			for (int32 KeyIndex = 0; KeyIndex < NumKeys; KeyIndex++)
				PosKeys[KeyIndex] = FVector3f(DefaultUePose.GetTranslation());
		}

		if (!Data || Data->TimesR.Num() == 0)
		{
			for (int32 KeyIndex = 0; KeyIndex < NumKeys; KeyIndex++)
				RotKeys[KeyIndex] = FQuat4f(DefaultUePose.GetRotation());
		}

		if (!Data || Data->TimesS.Num() == 0)
		{
			for (int32 KeyIndex = 0; KeyIndex < NumKeys; KeyIndex++)
				ScaleKeys[KeyIndex] = FVector3f(DefaultUePose.GetScale3D());
		}

		Controller.AddBoneCurve(BoneName, false);
		Controller.SetBoneTrackKeys(BoneName, PosKeys, RotKeys, ScaleKeys, false);
	}

	Controller.NotifyPopulated();
	Controller.CloseBracket(false);

	// 9. Save
	FAssetRegistryModule::AssetCreated(AnimSeq);
	SavePackage(AnimSeq);

	return AnimSeq;
}

bool USekiroImportCommandlet::SavePackage(UObject* Asset)
{
	if (!Asset) return false;

	UPackage* Package = Asset->GetOutermost();
	if (!Package) return false;

	Package->SetDirtyFlag(true);
	Package->MarkPackageDirty();

	FString PackageFileName = FPackageName::LongPackageNameToFilename(
		Package->GetName(), FPackageName::GetAssetPackageExtension());
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(PackageFileName), true);

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = EObjectFlags::RF_Public | EObjectFlags::RF_Standalone;
	SaveArgs.Error = GWarn;

	const bool bSaved = UPackage::SavePackage(Package, Asset, *PackageFileName, SaveArgs);
	UE_LOG(LogTemp, Display, TEXT("    SavePackage %s -> %s (%s)"), *Package->GetName(), *PackageFileName, bSaved ? TEXT("success") : TEXT("failed"));
	return bSaved;
}
