#include "Import/SekiroRetargetCommandlet.h"

#include "Animation/AnimSequence.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/Skeleton.h"
#include "BoneIndices.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/SkeletalMesh.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/SavePackage.h"

namespace
{
	static const TCHAR* DefaultSourceMeshPath = TEXT("/Game/SekiroAssets/Characters/c0000/Mesh/c0000/SkeletalMeshes/c0000.c0000");
	static const TCHAR* DefaultTargetMeshPath = TEXT("/Game/GhostSamurai_Bundle/Demo/Characters/Mannequins/Meshes/SKM_Manny.SKM_Manny");
	static const TCHAR* DefaultOutputBasePath = TEXT("/Game/Retargeting/SekiroToManny");
	static const TCHAR* DefaultOutputAnimSuffix = TEXT("_Manny");

	// Hard requirement: only these 17 animations
	static TArray<FString> GetDefaultAnimationPaths()
	{
		return {
			TEXT("/Game/SekiroAssets/Characters/c0000/Animations/a000_201030.a000_201030"),
			TEXT("/Game/SekiroAssets/Characters/c0000/Animations/a000_201050.a000_201050"),
			TEXT("/Game/SekiroAssets/Characters/c0000/Animations/a000_202010.a000_202010"),
			TEXT("/Game/SekiroAssets/Characters/c0000/Animations/a000_202011.a000_202011"),
			TEXT("/Game/SekiroAssets/Characters/c0000/Animations/a000_202012.a000_202012"),
			TEXT("/Game/SekiroAssets/Characters/c0000/Animations/a000_202035.a000_202035"),
			TEXT("/Game/SekiroAssets/Characters/c0000/Animations/a000_202100.a000_202100"),
			TEXT("/Game/SekiroAssets/Characters/c0000/Animations/a000_202110.a000_202110"),
			TEXT("/Game/SekiroAssets/Characters/c0000/Animations/a000_202112.a000_202112"),
			TEXT("/Game/SekiroAssets/Characters/c0000/Animations/a000_202300.a000_202300"),
			TEXT("/Game/SekiroAssets/Characters/c0000/Animations/a000_202310.a000_202310"),
			TEXT("/Game/SekiroAssets/Characters/c0000/Animations/a000_202400.a000_202400"),
			TEXT("/Game/SekiroAssets/Characters/c0000/Animations/a000_202410.a000_202410"),
			TEXT("/Game/SekiroAssets/Characters/c0000/Animations/a000_202600.a000_202600"),
			TEXT("/Game/SekiroAssets/Characters/c0000/Animations/a000_202610.a000_202610"),
			TEXT("/Game/SekiroAssets/Characters/c0000/Animations/a000_202700.a000_202700"),
			TEXT("/Game/SekiroAssets/Characters/c0000/Animations/a000_202710.a000_202710")
		};
	}

	// Sekiro bone -> Manny bone name mapping for FK retarget
	static TMap<FName, FName> GetBoneNameMap()
	{
		TMap<FName, FName> Map;
		Map.Add(TEXT("Master"),     TEXT("root"));
		Map.Add(TEXT("Pelvis"),     TEXT("pelvis"));
		Map.Add(TEXT("Spine"),      TEXT("spine_01"));
		Map.Add(TEXT("Spine1"),     TEXT("spine_02"));
		Map.Add(TEXT("Spine2"),     TEXT("spine_03"));
		Map.Add(TEXT("Neck"),       TEXT("neck_01"));
		Map.Add(TEXT("Head"),       TEXT("head"));
		Map.Add(TEXT("L_Shoulder"), TEXT("clavicle_l"));
		Map.Add(TEXT("L_UpperArm"), TEXT("upperarm_l"));
		Map.Add(TEXT("L_Forearm"),  TEXT("lowerarm_l"));
		Map.Add(TEXT("L_Hand"),     TEXT("hand_l"));
		Map.Add(TEXT("R_Shoulder"), TEXT("clavicle_r"));
		Map.Add(TEXT("R_UpperArm"), TEXT("upperarm_r"));
		Map.Add(TEXT("R_Forearm"),  TEXT("lowerarm_r"));
		Map.Add(TEXT("R_Hand"),     TEXT("hand_r"));
		Map.Add(TEXT("L_Thigh"),    TEXT("thigh_l"));
		Map.Add(TEXT("L_Calf"),     TEXT("calf_l"));
		Map.Add(TEXT("L_Foot"),     TEXT("foot_l"));
		Map.Add(TEXT("L_Toe0"),     TEXT("ball_l"));
		Map.Add(TEXT("R_Thigh"),    TEXT("thigh_r"));
		Map.Add(TEXT("R_Calf"),     TEXT("calf_r"));
		Map.Add(TEXT("R_Foot"),     TEXT("foot_r"));
		Map.Add(TEXT("R_Toe0"),     TEXT("ball_r"));
		return Map;
	}

