#include "Import/SekiroImportCommandlet.h"
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
#include "InterchangeGenericAssetsPipeline.h"
#include "InterchangeGenericAssetsPipelineSharedSettings.h"
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
#include "EditorAssetLibrary.h"
#include "Import/SekiroAssetImporter.h"
#include "Import/SekiroHumanoidValidation.h"
#include "Import/SekiroMaterialSetup.h"
#include "Data/SekiroCharacterData.h"
#include "Data/SekiroSkillDataAsset.h"

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

	static bool DeleteAssetDirectoryIfPresent(const FString& DirectoryPath)
	{
		if (!UEditorAssetLibrary::DoesDirectoryExist(DirectoryPath))
		{
			return true;
		}

		const bool bDeleted = UEditorAssetLibrary::DeleteDirectory(DirectoryPath);
		UE_LOG(LogTemp, Display, TEXT("    DeleteDirectory %s -> %s"), *DirectoryPath, bDeleted ? TEXT("success") : TEXT("failed"));
		return bDeleted;
	}

	static bool TryGetPureSpineOrdinal(const FName& BoneName, int32& OutOrdinal)
	{
		const FString Name = BoneName.ToString();
		if (Name.Equals(TEXT("Spine"), ESearchCase::IgnoreCase))
		{
			OutOrdinal = 0;
			return true;
		}

		if (!Name.StartsWith(TEXT("Spine"), ESearchCase::IgnoreCase))
		{
			return false;
		}

		const FString Suffix = Name.Mid(5);
		if (Suffix.IsEmpty())
		{
			return false;
		}

		for (const TCHAR Ch : Suffix)
		{
			if (!FChar::IsDigit(Ch))
			{
				return false;
			}
		}

		OutOrdinal = FCString::Atoi(*Suffix);
		return true;
	}

	static int32 FindExactBoneIndex(const FReferenceSkeleton& RefSkeleton, const TCHAR* BoneName)
	{
		return RefSkeleton.FindBoneIndex(FName(BoneName));
	}

	static void AddBoneIfPresent(const FReferenceSkeleton& RefSkeleton, const TCHAR* BoneName, TArray<int32>& Chain)
	{
		const int32 BoneIndex = FindExactBoneIndex(RefSkeleton, BoneName);
		if (BoneIndex != INDEX_NONE && !Chain.Contains(BoneIndex))
		{
			Chain.Add(BoneIndex);
		}
	}

	static TArray<int32> BuildImportedTorsoChain(const FReferenceSkeleton& RefSkeleton)
	{
		TArray<TPair<int32, int32>> PureSpines;
		PureSpines.Reserve(8);

		for (int32 BoneIndex = 0; BoneIndex < RefSkeleton.GetNum(); ++BoneIndex)
		{
			int32 Ordinal = -1;
			if (TryGetPureSpineOrdinal(RefSkeleton.GetBoneName(BoneIndex), Ordinal))
			{
				PureSpines.Emplace(Ordinal, BoneIndex);
			}
		}

		PureSpines.Sort([](const TPair<int32, int32>& Left, const TPair<int32, int32>& Right)
		{
			return Left.Key < Right.Key;
		});

		TArray<int32> Chain;
		Chain.Reserve(PureSpines.Num() + 4);
		for (const TPair<int32, int32>& Entry : PureSpines)
		{
			Chain.Add(Entry.Value);
		}

		AddBoneIfPresent(RefSkeleton, TEXT("Chest"), Chain);
		AddBoneIfPresent(RefSkeleton, TEXT("UpperChest"), Chain);
		AddBoneIfPresent(RefSkeleton, TEXT("Neck"), Chain);
		AddBoneIfPresent(RefSkeleton, TEXT("Head"), Chain);
		return Chain;
	}

	static void ValidateImportedHumanoidTorsoChain(USkeleton* Skeleton, TArray<FString>& Errors)
	{
		if (!Skeleton)
		{
			Errors.Add(TEXT("imported skeleton missing after model import"));
			return;
		}

		const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();
		const int32 PelvisIndex = FindExactBoneIndex(RefSkeleton, TEXT("Pelvis"));
		if (PelvisIndex == INDEX_NONE)
		{
			Errors.Add(TEXT("imported skeleton is missing required bone 'Pelvis'"));
			return;
		}

		TArray<int32> Chain = BuildImportedTorsoChain(RefSkeleton);
		if (Chain.Num() < 5)
		{
			Errors.Add(FString::Printf(TEXT("imported skeleton torso chain is incomplete under '%s'"), *Skeleton->GetPathName()));
			return;
		}

		TArray<FString> ChainNames;
		ChainNames.Reserve(Chain.Num() + 1);
		ChainNames.Add(RefSkeleton.GetBoneName(PelvisIndex).ToString());
		for (const int32 BoneIndex : Chain)
		{
			ChainNames.Add(RefSkeleton.GetBoneName(BoneIndex).ToString());
		}
		UE_LOG(LogTemp, Display, TEXT("  Imported torso chain: %s"), *FString::Join(ChainNames, TEXT(" -> ")));

		int32 ExpectedParentIndex = PelvisIndex;
		for (const int32 BoneIndex : Chain)
		{
			const int32 ActualParentIndex = RefSkeleton.GetParentIndex(BoneIndex);
			if (ActualParentIndex != ExpectedParentIndex)
			{
				const FString BoneName = RefSkeleton.GetBoneName(BoneIndex).ToString();
				const FString ExpectedParentName = RefSkeleton.GetBoneName(ExpectedParentIndex).ToString();
				const FString ActualParentName = ActualParentIndex != INDEX_NONE
					? RefSkeleton.GetBoneName(ActualParentIndex).ToString()
					: TEXT("<none>");
				Errors.Add(FString::Printf(TEXT("imported skeleton torso hierarchy mismatch: '%s' must be parented directly under '%s' but was under '%s'"), *BoneName, *ExpectedParentName, *ActualParentName));
				return;
			}

			ExpectedParentIndex = BoneIndex;
		}
	}

	static bool LoadJsonObjectFromFile(const FString& FilePath, TSharedPtr<FJsonObject>& OutObject);

	static bool LoadJsonObjectFromFile(const FString& FilePath, TSharedPtr<FJsonObject>& OutObject)
	{
		FString JsonString;
		if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
		{
			return false;
		}

		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
		return FJsonSerializer::Deserialize(Reader, OutObject) && OutObject.IsValid();
	}

	static bool ResolveDeliverableFiles(const TSharedPtr<FJsonObject>& AssetPackage, const FString& ExportDir, const FString& DeliverableId, TArray<FString>& OutFiles)
	{
		OutFiles.Reset();
		if (!AssetPackage.IsValid())
		{
			return false;
		}

		const TSharedPtr<FJsonObject>* DeliverablesObject = nullptr;
		if (!AssetPackage->TryGetObjectField(TEXT("deliverables"), DeliverablesObject) || !DeliverablesObject || !DeliverablesObject->IsValid())
		{
			return false;
		}

		const TSharedPtr<FJsonObject>* DeliverableObject = nullptr;
		if (!(*DeliverablesObject)->TryGetObjectField(DeliverableId, DeliverableObject) || !DeliverableObject || !DeliverableObject->IsValid())
		{
			return false;
		}

		const FString DeliverableStatus = (*DeliverableObject)->GetStringField(TEXT("status"));
		if (DeliverableStatus != TEXT("ready") && DeliverableStatus != TEXT("ok"))
		{
			return false;
		}

		const TArray<TSharedPtr<FJsonValue>>* FilesArray = nullptr;
		if (!(*DeliverableObject)->TryGetArrayField(TEXT("files"), FilesArray) || !FilesArray)
		{
			return false;
		}

		for (const TSharedPtr<FJsonValue>& FileValue : *FilesArray)
		{
			const FString RelativePath = FileValue->AsString();
			OutFiles.Add(FPaths::ConvertRelativePathToFull(ExportDir / RelativePath));
		}
		return OutFiles.Num() > 0;
	}

	static bool ResolveStringArrayField(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName, TArray<FString>& OutValues)
	{
		OutValues.Reset();
		if (!JsonObject.IsValid())
		{
			return false;
		}

		const TArray<TSharedPtr<FJsonValue>>* ValuesArray = nullptr;
		if (!JsonObject->TryGetArrayField(FieldName, ValuesArray) || !ValuesArray)
		{
			return false;
		}

		for (const TSharedPtr<FJsonValue>& Value : *ValuesArray)
		{
			const FString StringValue = Value.IsValid() ? Value->AsString() : FString();
			if (!StringValue.IsEmpty())
			{
				OutValues.Add(StringValue);
			}
		}

		return OutValues.Num() > 0;
	}

	static USekiroCharacterData* LoadCharacterDataAsset(const FString& ChrContent, const FString& ChrId)
	{
		const FString AssetName = FString::Printf(TEXT("CHR_%s"), *ChrId);
		const FString ObjectPath = FString::Printf(TEXT("%s/%s.%s"), *ChrContent, *AssetName, *AssetName);
		return LoadObject<USekiroCharacterData>(nullptr, *ObjectPath);
	}

	static FString NormalizeMaterialSlotName(const FString& SlotName)
	{
		FString Normalized = SlotName;
		Normalized.ReplaceInline(TEXT(" "), TEXT("_"));
		Normalized.ReplaceInline(TEXT("|"), TEXT("_"));
		Normalized.ReplaceInline(TEXT("#"), TEXT("_"));
		return Normalized;
	}

	static void PopulateCharacterMaterialMapFromManifest(USekiroCharacterData* CharacterAsset, const FString& ChrContent, const FString& ManifestPath)
	{
		if (!CharacterAsset)
		{
			return;
		}

		TSharedPtr<FJsonObject> ManifestObject;
		if (!LoadJsonObjectFromFile(ManifestPath, ManifestObject))
		{
			UE_LOG(LogTemp, Warning, TEXT("  Materials: failed to read manifest for CharacterData binding: %s"), *ManifestPath);
			return;
		}

		const TArray<TSharedPtr<FJsonValue>>* MaterialsArray = nullptr;
		if (!ManifestObject->TryGetArrayField(TEXT("materials"), MaterialsArray) || !MaterialsArray)
		{
			return;
		}

		CharacterAsset->MaterialMap.Reset();
		for (const TSharedPtr<FJsonValue>& MaterialValue : *MaterialsArray)
		{
			const TSharedPtr<FJsonObject> MaterialObj = MaterialValue->AsObject();
			if (!MaterialObj.IsValid())
			{
				continue;
			}

			FString SlotName;
			if (!MaterialObj->TryGetStringField(TEXT("slotName"), SlotName) || SlotName.IsEmpty())
			{
				continue;
			}

			FString MaterialKey;
			if (!MaterialObj->TryGetStringField(TEXT("materialInstanceKey"), MaterialKey) || MaterialKey.IsEmpty())
			{
				MaterialObj->TryGetStringField(TEXT("name"), MaterialKey);
			}

			MaterialKey.ReplaceInline(TEXT(" "), TEXT("_"));
			MaterialKey.ReplaceInline(TEXT("|"), TEXT("_"));
			const FString MaterialAssetName = FString::Printf(TEXT("MI_%s"), *MaterialKey);
			const FString MaterialObjectPath = FString::Printf(TEXT("%s/Materials/%s.%s"), *ChrContent, *MaterialAssetName, *MaterialAssetName);
			if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, *MaterialObjectPath))
			{
				CharacterAsset->MaterialMap.Add(SlotName, TSoftObjectPtr<UMaterialInterface>(Material));
				CharacterAsset->MaterialMap.Add(NormalizeMaterialSlotName(SlotName), TSoftObjectPtr<UMaterialInterface>(Material));
			}
		}

		CharacterAsset->MarkPackageDirty();
	}

	static TArray<FString> ParseCsvPatterns(const FString& Value)
	{
		TArray<FString> Result;
		if (Value.IsEmpty())
		{
			return Result;
		}

		Value.ParseIntoArray(Result, TEXT(","), true);
		for (FString& Entry : Result)
		{
			Entry = Entry.TrimStartAndEnd();
		}

		Result.RemoveAll([](const FString& Entry)
		{
			return Entry.IsEmpty();
		});
		return Result;
	}

	static bool MatchesAnimationFilter(const FString& AnimationName, const TArray<FString>& AnimFilters)
	{
		if (AnimFilters.Num() == 0)
		{
			return true;
		}

		for (const FString& Filter : AnimFilters)
		{
			if (Filter.Contains(TEXT("*")) || Filter.Contains(TEXT("?")))
			{
				if (AnimationName.MatchesWildcard(Filter, ESearchCase::IgnoreCase))
				{
					return true;
				}
			}
			else if (AnimationName.Equals(Filter, ESearchCase::IgnoreCase)
				|| AnimationName.StartsWith(Filter, ESearchCase::IgnoreCase))
			{
				return true;
			}
		}

		return false;
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
	FString AnimFilterStr;
	FString ContentBase = TEXT("/Game/SekiroAssets/Characters");
	int32 AnimLimit = -1;
	bool bImportAnimationsOnly = false;
	bool bImportModelOnly = false;

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

	if (ParamsMap.Contains(TEXT("AnimFilter")))
		AnimFilterStr = ParamsMap[TEXT("AnimFilter")];

	if (ParamsMap.Contains(TEXT("AnimLimit")))
		AnimLimit = FCString::Atoi(*ParamsMap[TEXT("AnimLimit")]);

	FString ImportAnimationsOnlyValue;
	bImportAnimationsOnly = FParse::Param(*Params, TEXT("ImportAnimationsOnly"))
		|| FParse::Param(*Params, TEXT("AnimationsOnly"))
		|| (FParse::Value(*Params, TEXT("ImportAnimationsOnly="), ImportAnimationsOnlyValue)
			&& ImportAnimationsOnlyValue.Equals(TEXT("true"), ESearchCase::IgnoreCase))
		|| (ParamsMap.Contains(TEXT("ImportAnimationsOnly")) && ParamsMap[TEXT("ImportAnimationsOnly")].Equals(TEXT("true"), ESearchCase::IgnoreCase));
	bImportModelOnly = FParse::Param(*Params, TEXT("ImportModelOnly"))
		|| (ParamsMap.Contains(TEXT("ImportModelOnly")) && ParamsMap[TEXT("ImportModelOnly")].Equals(TEXT("true"), ESearchCase::IgnoreCase));

	UE_LOG(LogTemp, Display, TEXT("=== Sekiro Import Commandlet ==="));
	UE_LOG(LogTemp, Display, TEXT("ExportDir: %s"), *ExportDir);

	TArray<FString> ChrDirs;
	IFileManager::Get().FindFiles(ChrDirs, *(ExportDir / TEXT("*")), false, true);

	TArray<FString> ChrFilter;
	if (!ChrFilterStr.IsEmpty())
		ChrFilterStr.ParseIntoArray(ChrFilter, TEXT(","));
	const TArray<FString> AnimFilters = ParseCsvPatterns(AnimFilterStr);

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
		ImportCharacter(ChrId, ExportDir / ChrId, ContentBase, AnimLimit, AnimFilters, bImportAnimationsOnly, bImportModelOnly);

		if ((i + 1) % 5 == 0)
			CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	}

	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	UE_LOG(LogTemp, Display, TEXT("=== Import Complete ==="));
	return 0;
}

