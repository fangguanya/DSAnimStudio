#include "Import/SekiroValidationCommandlet.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Engine/Texture2D.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Materials/MaterialInstance.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"

USekiroValidationCommandlet::USekiroValidationCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 USekiroValidationCommandlet::FCharacterValidation::GetErrorCount() const
{
	int32 Errors = 0;
	if (!bHasSkeleton) Errors++;
	if (!bHasSkeletalMesh) Errors++;
	if (bHasSkeletalMesh && !bHasPhysicsAsset) Errors++;
	if (BoneCount == 0 && bHasSkeleton) Errors++;
	Errors += AnimsWithZeroDuration;
	Errors += AnimsWithZeroTracks;
	Errors += AnimsWithWrongSkeleton;
	Errors += SkillSchemaErrors;
	// TexturesWithZeroSize excluded: GetSizeX() returns 0 in commandlet/headless mode (no GPU)
	return Errors;
}

int32 USekiroValidationCommandlet::FCharacterValidation::GetWarningCount() const
{
	int32 Warnings = 0;
	if (AnimSequenceCount == 0 && bHasSkeleton) Warnings++;
	if (!bHasSkillConfig) Warnings++;
	// Skill animation cross-reference mismatches are info-only, not warnings.
	// skill_config.json references ALL behavior variation animations (a000-a400 pools)
	// but only the character's own ANIBND animations are exported per-character.
	return Warnings;
}