	static FString CombinePackagePath(const FString& Left, const FString& Right)
	{
		FString Result = Left;
		Result.RemoveFromEnd(TEXT("/"));
		return Result + TEXT("/") + Right;
	}

	static FString SanitizeObjectPath(const FString& RawPath)
	{
		FString Path = RawPath.TrimStartAndEnd();
		int32 QuoteIndex = INDEX_NONE;
		if (Path.FindChar(TEXT('\''), QuoteIndex))
		{
			Path = Path.Mid(QuoteIndex + 1);
			Path.RemoveFromEnd(TEXT("'"));
		}
		if (!Path.Contains(TEXT(".")) && Path.StartsWith(TEXT("/")))
		{
			const FString AssetName = FPackageName::GetLongPackageAssetName(Path);
			Path += TEXT(".") + AssetName;
		}
		return Path;
	}

	template <typename T>
	static T* LoadTypedAsset(const FString& RawPath, const TCHAR* AssetLabel)
	{
		const FString ObjectPath = SanitizeObjectPath(RawPath);
		T* Asset = LoadObject<T>(nullptr, *ObjectPath);
		if (!Asset)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to load %s: %s"), AssetLabel, *ObjectPath);
		}
		return Asset;
	}

	static bool SaveAssetPackage(UObject* Asset)
	{
		if (!Asset) return false;
		UPackage* Package = Asset->GetOutermost();
		if (!Package) return false;
		Package->SetDirtyFlag(true);
		Package->MarkPackageDirty();
		const FString PackageFileName = FPackageName::LongPackageNameToFilename(
			Package->GetName(), FPackageName::GetAssetPackageExtension());
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(PackageFileName), true);
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = EObjectFlags::RF_Public | EObjectFlags::RF_Standalone;
		SaveArgs.Error = GWarn;
		const bool bSaved = UPackage::SavePackage(Package, Asset, *PackageFileName, SaveArgs);
		UE_LOG(LogTemp, Display, TEXT("Saved %s (%s)"), *Package->GetName(), bSaved ? TEXT("ok") : TEXT("FAILED"));
		return bSaved;
	}

	static void ParseObjectPathList(const FString& RawList, TArray<FString>& OutPaths)
	{
		FString Normalized = RawList;
		Normalized.ReplaceInline(TEXT(";"), TEXT(","));
		Normalized.ReplaceInline(TEXT("\r"), TEXT(","));
		Normalized.ReplaceInline(TEXT("\n"), TEXT(","));
		TArray<FString> Parts;
		Normalized.ParseIntoArray(Parts, TEXT(","), true);
		for (const FString& Part : Parts)
		{
			if (!Part.TrimStartAndEnd().IsEmpty())
				OutPaths.Add(Part.TrimStartAndEnd());
		}
	}

	// Manual FK retarget: sample source animation per-frame via GetBoneTransform(),
	// then write bone tracks to target animation via IAnimationDataController.
	// No IKRig/IKRetarget system used — avoids the UE5.7 zero-length chain crash entirely.
	static UAnimSequence* RetargetAnimationFK(
		UAnimSequence* SourceSeq,
		USkeleton* TargetSkeleton,
		const TMap<FName, FName>& BoneMap,
		const FString& OutputPath,
		const FString& AssetName)
	{
		const IAnimationDataModel* SourceModel = SourceSeq->GetDataModel();
		if (!SourceModel)
		{
			UE_LOG(LogTemp, Error, TEXT("  No data model on source: %s"), *SourceSeq->GetName());
			return nullptr;
		}

		const int32 NumFrames = SourceModel->GetNumberOfFrames();
		const FFrameRate FrameRate = SourceModel->GetFrameRate();
		const double PlayLength = SourceSeq->GetPlayLength();
		const FReferenceSkeleton& TargetRefSkel = TargetSkeleton->GetReferenceSkeleton();

		// Get source skeleton for bone enumeration
		USkeleton* SourceSkeleton = SourceSeq->GetSkeleton();
		if (!SourceSkeleton)
		{
			UE_LOG(LogTemp, Error, TEXT("  No skeleton on source: %s"), *SourceSeq->GetName());
			return nullptr;
		}
		const FReferenceSkeleton& SourceRefSkel = SourceSkeleton->GetReferenceSkeleton();
		const int32 SourceBoneCount = SourceRefSkel.GetNum();

		// Build list of bones to sample: find source bones that are in the bone map
		struct FBoneMapping
		{
			int32 SourceBoneIndex;
			FName SourceBoneName;
			FName TargetBoneName;
		};
		TArray<FBoneMapping> Mappings;

		for (int32 SrcIdx = 0; SrcIdx < SourceBoneCount; ++SrcIdx)
		{
			const FName SrcBoneName = SourceRefSkel.GetBoneName(SrcIdx);
			const FName* TargetBoneName = BoneMap.Find(SrcBoneName);
			if (!TargetBoneName)
				continue;

			if (TargetRefSkel.FindBoneIndex(*TargetBoneName) == INDEX_NONE)
			{
				UE_LOG(LogTemp, Warning, TEXT("  Target bone '%s' not found in target skeleton, skipping"), *TargetBoneName->ToString());
				continue;
			}

			Mappings.Add({SrcIdx, SrcBoneName, *TargetBoneName});
		}

		UE_LOG(LogTemp, Display, TEXT("  Found %d bone mappings in source skeleton (%d bones total)"), Mappings.Num(), SourceBoneCount);

		// Create output package and animation
		const FString PackageName = CombinePackagePath(OutputPath, AssetName);
		UPackage* Package = CreatePackage(*PackageName);
		if (!Package)
		{
			UE_LOG(LogTemp, Error, TEXT("  Failed to create package: %s"), *PackageName);
			return nullptr;
		}

		UAnimSequence* TargetSeq = NewObject<UAnimSequence>(Package, *AssetName, RF_Public | RF_Standalone);
		TargetSeq->SetSkeleton(TargetSkeleton);

		IAnimationDataController& Controller = TargetSeq->GetController();
		Controller.OpenBracket(FText::FromString(TEXT("FK Retarget")), false);
		Controller.SetFrameRate(FrameRate, false);
		Controller.SetNumberOfFrames(FFrameNumber(NumFrames), false);

		int32 MappedCount = 0;

		for (const FBoneMapping& M : Mappings)
		{
			// Sample the source animation at every frame for this bone
			const int32 TotalKeys = NumFrames + 1; // include last frame
			TArray<FVector3f> PosKeys;
			TArray<FQuat4f> RotKeys;
			TArray<FVector3f> ScaleKeys;
			PosKeys.SetNum(TotalKeys);
			RotKeys.SetNum(TotalKeys);
			ScaleKeys.SetNum(TotalKeys);

			const FSkeletonPoseBoneIndex BoneIndex(M.SourceBoneIndex);

			for (int32 Frame = 0; Frame < TotalKeys; ++Frame)
			{
				const double Time = (NumFrames > 0) ? (PlayLength * Frame / NumFrames) : 0.0;
				FAnimExtractContext ExtractionCtx(Time);

				FTransform BoneTransform;
				SourceSeq->GetBoneTransform(BoneTransform, BoneIndex, ExtractionCtx, false);

				PosKeys[Frame] = FVector3f(BoneTransform.GetLocation());
				RotKeys[Frame] = FQuat4f(BoneTransform.GetRotation());
				ScaleKeys[Frame] = FVector3f(BoneTransform.GetScale3D());
			}

			Controller.AddBoneCurve(M.TargetBoneName, false);
			Controller.SetBoneTrackKeys(M.TargetBoneName, PosKeys, RotKeys, ScaleKeys, false);
			MappedCount++;

			UE_LOG(LogTemp, Verbose, TEXT("    Mapped %s -> %s (%d keys)"), *M.SourceBoneName.ToString(), *M.TargetBoneName.ToString(), TotalKeys);
		}

		Controller.CloseBracket(false);

		TargetSeq->bEnableRootMotion = true;
		TargetSeq->bForceRootLock = false;

		FAssetRegistryModule::AssetCreated(TargetSeq);
		TargetSeq->MarkPackageDirty();
		SaveAssetPackage(TargetSeq);

		UE_LOG(LogTemp, Display, TEXT("  Retargeted %s -> %s (%d/%d bone mappings, %d frames)"),
			*SourceSeq->GetName(), *AssetName, MappedCount, Mappings.Num(), NumFrames);
		return TargetSeq;
	}
}