void USekiroImportCommandlet::ImportCharacter(const FString& ChrId, const FString& ExportDir, const FString& ContentBase, int32 AnimLimit, const TArray<FString>& AnimFilters, bool bImportAnimationsOnly, bool bImportModelOnly)
{
	FString ChrContent = ContentBase / ChrId;
	int32 TexCount = 0, AnimCount = 0;
	TArray<FString> Errors;
	bool bVisiblePoseValidated = false;
	USkeletalMesh* ImportedSkeletalMesh = nullptr;

	TSharedPtr<FJsonObject> AssetPackage;
	const FString AssetPackagePath = ExportDir / TEXT("asset_package.json");
	if (!LoadJsonObjectFromFile(AssetPackagePath, AssetPackage))
	{
		UE_LOG(LogTemp, Error, TEXT("  Asset package missing or unreadable: %s"), *AssetPackagePath);
		return;
	}

	TArray<FString> TextureFiles;
	TArray<FString> ModelFiles;
	TArray<FString> AnimationFiles;
	TArray<FString> ExpectedAnimationFiles;
	TArray<FString> MissingAnimationFiles;
	TArray<FString> MaterialManifestFiles;
	TArray<FString> SkillFiles;
	const bool bFormalOnly = AssetPackage->GetStringField(TEXT("deliveryMode")) == TEXT("formal-only");
	const bool bFormalSuccess = AssetPackage->GetBoolField(TEXT("formalSuccess"));
	const bool bHasTextures = ResolveDeliverableFiles(AssetPackage, ExportDir, TEXT("textures"), TextureFiles);
	const bool bHasModel = ResolveDeliverableFiles(AssetPackage, ExportDir, TEXT("model"), ModelFiles);
	const bool bHasAnimations = ResolveDeliverableFiles(AssetPackage, ExportDir, TEXT("animations"), AnimationFiles);
	ResolveStringArrayField(AssetPackage, TEXT("expectedAnimationFiles"), ExpectedAnimationFiles);
	ResolveStringArrayField(AssetPackage, TEXT("missingAnimationFiles"), MissingAnimationFiles);
	const bool bHasMaterialManifest = ResolveDeliverableFiles(AssetPackage, ExportDir, TEXT("materialManifest"), MaterialManifestFiles);
	const bool bHasSkills = ResolveDeliverableFiles(AssetPackage, ExportDir, TEXT("skills"), SkillFiles);

	if (!bFormalOnly)
	{
		UE_LOG(LogTemp, Error, TEXT("  Asset package is not a formal-only export: %s"), *AssetPackagePath);
		return;
	}

	if (!bFormalSuccess)
	{
		if (bImportModelOnly)
		{
			if (!(bHasTextures && bHasModel && bHasMaterialManifest))
			{
				UE_LOG(LogTemp, Error, TEXT("  Model-only import requires ready model, textures, and material manifest deliverables: %s"), *AssetPackagePath);
				return;
			}

			UE_LOG(LogTemp, Warning, TEXT("  Asset package formalSuccess=false; proceeding with model-only import because required model deliverables are present."));
		}
		else if (bImportAnimationsOnly)
		{
			if (!bHasAnimations)
			{
				UE_LOG(LogTemp, Error, TEXT("  Animations-only import requires ready animation deliverables: %s"), *AssetPackagePath);
				return;
			}

			UE_LOG(LogTemp, Warning, TEXT("  Asset package formalSuccess=false; proceeding with animations-only import because animation deliverables are present."));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("  Asset package is not a successful formal-only export: %s"), *AssetPackagePath);
			return;
		}
	}

	if (AnimFilters.Num() > 0)
	{
		const int32 OriginalAnimationCount = AnimationFiles.Num();
		const int32 OriginalExpectedAnimationCount = ExpectedAnimationFiles.Num();
		const int32 OriginalMissingAnimationCount = MissingAnimationFiles.Num();
		AnimationFiles = AnimationFiles.FilterByPredicate([&AnimFilters](const FString& AnimFile)
		{
			return MatchesAnimationFilter(FPaths::GetBaseFilename(AnimFile), AnimFilters);
		});
		ExpectedAnimationFiles = ExpectedAnimationFiles.FilterByPredicate([&AnimFilters](const FString& AnimFile)
		{
			return MatchesAnimationFilter(FPaths::GetBaseFilename(AnimFile), AnimFilters);
		});
		MissingAnimationFiles = MissingAnimationFiles.FilterByPredicate([&AnimFilters](const FString& AnimFile)
		{
			return MatchesAnimationFilter(FPaths::GetBaseFilename(AnimFile), AnimFilters);
		});
		UE_LOG(LogTemp, Display, TEXT("  AnimFilter: %d -> %d clips"), OriginalAnimationCount, AnimationFiles.Num());
		if (OriginalExpectedAnimationCount > 0)
		{
			UE_LOG(LogTemp, Display, TEXT("  ExpectedAnimFilter: %d -> %d clips"), OriginalExpectedAnimationCount, ExpectedAnimationFiles.Num());
		}
		if (OriginalMissingAnimationCount > 0)
		{
			UE_LOG(LogTemp, Display, TEXT("  MissingAnimFilter: %d -> %d clips"), OriginalMissingAnimationCount, MissingAnimationFiles.Num());
		}
	}

	if (AnimLimit > 0 && AnimationFiles.Num() > AnimLimit)
	{
		AnimationFiles.SetNum(AnimLimit);
		UE_LOG(LogTemp, Display, TEXT("  AnimLimit=%d: truncating to first %d animations"), AnimLimit, AnimLimit);
	}
	if (AnimLimit > 0 && ExpectedAnimationFiles.Num() > AnimLimit)
	{
		ExpectedAnimationFiles.SetNum(AnimLimit);
	}
	if (AnimLimit > 0 && MissingAnimationFiles.Num() > AnimLimit)
	{
		MissingAnimationFiles.SetNum(AnimLimit);
	}

	const int32 ExpectedAnimationCount = ExpectedAnimationFiles.Num() > 0 ? ExpectedAnimationFiles.Num() : AnimationFiles.Num();
	if (!bImportModelOnly && MissingAnimationFiles.Num() > 0)
	{
		for (const FString& MissingAnimationFile : MissingAnimationFiles)
		{
			Errors.Add(FString::Printf(TEXT("formal export missing expected animation deliverable: %s"), *FPaths::GetCleanFilename(MissingAnimationFile)));
		}
	}

	// 1. Import textures (PNG only)
	if (!bImportAnimationsOnly)
	{
		for (const FString& TextureFile : TextureFiles)
		{
			UObject* Result = ImportTexture(TextureFile, ChrContent / TEXT("Textures"));
			if (Result)
			{
				TexCount++;
			}
			else
			{
				Errors.Add(FString::Printf(TEXT("texture import failed: %s"), *FPaths::GetCleanFilename(TextureFile)));
			}
		}

		if (bHasTextures)
			UE_LOG(LogTemp, Display, TEXT("  Textures: %d"), TexCount);
	}

	// 2. Import model from formal asset package
	FString ModelFile = bHasModel ? ModelFiles[0] : TEXT("");
	const FString MeshContentPath = ChrContent / TEXT("Mesh");

	USkeleton* Skeleton = FindSkeletonInPackage(MeshContentPath);
	if (!bImportModelOnly && Skeleton)
	{
		UE_LOG(LogTemp, Display, TEXT("  Existing skeleton: %s"), *Skeleton->GetPathName());
	}

	if (!ModelFile.IsEmpty() && (!bImportAnimationsOnly || !Skeleton))
	{
		FString Extension = FPaths::GetExtension(ModelFile).ToLower();
		const FString ImportedModelRootPath = MeshContentPath / FPaths::GetBaseFilename(ModelFile);
		UE_LOG(LogTemp, Display, TEXT("  Importing model: %s (format: %s)"), *FPaths::GetCleanFilename(ModelFile), *Extension);

		if (!DeleteAssetDirectoryIfPresent(ImportedModelRootPath))
		{
			Errors.Add(FString::Printf(TEXT("failed to clear existing mesh asset directory: %s"), *ImportedModelRootPath));
		}

		UObject* Result = nullptr;

		if (Extension == TEXT("gltf") || Extension == TEXT("glb"))
		{
			Result = ImportViaAssetTask(ModelFile, MeshContentPath, nullptr, true);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("  Model: rejected non-glTF format '%s' - only .gltf/.glb accepted"), *Extension);
			Errors.Add(FString::Printf(TEXT("model format not accepted (only gltf/glb): %s"), *FPaths::GetCleanFilename(ModelFile)));
		}

		if (Result)
		{
			USkeletalMesh* SkelMesh = Cast<USkeletalMesh>(Result);
			if (SkelMesh)
			{
				UE_LOG(LogTemp, Display, TEXT("  Model: OK (SkeletalMesh)"));
				ImportedSkeletalMesh = SkelMesh;
				Skeleton = SkelMesh->GetSkeleton();
				SavePackage(SkelMesh);
				if (Skeleton)
					SavePackage(Skeleton);
				if (Skeleton)
					UE_LOG(LogTemp, Display, TEXT("  Skeleton: %s"), *Skeleton->GetPathName());
				ValidateImportedHumanoidTorsoChain(Skeleton, Errors);
				const SekiroHumanoidValidation::FVisiblePoseValidationResult PoseValidation =
					SekiroHumanoidValidation::ValidateImportedVisibleHumanoidPose(Skeleton, SkelMesh);
				bVisiblePoseValidated = PoseValidation.bValidated;
				Errors.Append(PoseValidation.Errors);
			}
			else
			{
				UE_LOG(LogTemp, Display, TEXT("  Model: OK (%s) - checking for skeleton..."),
					*Result->GetClass()->GetName());

				USkeletalMesh* ImportedSkelMesh = FindSkeletalMeshInPackage(MeshContentPath);
				ImportedSkeletalMesh = ImportedSkelMesh;
				Skeleton = FindSkeletonInPackage(MeshContentPath);
				if (ImportedSkelMesh)
				{
					SavePackage(ImportedSkelMesh);
				}
				if (Skeleton)
				{
					SavePackage(Skeleton);
				}
				if (Skeleton)
					UE_LOG(LogTemp, Display, TEXT("  Skeleton found: %s"), *Skeleton->GetPathName());
				ValidateImportedHumanoidTorsoChain(Skeleton, Errors);
				const SekiroHumanoidValidation::FVisiblePoseValidationResult PoseValidation =
					SekiroHumanoidValidation::ValidateImportedVisibleHumanoidPose(Skeleton, ImportedSkelMesh);
				bVisiblePoseValidated = PoseValidation.bValidated;
				Errors.Append(PoseValidation.Errors);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("  Model: FAILED (%s)"), *FPaths::GetCleanFilename(ModelFile));
			Errors.Add(FString::Printf(TEXT("model import failed: %s"), *FPaths::GetCleanFilename(ModelFile)));
		}
	}
	else if (!bImportAnimationsOnly)
	{
		Errors.Add(TEXT("missing model deliverable"));
	}

	// 3. Import animations via Interchange (same pipeline as model)
	int32 RootMotionEnabledCount = 0;
	TArray<FString> RootMotionFailedClips;
	if (!bImportModelOnly && Skeleton)
	{
		for (const FString& AnimFile : AnimationFiles)
		{
			FString Ext = FPaths::GetExtension(AnimFile).ToLower();

			if (Ext != TEXT("gltf") && Ext != TEXT("glb"))
			{
				UE_LOG(LogTemp, Error, TEXT("  Animation: rejected non-glTF format '%s'"), *Ext);
				Errors.Add(FString::Printf(TEXT("animation format not accepted: %s"), *FPaths::GetCleanFilename(AnimFile)));
				continue;
			}

			const FString AnimName = FPaths::GetBaseFilename(AnimFile);
			const FString AnimDest = ChrContent / TEXT("Animations");

			UObject* Result = ImportViaAssetTask(AnimFile, AnimDest, Skeleton);

			if (Result)
			{
				AnimCount++;

				// Enable root-motion on the imported animation
				if (UAnimSequence* AnimSeq = Cast<UAnimSequence>(Result))
				{
					AnimSeq->bEnableRootMotion = true;
					AnimSeq->bForceRootLock = false;
					AnimSeq->MarkPackageDirty();
					SavePackage(AnimSeq);
					RootMotionEnabledCount++;
				}
				else
				{
					RootMotionFailedClips.Add(AnimName);
				}
			}
			else
			{
				Errors.Add(FString::Printf(TEXT("animation import failed: %s"), *FPaths::GetCleanFilename(AnimFile)));
			}
		}

		if (bHasAnimations)
			UE_LOG(LogTemp, Display, TEXT("  Animations: %d (root-motion enabled: %d)"), AnimCount, RootMotionEnabledCount);

		if (bHasAnimations && AnimCount != ExpectedAnimationCount)
		{
			Errors.Add(FString::Printf(TEXT("animation coverage mismatch: imported %d of %d expected clips"), AnimCount, ExpectedAnimationCount));
		}

		if (RootMotionFailedClips.Num() > 0)
		{
			for (const FString& Clip : RootMotionFailedClips)
			{
				Errors.Add(FString::Printf(TEXT("root-motion enablement failed for clip: %s"), *Clip));
			}
		}

		if (AnimCount > 30)
			CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	}
	else if (!bImportModelOnly)
	{
		UE_LOG(LogTemp, Warning, TEXT("  Animations: SKIPPED (no skeleton)"));
		Errors.Add(TEXT("animations skipped because no skeleton was available"));
	}

	// 4. Create material instances and bind CharacterData slots from the formal manifest
	if (!bImportAnimationsOnly && bHasMaterialManifest)
	{
		TArray<UMaterialInstanceConstant*> MaterialInstances;
		TArray<FString> MaterialErrors;
		USekiroMaterialSetup::SetupMaterialsFromManifestStrict(
			MaterialManifestFiles[0],
			ExportDir / TEXT("Textures"),
			ChrContent,
			MaterialInstances,
			MaterialErrors);
		UE_LOG(LogTemp, Display, TEXT("  Materials: %d"), MaterialInstances.Num());
		Errors.Append(MaterialErrors);

		// Bind MI_* to SkeletalMesh slots
		if (Skeleton)
		{
			FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
			IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
			TArray<FAssetData> MeshAssets;
			AssetRegistry.GetAssetsByPath(FName(*(ChrContent / TEXT("Mesh"))), MeshAssets, true);
			for (const FAssetData& AssetData : MeshAssets)
			{
				if (USkeletalMesh* SkelMesh = Cast<USkeletalMesh>(AssetData.GetAsset()))
				{
					TArray<FString> BindErrors;
					USekiroMaterialSetup::BindMaterialsToSkeletalMeshStrict(SkelMesh, MaterialManifestFiles[0], ChrContent, BindErrors);
					Errors.Append(BindErrors);
					SavePackage(SkelMesh);
					break;
				}
			}
		}
	}
	else if (!bImportAnimationsOnly)
	{
		Errors.Add(TEXT("missing material manifest deliverable"));
	}

	// 5. Import skill configs
	if (!bImportAnimationsOnly && !bImportModelOnly && bHasSkills)
	{
		const FString SkillSrc = SkillFiles[0];
		FString SkillDst = FPaths::ProjectContentDir() / TEXT("SekiroAssets/Characters") / ChrId / TEXT("Skills/skill_config.json");
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(SkillDst), true);
		IFileManager::Get().Copy(*SkillDst, *SkillSrc, true);
		TArray<USekiroSkillDataAsset*> ImportedSkillAssets = USekiroAssetImporter::ImportSkillConfig(SkillSrc, ChrContent / TEXT("Skills"));
		UE_LOG(LogTemp, Display, TEXT("  Skills: imported %d skill assets"), ImportedSkillAssets.Num());
		for (USekiroSkillDataAsset* SkillAsset : ImportedSkillAssets)
		{
			if (!SkillAsset)
			{
				Errors.Add(TEXT("skill import returned a null skill asset"));
				continue;
			}

			if (SkillAsset->Animation.IsNull())
			{
				Errors.Add(FString::Printf(TEXT("skill asset '%s' is missing imported animation '%s'"), *SkillAsset->GetName(), *SkillAsset->SourceFileName));
			}
		}

		if (USekiroCharacterData* CharacterData = LoadCharacterDataAsset(ChrContent, ChrId))
		{
			if (bHasMaterialManifest)
			{
				PopulateCharacterMaterialMapFromManifest(CharacterData, ChrContent, MaterialManifestFiles[0]);
				SavePackage(CharacterData);
			}
		}
		else
		{
			Errors.Add(TEXT("failed to load CharacterData asset after skill import"));
		}
	}
	else if (!bImportAnimationsOnly && !bImportModelOnly)
	{
		Errors.Add(TEXT("missing skills deliverable"));
	}

	TSharedPtr<FJsonObject> ReportObject = MakeShared<FJsonObject>();
	ReportObject->SetStringField(TEXT("schemaVersion"), TEXT("2.0"));
	ReportObject->SetStringField(TEXT("characterId"), ChrId);
	ReportObject->SetStringField(TEXT("deliveryMode"), TEXT("formal-only"));
	ReportObject->SetBoolField(TEXT("modelImported"), !ModelFile.IsEmpty() && Skeleton != nullptr);
	ReportObject->SetBoolField(TEXT("skeletonImported"), Skeleton != nullptr);
	ReportObject->SetBoolField(TEXT("visiblePoseValidated"), bVisiblePoseValidated);
	ReportObject->SetNumberField(TEXT("textureCount"), TexCount);
	ReportObject->SetNumberField(TEXT("expectedAnimationCount"), ExpectedAnimationCount);
	ReportObject->SetNumberField(TEXT("animationCount"), AnimCount);
	ReportObject->SetBoolField(TEXT("hasPhysicsAsset"), ImportedSkeletalMesh != nullptr && ImportedSkeletalMesh->GetPhysicsAsset() != nullptr);

	// Resource relationship tracking
	TSharedPtr<FJsonObject> RelationshipsObject = MakeShared<FJsonObject>();
	RelationshipsObject->SetBoolField(TEXT("skeletalMeshBound"), ImportedSkeletalMesh != nullptr);
	RelationshipsObject->SetBoolField(TEXT("skeletonBound"), Skeleton != nullptr);
	RelationshipsObject->SetBoolField(TEXT("materialManifestApplied"), !bImportAnimationsOnly && bHasMaterialManifest);
	RelationshipsObject->SetBoolField(TEXT("skillConfigImported"), !bImportAnimationsOnly && !bImportModelOnly && bHasSkills);
	RelationshipsObject->SetBoolField(TEXT("animationsCoverageComplete"), AnimCount == ExpectedAnimationCount);
	ReportObject->SetObjectField(TEXT("relationships"), RelationshipsObject);

	// Root-motion binding results
	TSharedPtr<FJsonObject> RootMotionObject = MakeShared<FJsonObject>();
	RootMotionObject->SetStringField(TEXT("source"), TEXT("formal-export-summary"));
	RootMotionObject->SetBoolField(TEXT("consumedFromFormalSummary"), !bImportModelOnly && bHasAnimations);
	RootMotionObject->SetNumberField(TEXT("enabledCount"), RootMotionEnabledCount);
	RootMotionObject->SetNumberField(TEXT("totalAnimations"), AnimCount);
	TArray<TSharedPtr<FJsonValue>> RootMotionFailedValues;
	for (const FString& Clip : RootMotionFailedClips)
	{
		RootMotionFailedValues.Add(MakeShared<FJsonValueString>(Clip));
	}
	RootMotionObject->SetArrayField(TEXT("failedClips"), RootMotionFailedValues);
	ReportObject->SetObjectField(TEXT("rootMotion"), RootMotionObject);

	// Missing animation files from formal export
	TArray<TSharedPtr<FJsonValue>> MissingAnimValues;
	for (const FString& MissingAnimFile : MissingAnimationFiles)
	{
		MissingAnimValues.Add(MakeShared<FJsonValueString>(MissingAnimFile));
	}
	ReportObject->SetArrayField(TEXT("missingAnimationFiles"), MissingAnimValues);

	TArray<TSharedPtr<FJsonValue>> ErrorValues;
	for (const FString& Error : Errors)
	{
		ErrorValues.Add(MakeShared<FJsonValueString>(Error));
	}
	ReportObject->SetArrayField(TEXT("errors"), ErrorValues);
	ReportObject->SetBoolField(TEXT("success"), Errors.Num() == 0);
	FString ReportText;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ReportText);
	FJsonSerializer::Serialize(ReportObject.ToSharedRef(), Writer);
	FFileHelper::SaveStringToFile(ReportText, *(ExportDir / TEXT("ue_import_report.json")));
}