int32 USekiroValidationCommandlet::Main(const FString& Params)
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

	UE_LOG(LogTemp, Display, TEXT(""));
	UE_LOG(LogTemp, Display, TEXT("=============================================="));
	UE_LOG(LogTemp, Display, TEXT("   Sekiro Asset Validation Suite"));
	UE_LOG(LogTemp, Display, TEXT("=============================================="));
	UE_LOG(LogTemp, Display, TEXT("ExportDir: %s"), *ExportDir);
	UE_LOG(LogTemp, Display, TEXT("ContentBase: %s"), *ContentBase);

	// Discover characters from UE5 content
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	AssetRegistry.SearchAllAssets(true);

	// Find character directories from export dir
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

	UE_LOG(LogTemp, Display, TEXT("Characters to validate: %d"), ValidChrs.Num());
	UE_LOG(LogTemp, Display, TEXT(""));

	// Aggregate stats
	int32 TotalChrs = 0;
	int32 TotalSkeletons = 0;
	int32 TotalMeshes = 0;
	int32 TotalStaticMeshes = 0;
	int32 TotalAnims = 0;
	int32 TotalTextures = 0;
	int32 TotalMaterials = 0;
	int32 TotalSkillConfigs = 0;
	int32 TotalSkillEvents = 0;
	int32 TotalErrors = 0;
	int32 TotalWarnings = 0;
	int32 TotalBones = 0;
	float TotalAnimDuration = 0.0f;
	int32 TotalSkillAnimsMatched = 0;
	int32 TotalSkillAnimsUnmatched = 0;
	int32 TotalSkillAnimsSharedPool = 0;

	TArray<FString> ErrorMessages;
	TArray<FString> WarningMessages;
	TArray<TSharedPtr<FJsonValue>> CharacterReports;

	for (int32 i = 0; i < ValidChrs.Num(); i++)
	{
		const FString& ChrId = ValidChrs[i];
		TotalChrs++;

		FCharacterValidation V = ValidateCharacter(ChrId, ContentBase, ExportDir);
		TSharedPtr<FJsonObject> CharacterReport = MakeShared<FJsonObject>();
		CharacterReport->SetStringField(TEXT("characterId"), ChrId);
		CharacterReport->SetBoolField(TEXT("hasSkeleton"), V.bHasSkeleton);
		CharacterReport->SetBoolField(TEXT("hasSkeletalMesh"), V.bHasSkeletalMesh);
		CharacterReport->SetBoolField(TEXT("hasPhysicsAsset"), V.bHasPhysicsAsset);
		CharacterReport->SetNumberField(TEXT("boneCount"), V.BoneCount);
		CharacterReport->SetNumberField(TEXT("animationCount"), V.AnimSequenceCount);
		CharacterReport->SetNumberField(TEXT("textureCount"), V.TextureCount);
		CharacterReport->SetNumberField(TEXT("materialCount"), V.MaterialCount);
		CharacterReport->SetBoolField(TEXT("hasSkillConfig"), V.bHasSkillConfig);
		CharacterReport->SetNumberField(TEXT("skillEventCount"), V.SkillEventCount);
		CharacterReport->SetNumberField(TEXT("errorCount"), V.GetErrorCount());
		CharacterReport->SetNumberField(TEXT("warningCount"), V.GetWarningCount());
		CharacterReports.Add(MakeShared<FJsonValueObject>(CharacterReport));

		// Log per-character summary
		UE_LOG(LogTemp, Display, TEXT("[%d/%d] %s: Skel=%s Mesh=%s Bones=%d Anims=%d Tex=%d Mat=%d Skills=%s(%d events)"),
			i + 1, ValidChrs.Num(), *ChrId,
			V.bHasSkeleton ? TEXT("OK") : TEXT("MISS"),
			V.bHasSkeletalMesh ? TEXT("OK") : (V.bHasStaticMesh ? TEXT("STATIC") : TEXT("MISS")),
			V.BoneCount,
			V.AnimSequenceCount,
			V.TextureCount,
			V.MaterialCount,
			V.bHasSkillConfig ? TEXT("OK") : TEXT("MISS"),
			V.SkillEventCount);

		// Aggregate
		if (V.bHasSkeleton) TotalSkeletons++;
		if (V.bHasSkeletalMesh) TotalMeshes++;
		if (V.bHasStaticMesh && !V.bHasSkeletalMesh) TotalStaticMeshes++;
		TotalAnims += V.AnimSequenceCount;
		TotalTextures += V.TextureCount;
		TotalMaterials += V.MaterialCount;
		if (V.bHasSkillConfig) TotalSkillConfigs++;
		TotalSkillEvents += V.SkillEventCount;
		TotalBones += V.BoneCount;
		TotalAnimDuration += V.TotalAnimDuration;
		TotalSkillAnimsMatched += V.SkillAnimsWithMatchingAsset;
		TotalSkillAnimsUnmatched += V.SkillAnimsWithoutMatchingAsset;
		TotalSkillAnimsSharedPool += V.SkillAnimsSharedPool;

		// Only flag missing Skeleton/Mesh as errors if the export has a model file.
		// Characters without exported models (partsbnd, cutscene, env-only) are expected to have no mesh.
		FString ModelDir = ExportDir / ChrId / TEXT("Model");
		bool bHasExportedModel = IFileManager::Get().DirectoryExists(*ModelDir);
		if (bHasExportedModel)
		{
			// Check for actual model files (gltf/glb/fbx)
			TArray<FString> ModelFiles;
			IFileManager::Get().FindFiles(ModelFiles, *(ModelDir / TEXT("*.gltf")), true, false);
			if (ModelFiles.Num() == 0)
				IFileManager::Get().FindFiles(ModelFiles, *(ModelDir / TEXT("*.glb")), true, false);
			if (ModelFiles.Num() == 0)
				IFileManager::Get().FindFiles(ModelFiles, *(ModelDir / TEXT("*.fbx")), true, false);
			if (ModelFiles.Num() == 0)
				bHasExportedModel = false;
		}

		// Collect errors
		// StaticMesh downgrade is not acceptable in the formal import path.
		if (!V.bHasSkeleton && bHasExportedModel && !V.bHasStaticMesh)
			ErrorMessages.Add(FString::Printf(TEXT("  [ERROR] %s: Missing Skeleton (model exported but not imported)"), *ChrId));
		if (!V.bHasSkeletalMesh && bHasExportedModel && !V.bHasStaticMesh)
			ErrorMessages.Add(FString::Printf(TEXT("  [ERROR] %s: Missing SkeletalMesh (model exported but not imported)"), *ChrId));
		if (V.bHasStaticMesh && !V.bHasSkeletalMesh && bHasExportedModel)
			ErrorMessages.Add(FString::Printf(TEXT("  [ERROR] %s: Imported as StaticMesh instead of SkeletalMesh"), *ChrId));
		if (V.bHasSkeletalMesh && !V.bHasPhysicsAsset)
			ErrorMessages.Add(FString::Printf(TEXT("  [ERROR] %s: SkeletalMesh has no PhysicsAsset"), *ChrId));
		if (V.BoneCount == 0 && V.bHasSkeleton)
			ErrorMessages.Add(FString::Printf(TEXT("  [ERROR] %s: Skeleton has 0 bones"), *ChrId));
		if (V.AnimsWithZeroDuration > 0)
			ErrorMessages.Add(FString::Printf(TEXT("  [ERROR] %s: %d animations with zero duration"), *ChrId, V.AnimsWithZeroDuration));
		if (V.AnimsWithZeroTracks > 0)
			ErrorMessages.Add(FString::Printf(TEXT("  [ERROR] %s: %d animations with zero bone tracks"), *ChrId, V.AnimsWithZeroTracks));
		if (V.AnimsWithWrongSkeleton > 0)
			ErrorMessages.Add(FString::Printf(TEXT("  [ERROR] %s: %d animations reference the wrong or missing skeleton"), *ChrId, V.AnimsWithWrongSkeleton));
		if (V.SkillSchemaErrors > 0)
			ErrorMessages.Add(FString::Printf(TEXT("  [ERROR] %s: skill_config.json has %d canonical schema violations"), *ChrId, V.SkillSchemaErrors));
		// TexturesWithZeroSize not reported — false positive in headless mode

		if (V.AnimSequenceCount == 0 && V.bHasSkeleton)
			WarningMessages.Add(FString::Printf(TEXT("  [WARN]  %s: Has skeleton but 0 animations"), *ChrId));
		if (!V.bHasSkillConfig)
			WarningMessages.Add(FString::Printf(TEXT("  [WARN]  %s: No skill_config.json"), *ChrId));
		// Skill animation cross-reference mismatches are info-only (shared behavior pools)

		// Count errors: exported models must yield skeletal assets, not static fallbacks.
		int32 ChrErrors = V.AnimsWithZeroDuration + V.AnimsWithZeroTracks;
		ChrErrors += V.AnimsWithWrongSkeleton;
		ChrErrors += V.SkillSchemaErrors;
		if (V.bHasSkeletalMesh && !V.bHasPhysicsAsset)
			ChrErrors++;
		if (bHasExportedModel)
		{
			if (V.bHasStaticMesh && !V.bHasSkeletalMesh)
			{
				ChrErrors++;
			}
			else
			{
				if (!V.bHasSkeleton) ChrErrors++;
				if (!V.bHasSkeletalMesh) ChrErrors++;
				if (V.BoneCount == 0 && V.bHasSkeleton) ChrErrors++;
			}
		}
		TotalErrors += ChrErrors;
		TotalWarnings += V.GetWarningCount();

		// GC every 10 characters
		if ((i + 1) % 10 == 0)
			CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	}

	// Print final report
	UE_LOG(LogTemp, Display, TEXT(""));
	UE_LOG(LogTemp, Display, TEXT("=============================================="));
	UE_LOG(LogTemp, Display, TEXT("   VALIDATION REPORT"));
	UE_LOG(LogTemp, Display, TEXT("=============================================="));
	UE_LOG(LogTemp, Display, TEXT(""));
	UE_LOG(LogTemp, Display, TEXT("--- Asset Counts ---"));
	UE_LOG(LogTemp, Display, TEXT("  Characters:      %d"), TotalChrs);
	UE_LOG(LogTemp, Display, TEXT("  SkeletalMesh:    %d"), TotalMeshes);
	UE_LOG(LogTemp, Display, TEXT("  StaticMesh:      %d (no bone weights)"), TotalStaticMeshes);
	UE_LOG(LogTemp, Display, TEXT("  Skeletons:       %d"), TotalSkeletons);
	UE_LOG(LogTemp, Display, TEXT("  Total Bones:     %d (avg %.0f per character)"), TotalBones, TotalSkeletons > 0 ? (float)TotalBones / TotalSkeletons : 0.f);
	UE_LOG(LogTemp, Display, TEXT("  AnimSequences:   %d (total duration: %.1f min)"), TotalAnims, TotalAnimDuration / 60.0f);
	UE_LOG(LogTemp, Display, TEXT("  Textures:        %d"), TotalTextures);
	UE_LOG(LogTemp, Display, TEXT("  Materials:       %d"), TotalMaterials);
	UE_LOG(LogTemp, Display, TEXT("  Skill Configs:   %d"), TotalSkillConfigs);
	UE_LOG(LogTemp, Display, TEXT("  Skill Events:    %d"), TotalSkillEvents);
	UE_LOG(LogTemp, Display, TEXT(""));
	UE_LOG(LogTemp, Display, TEXT("--- Skill-Animation Cross-Check ---"));
	UE_LOG(LogTemp, Display, TEXT("  Skill anims with matching AnimSequence: %d"), TotalSkillAnimsMatched);
	UE_LOG(LogTemp, Display, TEXT("  Skill anims from shared pool (a000_*):  %d (expected unmatched)"), TotalSkillAnimsSharedPool);
	UE_LOG(LogTemp, Display, TEXT("  Character-specific unmatched:           %d"), TotalSkillAnimsUnmatched - TotalSkillAnimsSharedPool);
	UE_LOG(LogTemp, Display, TEXT(""));

	// Errors
	if (ErrorMessages.Num() > 0)
	{
		UE_LOG(LogTemp, Display, TEXT("--- ERRORS (%d) ---"), ErrorMessages.Num());
		for (const FString& Msg : ErrorMessages)
			UE_LOG(LogTemp, Error, TEXT("%s"), *Msg);
		UE_LOG(LogTemp, Display, TEXT(""));
	}

	// Warnings
	if (WarningMessages.Num() > 0)
	{
		UE_LOG(LogTemp, Display, TEXT("--- WARNINGS (%d) ---"), WarningMessages.Num());
		for (const FString& Msg : WarningMessages)
			UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
		UE_LOG(LogTemp, Display, TEXT(""));
	}

	// Final verdict
	UE_LOG(LogTemp, Display, TEXT("=============================================="));
	if (TotalErrors == 0)
	{
		UE_LOG(LogTemp, Display, TEXT("  RESULT: PASSED (%d warnings)"), TotalWarnings);
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("  RESULT: FAILED (%d errors, %d warnings)"), TotalErrors, TotalWarnings);
	}
	UE_LOG(LogTemp, Display, TEXT("=============================================="));

	TSharedPtr<FJsonObject> ReportObject = MakeShared<FJsonObject>();
	ReportObject->SetStringField(TEXT("schemaVersion"), TEXT("1.0"));
	ReportObject->SetStringField(TEXT("contentBase"), ContentBase);
	ReportObject->SetStringField(TEXT("exportDir"), ExportDir);
	ReportObject->SetArrayField(TEXT("characters"), CharacterReports);
	ReportObject->SetNumberField(TEXT("totalErrors"), TotalErrors);
	ReportObject->SetNumberField(TEXT("totalWarnings"), TotalWarnings);
	ReportObject->SetBoolField(TEXT("success"), TotalErrors == 0);
	FString ReportText;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ReportText);
	FJsonSerializer::Serialize(ReportObject.ToSharedRef(), Writer);
	FFileHelper::SaveStringToFile(ReportText, *(ExportDir / TEXT("validation_report.json")));

	return TotalErrors > 0 ? 1 : 0;
}