USekiroRetargetCommandlet::USekiroRetargetCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 USekiroRetargetCommandlet::Main(const FString& Params)
{
	TArray<FString> Tokens;
	TArray<FString> Switches;
	TMap<FString, FString> ParamsMap;
	ParseCommandLine(*Params, Tokens, Switches, ParamsMap);

	const FString SourceMeshPath = ParamsMap.Contains(TEXT("SourceMesh")) ? ParamsMap[TEXT("SourceMesh")] : DefaultSourceMeshPath;
	const FString TargetMeshPath = ParamsMap.Contains(TEXT("TargetMesh")) ? ParamsMap[TEXT("TargetMesh")] : DefaultTargetMeshPath;
	const FString OutputBasePath = ParamsMap.Contains(TEXT("OutputBase")) ? ParamsMap[TEXT("OutputBase")] : DefaultOutputBasePath;
	const FString OutputAnimPath = ParamsMap.Contains(TEXT("OutputAnimPath")) ? ParamsMap[TEXT("OutputAnimPath")] : CombinePackagePath(OutputBasePath, TEXT("Animations"));
	const FString OutputSuffix = ParamsMap.Contains(TEXT("OutputSuffix")) ? ParamsMap[TEXT("OutputSuffix")] : DefaultOutputAnimSuffix;

	TArray<FString> AnimationPaths = GetDefaultAnimationPaths();
	if (ParamsMap.Contains(TEXT("Animations")))
	{
		AnimationPaths.Reset();
		ParseObjectPathList(ParamsMap[TEXT("Animations")], AnimationPaths);
	}

	UE_LOG(LogTemp, Display, TEXT(""));
	UE_LOG(LogTemp, Display, TEXT("=============================================="));
	UE_LOG(LogTemp, Display, TEXT("   Sekiro FK Retarget Commandlet"));
	UE_LOG(LogTemp, Display, TEXT("=============================================="));
	UE_LOG(LogTemp, Display, TEXT("SourceMesh: %s"), *SanitizeObjectPath(SourceMeshPath));
	UE_LOG(LogTemp, Display, TEXT("TargetMesh: %s"), *SanitizeObjectPath(TargetMeshPath));
	UE_LOG(LogTemp, Display, TEXT("OutputAnimPath: %s"), *OutputAnimPath);
	UE_LOG(LogTemp, Display, TEXT("AnimationCount: %d"), AnimationPaths.Num());
	UE_LOG(LogTemp, Display, TEXT("Method: Manual FK bone-name-mapped retarget (bypasses IKRetarget to avoid zero-length chain crash)"));

	USkeletalMesh* TargetMesh = LoadTypedAsset<USkeletalMesh>(TargetMeshPath, TEXT("target skeletal mesh"));
	if (!TargetMesh || !TargetMesh->GetSkeleton())
	{
		UE_LOG(LogTemp, Error, TEXT("Target mesh or skeleton not available."));
		return 1;
	}

	USkeleton* TargetSkeleton = TargetMesh->GetSkeleton();
	const TMap<FName, FName> BoneMap = GetBoneNameMap();

	UE_LOG(LogTemp, Display, TEXT("Bone mapping (%d entries):"), BoneMap.Num());
	for (const auto& Pair : BoneMap)
	{
		UE_LOG(LogTemp, Display, TEXT("  %s -> %s"), *Pair.Key.ToString(), *Pair.Value.ToString());
	}

	int32 SuccessCount = 0;
	for (const FString& AnimPath : AnimationPaths)
	{
		UAnimSequence* SourceSeq = LoadTypedAsset<UAnimSequence>(AnimPath, TEXT("source animation"));
		if (!SourceSeq)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to load source animation: %s"), *AnimPath);
			continue;
		}

		const FString OutName = SourceSeq->GetName() + OutputSuffix;
		UAnimSequence* Result = RetargetAnimationFK(SourceSeq, TargetSkeleton, BoneMap, OutputAnimPath, OutName);
		if (Result)
		{
			SuccessCount++;
		}
	}

	UE_LOG(LogTemp, Display, TEXT(""));
	UE_LOG(LogTemp, Display, TEXT("=============================================="));
	UE_LOG(LogTemp, Display, TEXT("  FK Retarget complete: %d/%d animations"), SuccessCount, AnimationPaths.Num());
	UE_LOG(LogTemp, Display, TEXT("=============================================="));

	return (SuccessCount == AnimationPaths.Num()) ? 0 : 1;
}