UObject* USekiroImportCommandlet::ImportViaAssetTask(const FString& FilePath, const FString& DestPackagePath, USkeleton* SkeletonOverride, bool bSaveAssetsInDestinationPath)
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

	if (SkeletonOverride)
	{
		UInterchangeGenericAssetsPipeline* Pipeline = NewObject<UInterchangeGenericAssetsPipeline>(GetTransientPackage());
		if (Pipeline->CommonSkeletalMeshesAndAnimationsProperties)
		{
			Pipeline->CommonSkeletalMeshesAndAnimationsProperties->Skeleton = SkeletonOverride;
			Pipeline->CommonSkeletalMeshesAndAnimationsProperties->bImportOnlyAnimations = true;
		}
		ImportParams.OverridePipelines.Add(FSoftObjectPath(Pipeline));
	}

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

	// Model import creates related assets outside the returned object list.
	// Animation import does not need a recursive resave of the entire folder, which
	// causes repeated rewrites of every prior sequence and eventually file-lock failures.
	if (bSaveAssetsInDestinationPath)
	{
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
	}

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

USkeletalMesh* USekiroImportCommandlet::FindSkeletalMeshInPackage(const FString& PackagePath)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> Assets;
	AssetRegistry.GetAssetsByPath(FName(*PackagePath), Assets, true);

	for (const FAssetData& Asset : Assets)
	{
		if (USkeletalMesh* SkelMesh = Cast<USkeletalMesh>(Asset.GetAsset()))
		{
			return SkelMesh;
		}
	}

	return nullptr;
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