USekiroValidationCommandlet::FCharacterValidation USekiroValidationCommandlet::ValidateCharacter(
	const FString& ChrId, const FString& ContentBase, const FString& ExportDir)
{
	FCharacterValidation V;
	V.ChrId = ChrId;

	FString ChrContent = ContentBase / ChrId;
	USkeleton* ExpectedSkeleton = nullptr;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// 1. Validate Skeleton & SkeletalMesh (or StaticMesh for characters without bone weights)
	{
		TArray<FAssetData> MeshAssets;
		AssetRegistry.GetAssetsByPath(FName(*(ChrContent / TEXT("Mesh"))), MeshAssets, true);

		USkeleton* FoundSkeleton = nullptr;

		for (const FAssetData& AssetData : MeshAssets)
		{
			UObject* Asset = AssetData.GetAsset();
			if (!Asset) continue;

			if (USkeletalMesh* SkelMesh = Cast<USkeletalMesh>(Asset))
			{
				V.bHasSkeletalMesh = true;
				V.bHasPhysicsAsset = V.bHasPhysicsAsset || (SkelMesh->GetPhysicsAsset() != nullptr);
				if (SkelMesh->GetResourceForRendering())
				{
					const FSkeletalMeshRenderData* RenderData = SkelMesh->GetResourceForRendering();
					if (RenderData->LODRenderData.Num() > 0)
					{
						const FSkeletalMeshLODRenderData& LOD0 = RenderData->LODRenderData[0];
						V.MeshSectionCount = LOD0.RenderSections.Num();
						V.VertexCount = LOD0.GetNumVertices();
					}
				}
				if (SkelMesh->GetSkeleton())
				{
					FoundSkeleton = SkelMesh->GetSkeleton();
				}
			}
			else if (USkeleton* Skel = Cast<USkeleton>(Asset))
			{
				V.bHasSkeleton = true;
				V.BoneCount = Skel->GetReferenceSkeleton().GetRawBoneNum();
				FoundSkeleton = Skel;
			}
			else if (Cast<UStaticMesh>(Asset))
			{
				V.bHasStaticMesh = true;
			}
		}

		// If we found mesh but not skeleton explicitly, get it from mesh
		if (!V.bHasSkeleton && FoundSkeleton)
		{
			V.bHasSkeleton = true;
			V.BoneCount = FoundSkeleton->GetReferenceSkeleton().GetRawBoneNum();
		}

		ExpectedSkeleton = FoundSkeleton;
	}

	// 2. Validate AnimSequences
	{
		TArray<FAssetData> AnimAssets;
		AssetRegistry.GetAssetsByPath(FName(*(ChrContent / TEXT("Animations"))), AnimAssets, true);

		for (const FAssetData& AssetData : AnimAssets)
		{
			UAnimSequence* AnimSeq = Cast<UAnimSequence>(AssetData.GetAsset());
			if (!AnimSeq) continue;

			V.AnimSequenceCount++;

			float Duration = AnimSeq->GetPlayLength();
			V.TotalAnimDuration += Duration;

			if (Duration <= 0.0f)
				V.AnimsWithZeroDuration++;

			// Check bone tracks via data model
			const IAnimationDataModel* DataModel = AnimSeq->GetDataModel();
			if (DataModel)
			{
				int32 TrackCount = DataModel->GetNumBoneTracks();
				if (TrackCount == 0)
					V.AnimsWithZeroTracks++;
			}
			else
			{
				V.AnimsWithZeroTracks++;
			}

			// Check skeleton reference
			USkeleton* AnimSkeleton = AnimSeq->GetSkeleton();
			if (!AnimSkeleton || (V.bHasSkeleton && ExpectedSkeleton && AnimSkeleton != ExpectedSkeleton))
				V.AnimsWithWrongSkeleton++;
		}
	}

	// 3. Validate Textures
	// Note: GetSizeX()/GetSizeY() return 0 in commandlet mode without GPU (-nullrhi).
	// We validate texture count and existence via asset registry instead.
	{
		TArray<FAssetData> TexAssets;
		AssetRegistry.GetAssetsByPath(FName(*(ChrContent / TEXT("Textures"))), TexAssets, true);

		for (const FAssetData& AssetData : TexAssets)
		{
			if (AssetData.AssetClassPath.GetAssetName() == FName(TEXT("Texture2D")))
			{
				V.TextureCount++;
			}
		}
	}

	// 4. Validate Materials
	{
		TArray<FAssetData> MatAssets;
		AssetRegistry.GetAssetsByPath(FName(*(ChrContent / TEXT("Materials"))), MatAssets, true);

		for (const FAssetData& AssetData : MatAssets)
		{
			if (Cast<UMaterialInterface>(AssetData.GetAsset()))
				V.MaterialCount++;
		}
	}

	// 5. Validate Skill Config
	{
		FString ImportReportPath = ExportDir / ChrId / TEXT("ue_import_report.json");
		FString SkillJsonPath = ExportDir / ChrId / TEXT("Skills/skill_config.json");
		FString CopiedSkillJson = FPaths::ProjectContentDir() / TEXT("SekiroAssets/Characters") / ChrId / TEXT("Skills/skill_config.json");

		if (!FPaths::FileExists(ImportReportPath))
		{
			V.SkillSchemaErrors++;
		}

		if (FPaths::FileExists(CopiedSkillJson))
		{
			ValidateSkillConfig(ChrId, CopiedSkillJson, ChrContent, V);
		}
		else if (FPaths::FileExists(SkillJsonPath))
		{
			ValidateSkillConfig(ChrId, SkillJsonPath, ChrContent, V);
		}
	}

	return V;
}

void USekiroValidationCommandlet::ValidateSkillConfig(
	const FString& ChrId, const FString& SkillJsonPath, const FString& ContentBase, FCharacterValidation& Result)
{
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *SkillJsonPath))
		return;

	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
		return;

	Result.bHasSkillConfig = true;

	FString DeliveryMode;
	if (!Root->TryGetStringField(TEXT("deliveryMode"), DeliveryMode) || DeliveryMode != TEXT("formal-only"))
	{
		Result.SkillSchemaErrors++;
	}

	const TArray<TSharedPtr<FJsonValue>>* CharactersArray = nullptr;
	const TSharedPtr<FJsonObject>* ParamsObject = nullptr;
	if (!Root->TryGetArrayField(TEXT("characters"), CharactersArray) || !CharactersArray || CharactersArray->Num() == 0)
	{
		Result.SkillSchemaErrors++;
		return;
	}
	if (!Root->TryGetObjectField(TEXT("params"), ParamsObject) || !ParamsObject)
	{
		Result.SkillSchemaErrors++;
		return;
	}

	Result.bHasCanonicalSkillConfig = true;

	TSet<FString> EventCategories;
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> AnimAssets;
	AssetRegistry.GetAssetsByPath(FName(*(ContentBase / TEXT("Animations"))), AnimAssets, true);
	TSet<FString> ImportedAnimNames;
	for (const FAssetData& AD : AnimAssets)
	{
		ImportedAnimNames.Add(AD.AssetName.ToString());
	}

	TSharedPtr<FJsonObject> CharacterObject;
	for (const TSharedPtr<FJsonValue>& CharacterValue : *CharactersArray)
	{
		const TSharedPtr<FJsonObject> Candidate = CharacterValue->AsObject();
		if (!Candidate.IsValid())
		{
			Result.SkillSchemaErrors++;
			continue;
		}

		FString CharacterId;
		if (!Candidate->TryGetStringField(TEXT("id"), CharacterId))
		{
			Result.SkillSchemaErrors++;
			continue;
		}

		if (CharacterId == ChrId)
		{
			CharacterObject = Candidate;
			break;
		}
	}

	if (!CharacterObject.IsValid())
	{
		Result.SkillSchemaErrors++;
		return;
	}

	const TArray<TSharedPtr<FJsonValue>>* AnimationsArray = nullptr;
	if (!CharacterObject->TryGetArrayField(TEXT("animations"), AnimationsArray) || !AnimationsArray)
	{
		Result.SkillSchemaErrors++;
		return;
	}

	for (const TSharedPtr<FJsonValue>& AnimValue : *AnimationsArray)
	{
		const TSharedPtr<FJsonObject> AnimObj = AnimValue->AsObject();
		if (!AnimObj.IsValid())
		{
			Result.SkillSchemaErrors++;
			continue;
		}

		FString AnimName;
		FString FileName;
		double FrameRate = 0.0;
		double FrameCount = 0.0;
		if (!AnimObj->TryGetStringField(TEXT("name"), AnimName))
			Result.SkillSchemaErrors++;
		if (!AnimObj->TryGetStringField(TEXT("fileName"), FileName))
			Result.SkillSchemaErrors++;
		if (!AnimObj->HasField(TEXT("relativePath")))
			Result.SkillSchemaErrors++;
		if (!AnimObj->TryGetNumberField(TEXT("frameRate"), FrameRate) || FrameRate <= 0.0)
			Result.SkillSchemaErrors++;
		if (!AnimObj->TryGetNumberField(TEXT("frameCount"), FrameCount) || FrameCount <= 0.0)
			Result.SkillSchemaErrors++;

		Result.SkillAnimCount++;
		const FString ImportedAnimKey = !FileName.IsEmpty() ? FPaths::GetBaseFilename(FileName) : AnimName;
		if (ImportedAnimNames.Contains(ImportedAnimKey))
		{
			Result.SkillAnimsWithMatchingAsset++;
		}
		else
		{
			Result.SkillAnimsWithoutMatchingAsset++;
			if (AnimName.StartsWith(TEXT("a000_")) || AnimName.StartsWith(TEXT("a00_")))
			{
				Result.SkillAnimsSharedPool++;
			}
		}

		const TArray<TSharedPtr<FJsonValue>>* EventsArray = nullptr;
		if (!AnimObj->TryGetArrayField(TEXT("events"), EventsArray) || !EventsArray)
		{
			Result.SkillSchemaErrors++;
			continue;
		}

		Result.SkillEventCount += EventsArray->Num();
		for (const TSharedPtr<FJsonValue>& EventVal : *EventsArray)
		{
			const TSharedPtr<FJsonObject> EventObj = EventVal->AsObject();
			if (!EventObj.IsValid())
			{
				Result.SkillSchemaErrors++;
				continue;
			}

			FString Category;
			double StartFrame = -1.0;
			double EndFrame = -1.0;
			const TArray<TSharedPtr<FJsonValue>>* ParamsArray = nullptr;
			if (!EventObj->TryGetStringField(TEXT("category"), Category))
				Result.SkillSchemaErrors++;
			if (!EventObj->TryGetNumberField(TEXT("startFrame"), StartFrame) || StartFrame < 0.0)
				Result.SkillSchemaErrors++;
			if (!EventObj->TryGetNumberField(TEXT("endFrame"), EndFrame) || EndFrame < StartFrame)
				Result.SkillSchemaErrors++;
			if (!EventObj->TryGetArrayField(TEXT("params"), ParamsArray))
				Result.SkillSchemaErrors++;
			if (!EventObj->HasField(TEXT("parameters")))
				Result.SkillSchemaErrors++;

			if (!Category.IsEmpty())
			{
				EventCategories.Add(Category);
			}
		}
	}

	Result.SkillEventCategories = EventCategories.Num();
}
