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
#include "InterchangeAnimationTrackSetNode.h"
#include "InterchangeCommonAnimationPayload.h"
#include "InterchangeSceneNode.h"
#include "Animation/InterchangeAnimationPayloadInterface.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkinWeightVertexBuffer.h"
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
#include "Import/SekiroHumanoidImportPipeline.h"
#include "Import/SekiroMaterialSetup.h"
#include "Data/SekiroCharacterData.h"
#include "Data/SekiroSkillDataAsset.h"

namespace
{
		struct FDirectionMetricSpec
		{
			const TCHAR* MetricName;
			const TCHAR* StartBone;
			const TCHAR* EndBone;
		};

		struct FHandBasisMetricSpec
		{
			const TCHAR* Prefix;
			const TCHAR* HandBone;
			const TCHAR* IndexBone;
			const TCHAR* MiddleBone;
			const TCHAR* PinkyBone;
		};

		struct FHandBranchBasis
		{
			bool bValid = false;
			FVector PalmForward = FVector::ZeroVector;
			FVector PalmLateral = FVector::ZeroVector;
			FVector PalmNormal = FVector::ZeroVector;
		};

		struct FImportedAnimationValidationSummary
		{
			FString ClipName;
			bool bSuccess = false;
			TMap<FString, double> MinimumDirectionDots;
			TMap<FString, double> MinimumHandBasisDots;
			TArray<FString> Issues;
		};

	struct FGltfPayload
	{
		FString JsonString;
		TArray<uint8> BinaryChunk;
	};

	constexpr double GltfToUeLengthScale = 100.0;

		static const TArray<FDirectionMetricSpec>& GetImportAnimationDirectionMetrics()
		{
			static const TArray<FDirectionMetricSpec> Metrics = {
				{ TEXT("left_shoulder_hand"), TEXT("L_Shoulder"), TEXT("L_Hand") },
				{ TEXT("left_upper_lower"), TEXT("L_UpperArm"), TEXT("L_Forearm") },
				{ TEXT("left_lower_hand"), TEXT("L_Forearm"), TEXT("L_Hand") },
				{ TEXT("right_shoulder_hand"), TEXT("R_Shoulder"), TEXT("R_Hand") },
				{ TEXT("right_upper_lower"), TEXT("R_UpperArm"), TEXT("R_Forearm") },
				{ TEXT("right_lower_hand"), TEXT("R_Forearm"), TEXT("R_Hand") },
			};
			return Metrics;
		}

		static const TArray<FHandBasisMetricSpec>& GetImportAnimationHandBasisMetrics()
		{
			static const TArray<FHandBasisMetricSpec> Metrics = {
				{ TEXT("left_hand"), TEXT("L_Hand"), TEXT("L_Finger1"), TEXT("L_Finger2"), TEXT("L_Finger4") },
				{ TEXT("right_hand"), TEXT("R_Hand"), TEXT("R_Finger1"), TEXT("R_Finger2"), TEXT("R_Finger4") },
			};
			return Metrics;
		}

		static const TMap<FString, double>& GetImportAnimationDirectionThresholds()
		{
			static const TMap<FString, double> Thresholds = {
				{ TEXT("left_shoulder_hand"), 0.99 },
				{ TEXT("left_upper_lower"), 0.99 },
				{ TEXT("left_lower_hand"), 0.99 },
				{ TEXT("right_shoulder_hand"), 0.99 },
				{ TEXT("right_upper_lower"), 0.99 },
				{ TEXT("right_lower_hand"), 0.99 },
			};
			return Thresholds;
		}

		static const TMap<FString, double>& GetImportAnimationHandBasisThresholds()
		{
			static const TMap<FString, double> Thresholds = {
				{ TEXT("left_hand_palm_forward"), 0.98 },
				{ TEXT("left_hand_palm_lateral"), 0.98 },
				{ TEXT("left_hand_palm_normal"), 0.98 },
				{ TEXT("right_hand_palm_forward"), 0.98 },
				{ TEXT("right_hand_palm_lateral"), 0.98 },
				{ TEXT("right_hand_palm_normal"), 0.98 },
			};
			return Thresholds;
		}

		static bool TryGetBoneTranslation(const TMap<FString, FTransform>& BoneWorldByName, const FString& BoneName, FVector& OutTranslation)
		{
			if (const FTransform* BoneTransform = BoneWorldByName.Find(BoneName))
			{
				OutTranslation = BoneTransform->GetLocation();
				return true;
			}
			return false;
		}

		static bool TryGetNormalizedDirection(const TMap<FString, FTransform>& BoneWorldByName, const FString& StartBone, const FString& EndBone, FVector& OutDirection)
		{
			FVector StartTranslation = FVector::ZeroVector;
			FVector EndTranslation = FVector::ZeroVector;
			if (!TryGetBoneTranslation(BoneWorldByName, StartBone, StartTranslation)
				|| !TryGetBoneTranslation(BoneWorldByName, EndBone, EndTranslation))
			{
				return false;
			}

			OutDirection = (EndTranslation - StartTranslation).GetSafeNormal();
			return !OutDirection.IsNearlyZero();
		}

		static bool TryComputeHandBranchBasis(
			const TMap<FString, FTransform>& BoneWorldByName,
			const FString& HandBone,
			const FString& IndexBone,
			const FString& MiddleBone,
			const FString& PinkyBone,
			FHandBranchBasis& OutBasis)
		{
			FVector HandTranslation = FVector::ZeroVector;
			FVector IndexTranslation = FVector::ZeroVector;
			FVector MiddleTranslation = FVector::ZeroVector;
			FVector PinkyTranslation = FVector::ZeroVector;
			if (!TryGetBoneTranslation(BoneWorldByName, HandBone, HandTranslation)
				|| !TryGetBoneTranslation(BoneWorldByName, IndexBone, IndexTranslation)
				|| !TryGetBoneTranslation(BoneWorldByName, MiddleBone, MiddleTranslation)
				|| !TryGetBoneTranslation(BoneWorldByName, PinkyBone, PinkyTranslation))
			{
				return false;
			}

			const FVector PalmForward = (MiddleTranslation - HandTranslation).GetSafeNormal();
			const FVector PalmLateralSeed = (IndexTranslation - PinkyTranslation).GetSafeNormal();
			const FVector PalmNormal = FVector::CrossProduct(PalmForward, PalmLateralSeed).GetSafeNormal();
			const FVector PalmLateral = FVector::CrossProduct(PalmNormal, PalmForward).GetSafeNormal();

			if (PalmForward.IsNearlyZero() || PalmLateral.IsNearlyZero() || PalmNormal.IsNearlyZero())
			{
				return false;
			}

			OutBasis.bValid = true;
			OutBasis.PalmForward = PalmForward;
			OutBasis.PalmLateral = PalmLateral;
			OutBasis.PalmNormal = PalmNormal;
			return true;
		}

		static void AccumulateMinimumMetric(TMap<FString, double>& InOutMetrics, const FString& MetricName, const double Value)
		{
			const double* ExistingValue = InOutMetrics.Find(MetricName);
			if (!ExistingValue || Value < *ExistingValue)
			{
				InOutMetrics.Add(MetricName, Value);
			}
		}

		static void AccumulateImportedAnimationValidationFrame(
			const TMap<FString, FTransform>& SourceNormalizedWorldByBone,
			const TMap<FString, FTransform>& TargetWorldByBone,
			TMap<FString, double>& InOutMinimumDirectionDots,
			TSet<FString>& InOutSeenDirectionMetrics,
			TMap<FString, double>& InOutMinimumHandBasisDots,
			TSet<FString>& InOutSeenHandBasisMetrics)
		{
			for (const FDirectionMetricSpec& Metric : GetImportAnimationDirectionMetrics())
			{
				FVector SourceDirection = FVector::ZeroVector;
				FVector TargetDirection = FVector::ZeroVector;
				if (!TryGetNormalizedDirection(SourceNormalizedWorldByBone, Metric.StartBone, Metric.EndBone, SourceDirection)
					|| !TryGetNormalizedDirection(TargetWorldByBone, Metric.StartBone, Metric.EndBone, TargetDirection))
				{
					continue;
				}

				AccumulateMinimumMetric(InOutMinimumDirectionDots, Metric.MetricName, FVector::DotProduct(SourceDirection, TargetDirection));
				InOutSeenDirectionMetrics.Add(Metric.MetricName);
			}

			for (const FHandBasisMetricSpec& Metric : GetImportAnimationHandBasisMetrics())
			{
				FHandBranchBasis SourceBasis;
				FHandBranchBasis TargetBasis;
				if (!TryComputeHandBranchBasis(SourceNormalizedWorldByBone, Metric.HandBone, Metric.IndexBone, Metric.MiddleBone, Metric.PinkyBone, SourceBasis)
					|| !TryComputeHandBranchBasis(TargetWorldByBone, Metric.HandBone, Metric.IndexBone, Metric.MiddleBone, Metric.PinkyBone, TargetBasis))
				{
					continue;
				}

				const FString ForwardMetric = FString::Printf(TEXT("%s_palm_forward"), Metric.Prefix);
				const FString LateralMetric = FString::Printf(TEXT("%s_palm_lateral"), Metric.Prefix);
				const FString NormalMetric = FString::Printf(TEXT("%s_palm_normal"), Metric.Prefix);

				AccumulateMinimumMetric(InOutMinimumHandBasisDots, ForwardMetric, FVector::DotProduct(SourceBasis.PalmForward, TargetBasis.PalmForward));
				AccumulateMinimumMetric(InOutMinimumHandBasisDots, LateralMetric, FVector::DotProduct(SourceBasis.PalmLateral, TargetBasis.PalmLateral));
				AccumulateMinimumMetric(InOutMinimumHandBasisDots, NormalMetric, FVector::DotProduct(SourceBasis.PalmNormal, TargetBasis.PalmNormal));

				InOutSeenHandBasisMetrics.Add(ForwardMetric);
				InOutSeenHandBasisMetrics.Add(LateralMetric);
				InOutSeenHandBasisMetrics.Add(NormalMetric);
			}
		}

		static void FinalizeImportedAnimationValidation(
			FImportedAnimationValidationSummary& OutSummary,
			const TMap<FString, double>& MinimumDirectionDots,
			const TSet<FString>& SeenDirectionMetrics,
			const TMap<FString, double>& MinimumHandBasisDots,
			const TSet<FString>& SeenHandBasisMetrics)
		{
			OutSummary.MinimumDirectionDots = MinimumDirectionDots;
			OutSummary.MinimumHandBasisDots = MinimumHandBasisDots;

			for (const TPair<FString, double>& Threshold : GetImportAnimationDirectionThresholds())
			{
				if (!SeenDirectionMetrics.Contains(Threshold.Key))
				{
					OutSummary.Issues.Add(FString::Printf(TEXT("missing imported animation direction metric '%s'"), *Threshold.Key));
					continue;
				}

				const double* Value = MinimumDirectionDots.Find(Threshold.Key);
				if (!Value || *Value < Threshold.Value)
				{
					OutSummary.Issues.Add(FString::Printf(TEXT("imported animation direction metric '%s' fell below %.3f: %.6f"), *Threshold.Key, Threshold.Value, Value ? *Value : -1.0));
				}
			}

			for (const TPair<FString, double>& Threshold : GetImportAnimationHandBasisThresholds())
			{
				if (!SeenHandBasisMetrics.Contains(Threshold.Key))
				{
					OutSummary.Issues.Add(FString::Printf(TEXT("missing imported animation hand basis metric '%s'"), *Threshold.Key));
					continue;
				}

				const double* Value = MinimumHandBasisDots.Find(Threshold.Key);
				if (!Value || *Value < Threshold.Value)
				{
					OutSummary.Issues.Add(FString::Printf(TEXT("imported animation hand basis metric '%s' fell below %.3f: %.6f"), *Threshold.Key, Threshold.Value, Value ? *Value : -1.0));
				}
			}

			OutSummary.bSuccess = OutSummary.Issues.Num() == 0;
		}

		static void BuildChildBonesByParent(const TMap<FString, FString>& ParentByBone, TMap<FString, TArray<FString>>& OutChildBonesByParent)
		{
			OutChildBonesByParent.Reset();
			for (const TPair<FString, FString>& Pair : ParentByBone)
			{
				if (!Pair.Value.IsEmpty())
				{
					OutChildBonesByParent.FindOrAdd(Pair.Value).Add(Pair.Key);
				}
			}
		}

		static void ApplyComponentRotationRebaseToSubtree(
			const FString& RootBoneName,
			const FQuat& RotationDelta,
			const TMap<FString, TArray<FString>>& ChildBonesByParent,
			TMap<FString, FTransform>& InOutWorldByBone)
		{
			FTransform* RootWorldTransform = InOutWorldByBone.Find(RootBoneName);
			if (!RootWorldTransform)
			{
				return;
			}

			const FVector PivotLocation = RootWorldTransform->GetLocation();
			TArray<FString> BoneStack = { RootBoneName };
			while (!BoneStack.IsEmpty())
			{
				const FString BoneName = BoneStack.Pop(EAllowShrinking::No);
				if (FTransform* BoneWorldTransform = InOutWorldByBone.Find(BoneName))
				{
					BoneWorldTransform->SetRotation((RotationDelta * BoneWorldTransform->GetRotation()).GetNormalized());
					if (BoneName != RootBoneName)
					{
						const FVector RelativeLocation = BoneWorldTransform->GetLocation() - PivotLocation;
						BoneWorldTransform->SetLocation(PivotLocation + RotationDelta.RotateVector(RelativeLocation));
					}
				}

				if (const TArray<FString>* ChildBones = ChildBonesByParent.Find(BoneName))
				{
					for (const FString& ChildBoneName : *ChildBones)
					{
						BoneStack.Add(ChildBoneName);
					}
				}
			}
		}

		static void ApplyComponentRotationRebasesToWorldMap(
			const TMap<FString, FQuat>& ComponentRotationRebaseByBone,
			const TMap<FString, FString>& ParentByBone,
			TMap<FString, FTransform>& InOutWorldByBone)
		{
			TMap<FString, TArray<FString>> ChildBonesByParent;
			BuildChildBonesByParent(ParentByBone, ChildBonesByParent);
			for (const TPair<FString, FQuat>& Pair : ComponentRotationRebaseByBone)
			{
				ApplyComponentRotationRebaseToSubtree(Pair.Key, Pair.Value, ChildBonesByParent, InOutWorldByBone);
			}
		}

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

	static bool DeleteAssetIfPresent(const FString& AssetObjectPath)
	{
		if (!UEditorAssetLibrary::DoesAssetExist(AssetObjectPath))
		{
			return true;
		}

		const bool bDeleted = UEditorAssetLibrary::DeleteAsset(AssetObjectPath);
		UE_LOG(LogTemp, Display, TEXT("    DeleteAsset %s -> %s"), *AssetObjectPath, bDeleted ? TEXT("success") : TEXT("failed"));
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

	static void ValidateNormalizedHumanoidAssets(USkeleton* Skeleton, USkeletalMesh* SkeletalMesh, TArray<FString>& Errors)
	{
		if (!Skeleton)
		{
			Errors.Add(TEXT("normalized skeleton missing after model import"));
			return;
		}

		ValidateImportedHumanoidTorsoChain(Skeleton, Errors);

		const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();
		const int32 PelvisIndex = FindExactBoneIndex(RefSkeleton, TEXT("Pelvis"));
		const int32 HeadIndex = FindExactBoneIndex(RefSkeleton, TEXT("Head"));
		if (PelvisIndex == INDEX_NONE || HeadIndex == INDEX_NONE)
		{
			Errors.Add(TEXT("normalized skeleton is missing the required Pelvis/Head bind chain"));
			return;
		}

		const TArray<FTransform>& RefPose = RefSkeleton.GetRefBonePose();
		TArray<FTransform> WorldPose;
		WorldPose.SetNum(RefSkeleton.GetNum());
		for (int32 BoneIndex = 0; BoneIndex < RefSkeleton.GetNum(); ++BoneIndex)
		{
			const int32 ParentIndex = RefSkeleton.GetParentIndex(BoneIndex);
			WorldPose[BoneIndex] = ParentIndex == INDEX_NONE
				? RefPose[BoneIndex]
				: RefPose[BoneIndex] * WorldPose[ParentIndex];
		}

		if (WorldPose[HeadIndex].GetLocation().Z <= WorldPose[PelvisIndex].GetLocation().Z)
		{
			Errors.Add(TEXT("normalized bind pose is inverted: Head is not above Pelvis in UE Z"));
		}

		const int32 LeftHandIndex = FindExactBoneIndex(RefSkeleton, TEXT("L_Hand"));
		const int32 RightHandIndex = FindExactBoneIndex(RefSkeleton, TEXT("R_Hand"));
		const int32 LeftShoulderIndex = FindExactBoneIndex(RefSkeleton, TEXT("L_Shoulder"));
		const int32 RightShoulderIndex = FindExactBoneIndex(RefSkeleton, TEXT("R_Shoulder"));
		if (LeftShoulderIndex != INDEX_NONE && RightShoulderIndex != INDEX_NONE)
		{
			UE_LOG(LogTemp, Display, TEXT("  Imported world shoulders: L=%s R=%s Delta=%s"),
				*WorldPose[LeftShoulderIndex].GetLocation().ToString(),
				*WorldPose[RightShoulderIndex].GetLocation().ToString(),
				*(WorldPose[LeftShoulderIndex].GetLocation() - WorldPose[RightShoulderIndex].GetLocation()).ToString());
		}
		if (LeftHandIndex != INDEX_NONE && RightHandIndex != INDEX_NONE)
		{
			UE_LOG(LogTemp, Display, TEXT("  Imported world hands: L=%s R=%s Delta=%s"),
				*WorldPose[LeftHandIndex].GetLocation().ToString(),
				*WorldPose[RightHandIndex].GetLocation().ToString(),
				*(WorldPose[LeftHandIndex].GetLocation() - WorldPose[RightHandIndex].GetLocation()).ToString());
			const float HandSeparationX = FMath::Abs(WorldPose[LeftHandIndex].GetLocation().X - WorldPose[RightHandIndex].GetLocation().X);
			if (HandSeparationX < 1.0f)
			{
				Errors.Add(TEXT("normalized bind pose collapsed on the lateral axis: left/right hands are not separated on UE X"));
			}
		}

		if (LeftShoulderIndex != INDEX_NONE && RightShoulderIndex != INDEX_NONE)
		{
			FVector ImportedLateralAxis = WorldPose[LeftShoulderIndex].GetLocation() - WorldPose[RightShoulderIndex].GetLocation();
			if (ImportedLateralAxis.Normalize())
			{
				const FVector ImportedUpAxis = (WorldPose[HeadIndex].GetLocation() - WorldPose[PelvisIndex].GetLocation()).GetSafeNormal();
				const FVector ImportedForwardAxis = FVector::CrossProduct(ImportedUpAxis, ImportedLateralAxis).GetSafeNormal();
				UE_LOG(LogTemp, Display, TEXT("  Imported derived axes: Lateral=%s Up=%s Forward=%s"),
					*ImportedLateralAxis.ToString(),
					*ImportedUpAxis.ToString(),
					*ImportedForwardAxis.ToString());
			}
		}

		if (SkeletalMesh && SkeletalMesh->GetSkeleton() && SkeletalMesh->GetSkeleton() != Skeleton)
		{
			Errors.Add(TEXT("normalized skeletal mesh was imported with the wrong skeleton binding"));
		}
	}

	static void LogSectionDominantBonesByMaterial(USkeletalMesh* SkeletalMesh)
	{
		if (!SkeletalMesh)
		{
			return;
		}

		const FSkeletalMeshRenderData* RenderData = SkeletalMesh->GetResourceForRendering();
		if (!RenderData || RenderData->LODRenderData.Num() == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("  Skeletal mesh render data unavailable for section diagnostics"));
			return;
		}

		const FSkeletalMeshLODRenderData& LODRenderData = RenderData->LODRenderData[0];
		TArray<FSkinWeightInfo> SkinWeights;
		LODRenderData.SkinWeightVertexBuffer.GetSkinWeights(SkinWeights);
		const FReferenceSkeleton& RefSkeleton = SkeletalMesh->GetRefSkeleton();

		for (int32 SectionIndex = 0; SectionIndex < LODRenderData.RenderSections.Num(); ++SectionIndex)
		{
			const FSkelMeshRenderSection& Section = LODRenderData.RenderSections[SectionIndex];
			const int32 MaterialIndex = Section.MaterialIndex;
			if (!SkeletalMesh->GetMaterials().IsValidIndex(MaterialIndex))
			{
				continue;
			}

			const FString MaterialSlotName = SkeletalMesh->GetMaterials()[MaterialIndex].MaterialSlotName.ToString();
			if (!MaterialSlotName.Equals(TEXT("artificialarm"), ESearchCase::IgnoreCase))
			{
				continue;
			}

			TMap<FString, double> BoneWeightTotals;
			FVector SectionPositionSum = FVector::ZeroVector;
			uint32 SectionPositionCount = 0;
			for (uint32 VertexOffset = 0; VertexOffset < Section.NumVertices; ++VertexOffset)
			{
				const uint32 VertexIndex = Section.BaseVertexIndex + VertexOffset;
				SectionPositionSum += FVector(LODRenderData.StaticVertexBuffers.PositionVertexBuffer.VertexPosition(VertexIndex));
				++SectionPositionCount;
				if (!SkinWeights.IsValidIndex(static_cast<int32>(VertexIndex)))
				{
					continue;
				}

				const FSkinWeightInfo& SkinWeight = SkinWeights[VertexIndex];
				uint8 DominantInfluenceIndex = 0;
				uint16 DominantWeight = 0;
				for (uint8 InfluenceIndex = 0; InfluenceIndex < MAX_TOTAL_INFLUENCES; ++InfluenceIndex)
				{
					if (SkinWeight.InfluenceWeights[InfluenceIndex] > DominantWeight)
					{
						DominantWeight = SkinWeight.InfluenceWeights[InfluenceIndex];
						DominantInfluenceIndex = InfluenceIndex;
					}
				}

				if (DominantWeight == 0 || !Section.BoneMap.IsValidIndex(SkinWeight.InfluenceBones[DominantInfluenceIndex]))
				{
					continue;
				}

				const int32 BoneIndex = Section.BoneMap[SkinWeight.InfluenceBones[DominantInfluenceIndex]];
				const FString BoneName = RefSkeleton.GetBoneName(BoneIndex).ToString();
				BoneWeightTotals.FindOrAdd(BoneName) += static_cast<double>(DominantWeight);
			}

			TArray<TPair<FString, double>> SortedTotals = BoneWeightTotals.Array();
			SortedTotals.Sort([](const TPair<FString, double>& A, const TPair<FString, double>& B)
			{
				return A.Value > B.Value;
			});

			TArray<FString> TopBones;
			for (int32 Index = 0; Index < FMath::Min(8, SortedTotals.Num()); ++Index)
			{
				TopBones.Add(FString::Printf(TEXT("%s=%0.0f"), *SortedTotals[Index].Key, SortedTotals[Index].Value));
			}

			const FVector SectionCentroid = SectionPositionCount > 0
				? (SectionPositionSum / static_cast<double>(SectionPositionCount))
				: FVector::ZeroVector;

			UE_LOG(LogTemp, Display, TEXT("  ArtificialArm section=%d material=%s centroid=%s dominant bones: %s"), SectionIndex, *MaterialSlotName, *SectionCentroid.ToString(), *FString::Join(TopBones, TEXT(", ")));
		}
	}

	static UAnimSequence* CreateAnimationSequenceAsset(
		const FString& AssetPackagePath,
		USkeleton* Skeleton,
		USkeletalMesh* PreviewMesh,
		TArray<FString>& OutErrors)
	{
		if (!Skeleton)
		{
			OutErrors.Add(FString::Printf(TEXT("animation asset creation requires a skeleton for '%s'"), *AssetPackagePath));
			return nullptr;
		}

		const FString AssetName = FPaths::GetBaseFilename(AssetPackagePath);
		const FString AssetObjectPath = FString::Printf(TEXT("%s.%s"), *AssetPackagePath, *AssetName);
		if (!DeleteAssetIfPresent(AssetObjectPath))
		{
			OutErrors.Add(FString::Printf(TEXT("failed to delete existing animation asset '%s'"), *AssetObjectPath));
			return nullptr;
		}

		UPackage* Package = CreatePackage(*AssetPackagePath);
		if (!Package)
		{
			OutErrors.Add(FString::Printf(TEXT("failed to create animation package '%s'"), *AssetPackagePath));
			return nullptr;
		}

		Package->FullyLoad();

		UAnimSequence* AnimSequence = NewObject<UAnimSequence>(Package, UAnimSequence::StaticClass(), *AssetName, RF_Public | RF_Standalone);
		if (!AnimSequence)
		{
			OutErrors.Add(FString::Printf(TEXT("failed to allocate animation asset '%s'"), *AssetObjectPath));
			return nullptr;
		}

		AnimSequence->SetSkeleton(Skeleton);
		if (PreviewMesh)
		{
			AnimSequence->SetPreviewMesh(PreviewMesh);
		}

		IAnimationDataController& Controller = AnimSequence->GetController();
		Controller.InitializeModel();
		Controller.SetNumberOfFrames(1, false);
		Controller.NotifyPopulated();

		FAssetRegistryModule::AssetCreated(AnimSequence);
		AnimSequence->MarkPackageDirty();
		return AnimSequence;
	}

		static bool RewriteAnimationFromTranslatedSource(const FString& FilePath, UAnimSequence* TargetSequence, TArray<FString>& OutErrors, FImportedAnimationValidationSummary* OutValidationSummary = nullptr)
	{
			if (OutValidationSummary)
			{
				OutValidationSummary->ClipName = FPaths::GetBaseFilename(FilePath);
			}

		if (!TargetSequence || !TargetSequence->GetSkeleton())
		{
			OutErrors.Add(FString::Printf(TEXT("animation rewrite requires a valid target sequence and skeleton for '%s'"), *FPaths::GetCleanFilename(FilePath)));
			return false;
		}

		UInterchangeSourceData* SourceData = UInterchangeManager::CreateSourceData(FilePath);
		if (!SourceData)
		{
			OutErrors.Add(FString::Printf(TEXT("failed to create Interchange source data for '%s'"), *FilePath));
			return false;
		}

		UE::Interchange::FScopedTranslator ScopedTranslator(SourceData);
		UInterchangeTranslatorBase* Translator = ScopedTranslator.GetTranslator();
		if (!Translator)
		{
			OutErrors.Add(FString::Printf(TEXT("failed to create Interchange translator for '%s'"), *FilePath));
			return false;
		}

		UE::Interchange::FScopedBaseNodeContainer ScopedNodeContainer;
		UInterchangeBaseNodeContainer* NodeContainer = ScopedNodeContainer.GetBaseNodeContainer();
		if (!NodeContainer || !Translator->Translate(*NodeContainer))
		{
			OutErrors.Add(FString::Printf(TEXT("Interchange translator failed to translate '%s'"), *FilePath));
			return false;
		}

		FSekiroHumanoidNormalizationData NormalizationData;
		if (!SekiroHumanoidImport::BuildNormalizationData(NodeContainer, NormalizationData, OutErrors))
		{
			return false;
		}

		IInterchangeAnimationPayloadInterface* AnimationPayloadInterface = Cast<IInterchangeAnimationPayloadInterface>(Translator);
		if (!AnimationPayloadInterface)
		{
			OutErrors.Add(FString::Printf(TEXT("translator for '%s' does not expose animation payload data"), *FPaths::GetCleanFilename(FilePath)));
			return false;
		}

		UInterchangeSkeletalAnimationTrackNode* AnimationTrackNode = nullptr;
		NodeContainer->BreakableIterateNodesOfType<UInterchangeSkeletalAnimationTrackNode>(
			[&AnimationTrackNode](const FString&, UInterchangeSkeletalAnimationTrackNode* TrackNode)
			{
				AnimationTrackNode = TrackNode;
				return true;
			});

		if (!AnimationTrackNode)
		{
			OutErrors.Add(FString::Printf(TEXT("translated animation '%s' did not contain a skeletal animation track node"), *FPaths::GetCleanFilename(FilePath)));
			return false;
		}

		TMap<FString, const UInterchangeSceneNode*> SceneNodesByBoneName;
		if (!SekiroHumanoidImport::CollectJointSceneNodes(NodeContainer, SceneNodesByBoneName, OutErrors))
		{
			return false;
		}

		double SampleRate = 0.0;
		double StartTime = 0.0;
		double StopTime = 0.0;
		AnimationTrackNode->GetCustomAnimationSampleRate(SampleRate);
		AnimationTrackNode->GetCustomAnimationStartTime(StartTime);
		AnimationTrackNode->GetCustomAnimationStopTime(StopTime);
		if (SampleRate <= 0.0)
		{
			SampleRate = 30.0;
		}

		TMap<FString, FString> PayloadUidBySceneNode;
		TMap<FString, uint8> PayloadTypeBySceneNode;
		AnimationTrackNode->GetSceneNodeAnimationPayloadKeys(PayloadUidBySceneNode, PayloadTypeBySceneNode);

		TArray<UE::Interchange::FAnimationPayloadQuery> Queries;
		Queries.Reserve(PayloadUidBySceneNode.Num());
		for (const TPair<FString, FString>& Pair : PayloadUidBySceneNode)
		{
			const uint8* PayloadType = PayloadTypeBySceneNode.Find(Pair.Key);
			Queries.Emplace(Pair.Key, FInterchangeAnimationPayLoadKey(Pair.Value, PayloadType ? static_cast<EInterchangeAnimationPayLoadType>(*PayloadType) : EInterchangeAnimationPayLoadType::BAKED), SampleRate, StartTime, StopTime);
		}

		TArray<UE::Interchange::FAnimationPayloadData> Payloads = AnimationPayloadInterface->GetAnimationPayloadData(Queries);
		TMap<FString, UE::Interchange::FAnimationPayloadData> PayloadByNodeUid;
		for (UE::Interchange::FAnimationPayloadData& Payload : Payloads)
		{
			if (Payload.PayloadKey.Type != EInterchangeAnimationPayLoadType::BAKED || Payload.Transforms.Num() == 0)
			{
				FTransform DefaultTransform = FTransform::Identity;
				for (const TPair<FString, const UInterchangeSceneNode*>& BonePair : SceneNodesByBoneName)
				{
					if (BonePair.Value && BonePair.Value->GetUniqueID() == Payload.SceneNodeUniqueID)
					{
						if (const FSekiroHumanoidSceneNodeData* BoneData = NormalizationData.BoneDataByName.Find(BonePair.Key))
						{
							DefaultTransform = BoneData->LocalBindTransform;
						}
						break;
					}
				}
				Payload.CalculateDataFor(EInterchangeAnimationPayLoadType::BAKED, DefaultTransform);
			}

			PayloadByNodeUid.Add(Payload.SceneNodeUniqueID, MoveTemp(Payload));
		}

		int32 TotalKeys = 0;
		for (const TPair<FString, UE::Interchange::FAnimationPayloadData>& Pair : PayloadByNodeUid)
		{
			TotalKeys = FMath::Max(TotalKeys, Pair.Value.Transforms.Num());
		}

		if (TotalKeys <= 1)
		{
			OutErrors.Add(FString::Printf(TEXT("translated animation '%s' did not yield any baked bone keys"), *FPaths::GetCleanFilename(FilePath)));
			return false;
		}

		TMap<FString, FTransform> BindLocalByBone;
		TMap<FString, FString> SourceParentByBone;
		for (const TPair<FString, FSekiroHumanoidSceneNodeData>& Pair : NormalizationData.BoneDataByName)
		{
			BindLocalByBone.Add(Pair.Key, Pair.Value.LocalBindTransform);
			SourceParentByBone.Add(Pair.Key, Pair.Value.ParentBoneName);
		}

		const FReferenceSkeleton& TargetRefSkeleton = TargetSequence->GetSkeleton()->GetReferenceSkeleton();
		TMap<FString, FString> TargetParentByBone;
		for (int32 BoneIndex = 0; BoneIndex < TargetRefSkeleton.GetNum(); ++BoneIndex)
		{
			const FString BoneName = TargetRefSkeleton.GetBoneName(BoneIndex).ToString();
			const int32 ParentIndex = TargetRefSkeleton.GetParentIndex(BoneIndex);
			TargetParentByBone.Add(BoneName, ParentIndex != INDEX_NONE ? TargetRefSkeleton.GetBoneName(ParentIndex).ToString() : FString());
		}

		const FFrameRate FrameRate = FFrameRate(FMath::Max(1, FMath::RoundToInt32(SampleRate)), 1);
		IAnimationDataController& Controller = TargetSequence->GetController();
		Controller.OpenBracket(FText::FromString(TEXT("Rewrite normalized Sekiro animation")), false);
		Controller.SetFrameRate(FrameRate, false);
		Controller.SetNumberOfFrames(FFrameNumber(TotalKeys - 1), false);
		Controller.RemoveAllBoneTracks(false);

			TArray<TArray<FVector3f>> PosKeysByBone;
			TArray<TArray<FQuat4f>> RotKeysByBone;
			TArray<TArray<FVector3f>> ScaleKeysByBone;
			PosKeysByBone.SetNum(TargetRefSkeleton.GetNum());
			RotKeysByBone.SetNum(TargetRefSkeleton.GetNum());
			ScaleKeysByBone.SetNum(TargetRefSkeleton.GetNum());

			TMap<FString, double> MinimumDirectionDots;
			TSet<FString> SeenDirectionMetrics;
			TMap<FString, double> MinimumHandBasisDots;
			TSet<FString> SeenHandBasisMetrics;

			for (int32 FrameIndex = 0; FrameIndex < TotalKeys; ++FrameIndex)
			{
				TMap<FString, FTransform> SourceWorldByBone;
				TMap<FString, FTransform> SourceNormalizedWorldByBone;
				for (const FString& SourceBoneName : NormalizationData.BoneOrder)
				{
					const FString ParentSourceBoneName = SourceParentByBone.FindRef(SourceBoneName);
					FTransform LocalTransform = BindLocalByBone.FindRef(SourceBoneName);
					if (const UInterchangeSceneNode* const* SceneNode = SceneNodesByBoneName.Find(SourceBoneName))
					{
						if (const UE::Interchange::FAnimationPayloadData* Payload = PayloadByNodeUid.Find((*SceneNode)->GetUniqueID()))
						{
							if (Payload->Transforms.IsValidIndex(FrameIndex))
							{
								LocalTransform = Payload->Transforms[FrameIndex];
							}
						}
					}

					const FTransform SourceWorldTransform = (!ParentSourceBoneName.IsEmpty() && SourceWorldByBone.Contains(ParentSourceBoneName))
						? (LocalTransform * SourceWorldByBone[ParentSourceBoneName])
						: LocalTransform;
					SourceWorldByBone.Add(SourceBoneName, SourceWorldTransform);
					SourceNormalizedWorldByBone.Add(SourceBoneName, FTransform(SourceWorldTransform.ToMatrixWithScale() * NormalizationData.GlobalNormalizationMatrix));
				}

				ApplyComponentRotationRebasesToWorldMap(NormalizationData.ComponentRotationRebaseByBone, SourceParentByBone, SourceNormalizedWorldByBone);

				TMap<FString, FTransform> TargetWorldByBone;
				for (int32 BoneIndex = 0; BoneIndex < TargetRefSkeleton.GetNum(); ++BoneIndex)
				{
					const FName BoneName = TargetRefSkeleton.GetBoneName(BoneIndex);
					const FString BoneNameString = BoneName.ToString();
					const FString ParentBoneName = TargetParentByBone.FindRef(BoneNameString);
					FTransform LocalTrackTransform = TargetRefSkeleton.GetRefBonePose()[BoneIndex];

					if (const FTransform* SourceNormalizedWorldTransform = SourceNormalizedWorldByBone.Find(BoneNameString))
					{
						if (!ParentBoneName.IsEmpty() && TargetWorldByBone.Contains(ParentBoneName))
						{
							LocalTrackTransform = FTransform(SourceNormalizedWorldTransform->ToMatrixWithScale() * TargetWorldByBone[ParentBoneName].ToMatrixWithScale().InverseFast());
						}
						else
						{
							LocalTrackTransform = *SourceNormalizedWorldTransform;
						}
					}

					const FTransform TargetWorldTransform = (!ParentBoneName.IsEmpty() && TargetWorldByBone.Contains(ParentBoneName))
						? (LocalTrackTransform * TargetWorldByBone[ParentBoneName])
						: LocalTrackTransform;
					TargetWorldByBone.Add(BoneNameString, TargetWorldTransform);

					PosKeysByBone[BoneIndex].Add(FVector3f(LocalTrackTransform.GetLocation()));
					RotKeysByBone[BoneIndex].Add(FQuat4f(LocalTrackTransform.GetRotation()));
					ScaleKeysByBone[BoneIndex].Add(FVector3f(LocalTrackTransform.GetScale3D()));
				}

				if (OutValidationSummary)
				{
					AccumulateImportedAnimationValidationFrame(
						SourceNormalizedWorldByBone,
						TargetWorldByBone,
						MinimumDirectionDots,
						SeenDirectionMetrics,
						MinimumHandBasisDots,
						SeenHandBasisMetrics);
				}
			}

			for (int32 BoneIndex = 0; BoneIndex < TargetRefSkeleton.GetNum(); ++BoneIndex)
			{
				const FName BoneName = TargetRefSkeleton.GetBoneName(BoneIndex);
				Controller.AddBoneCurve(BoneName, false);
				Controller.SetBoneTrackKeys(BoneName, PosKeysByBone[BoneIndex], RotKeysByBone[BoneIndex], ScaleKeysByBone[BoneIndex], false);
			}

		Controller.CloseBracket(false);
		TargetSequence->MarkPackageDirty();

			if (OutValidationSummary)
			{
				FinalizeImportedAnimationValidation(*OutValidationSummary, MinimumDirectionDots, SeenDirectionMetrics, MinimumHandBasisDots, SeenHandBasisMetrics);
			}
		return true;
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
	TMap<FString, FString> CharacterExportDirs;
	for (const FString& Dir : ChrDirs)
	{
		if (Dir.StartsWith(TEXT("c")) && Dir.Len() >= 4)
		{
			if (ChrFilter.Num() == 0 || ChrFilter.Contains(Dir))
			{
				ValidChrs.Add(Dir);
				CharacterExportDirs.Add(Dir, ExportDir / Dir);
			}
		}
	}

	TSharedPtr<FJsonObject> RootAssetPackage;
	const FString RootAssetPackagePath = ExportDir / TEXT("asset_package.json");
	if (LoadJsonObjectFromFile(RootAssetPackagePath, RootAssetPackage))
	{
		FString RootChrId;
		if (RootAssetPackage->TryGetStringField(TEXT("characterId"), RootChrId)
			&& !RootChrId.IsEmpty()
			&& (ChrFilter.Num() == 0 || ChrFilter.Contains(RootChrId))
			&& !CharacterExportDirs.Contains(RootChrId))
		{
			ValidChrs.Add(RootChrId);
			CharacterExportDirs.Add(RootChrId, ExportDir);
			UE_LOG(LogTemp, Display, TEXT("Detected root-level asset package for %s"), *RootChrId);
		}
	}
	ValidChrs.Sort();

	UE_LOG(LogTemp, Display, TEXT("Found %d character(s) to import"), ValidChrs.Num());

	for (int32 i = 0; i < ValidChrs.Num(); i++)
	{
		const FString& ChrId = ValidChrs[i];
		UE_LOG(LogTemp, Display, TEXT("[%d/%d] Importing %s..."), i + 1, ValidChrs.Num(), *ChrId);
		ImportCharacter(ChrId, CharacterExportDirs.FindRef(ChrId), ContentBase, AnimLimit, AnimFilters, bImportAnimationsOnly, bImportModelOnly);

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
	TArray<FString> Warnings;
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
	const bool bHasSkeletalImportModelInputs = bHasTextures && bHasModel && bHasMaterialManifest;
	const bool bHasSkeletalImportAnimationInputs = bHasAnimations;
	const bool bHasSkeletalImportFullInputs = bHasSkeletalImportModelInputs && bHasSkeletalImportAnimationInputs;

	if (!bFormalOnly)
	{
		UE_LOG(LogTemp, Error, TEXT("  Asset package is not a formal-only export: %s"), *AssetPackagePath);
		return;
	}

	if (!bFormalSuccess)
	{
		if (bImportModelOnly)
		{
			if (!bHasSkeletalImportModelInputs)
			{
				UE_LOG(LogTemp, Error, TEXT("  Model-only import requires ready model, textures, and material manifest deliverables: %s"), *AssetPackagePath);
				return;
			}

			UE_LOG(LogTemp, Warning, TEXT("  Asset package formalSuccess=false; proceeding with model-only import because required model deliverables are present."));
		}
		else if (bImportAnimationsOnly)
		{
			if (!bHasSkeletalImportAnimationInputs)
			{
				UE_LOG(LogTemp, Error, TEXT("  Animations-only import requires ready animation deliverables: %s"), *AssetPackagePath);
				return;
			}

			UE_LOG(LogTemp, Warning, TEXT("  Asset package formalSuccess=false; proceeding with animations-only import because animation deliverables are present."));
		}
		else if (bHasSkeletalImportFullInputs)
		{
			UE_LOG(LogTemp, Warning, TEXT("  Asset package formalSuccess=false due to unrelated required deliverables, but full skeletal import inputs are present; proceeding with model+animation import."));
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
			Result = ImportViaAssetTask(ModelFile, MeshContentPath, nullptr, true, true, &Errors);
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
				LogSectionDominantBonesByMaterial(SkelMesh);
				ValidateNormalizedHumanoidAssets(Skeleton, SkelMesh, Errors);
				bVisiblePoseValidated = true;
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
				LogSectionDominantBonesByMaterial(ImportedSkelMesh);
				ValidateNormalizedHumanoidAssets(Skeleton, ImportedSkelMesh, Errors);
				bVisiblePoseValidated = true;
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

	// 3. Import animations by creating a destination sequence and rewriting tracks from translated source payloads
	int32 RootMotionEnabledCount = 0;
	TArray<FString> RootMotionFailedClips;
	TArray<FImportedAnimationValidationSummary> AnimationValidationSummaries;
	bool bAnimationBranchBasisValidated = false;
	if (!bImportModelOnly && Skeleton)
	{
		USkeletalMesh* AnimationPreviewMesh = ImportedSkeletalMesh ? ImportedSkeletalMesh : FindSkeletalMeshInPackage(MeshContentPath);
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
			const FString AnimAssetPackagePath = AnimDest / AnimName;

			UAnimSequence* AnimSeq = CreateAnimationSequenceAsset(AnimAssetPackagePath, Skeleton, AnimationPreviewMesh, Errors);
			if (AnimSeq)
			{
				FImportedAnimationValidationSummary ValidationSummary;
				if (!RewriteAnimationFromTranslatedSource(AnimFile, AnimSeq, Errors, &ValidationSummary))
				{
					Errors.Add(FString::Printf(TEXT("animation normalization rewrite failed: %s"), *FPaths::GetCleanFilename(AnimFile)));
					continue;
				}

				ValidationSummary.ClipName = AnimName;
				AnimationValidationSummaries.Add(ValidationSummary);
				bAnimationBranchBasisValidated = true;
				if (!ValidationSummary.bSuccess)
				{
					for (const FString& Issue : ValidationSummary.Issues)
					{
						Errors.Add(FString::Printf(TEXT("animation branch basis validation failed for '%s': %s"), *AnimName, *Issue));
					}
				}

				AnimSeq->bEnableRootMotion = true;
				AnimSeq->bForceRootLock = false;
				AnimSeq->MarkPackageDirty();
				SavePackage(AnimSeq);
				AnimCount++;
				RootMotionEnabledCount++;
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
				Warnings.Add(TEXT("skill import returned a null skill asset"));
				continue;
			}

			if (SkillAsset->Animation.IsNull())
			{
				Warnings.Add(FString::Printf(TEXT("skill asset '%s' is missing imported animation '%s'"), *SkillAsset->GetName(), *SkillAsset->SourceFileName));
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
			Warnings.Add(TEXT("failed to load CharacterData asset after skill import"));
		}
	}
	else if (!bImportAnimationsOnly && !bImportModelOnly)
	{
		Warnings.Add(TEXT("missing skills deliverable"));
	}

	TSharedPtr<FJsonObject> ReportObject = MakeShared<FJsonObject>();
	ReportObject->SetStringField(TEXT("schemaVersion"), TEXT("2.1"));
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

	TSharedPtr<FJsonObject> PostImportChecksObject = MakeShared<FJsonObject>();
	PostImportChecksObject->SetBoolField(TEXT("animationBranchBasisValidated"), bAnimationBranchBasisValidated);
	PostImportChecksObject->SetBoolField(TEXT("rtgDiagnosticsRequiredForSuccess"), false);
	PostImportChecksObject->SetStringField(TEXT("rtgDiagnosticsRole"), TEXT("post-import-only"));
	ReportObject->SetObjectField(TEXT("postImportChecks"), PostImportChecksObject);

	TSharedPtr<FJsonObject> AnimationValidationObject = MakeShared<FJsonObject>();
	TSharedPtr<FJsonObject> DirectionThresholdsObject = MakeShared<FJsonObject>();
	for (const TPair<FString, double>& Threshold : GetImportAnimationDirectionThresholds())
	{
		DirectionThresholdsObject->SetNumberField(Threshold.Key, Threshold.Value);
	}
	AnimationValidationObject->SetObjectField(TEXT("directionThresholds"), DirectionThresholdsObject);

	TSharedPtr<FJsonObject> HandBasisThresholdsObject = MakeShared<FJsonObject>();
	for (const TPair<FString, double>& Threshold : GetImportAnimationHandBasisThresholds())
	{
		HandBasisThresholdsObject->SetNumberField(Threshold.Key, Threshold.Value);
	}
	AnimationValidationObject->SetObjectField(TEXT("handBasisThresholds"), HandBasisThresholdsObject);

	TArray<TSharedPtr<FJsonValue>> AnimationValidationClipValues;
	for (const FImportedAnimationValidationSummary& ValidationSummary : AnimationValidationSummaries)
	{
		TSharedPtr<FJsonObject> ClipObject = MakeShared<FJsonObject>();
		ClipObject->SetStringField(TEXT("clip"), ValidationSummary.ClipName);
		ClipObject->SetBoolField(TEXT("success"), ValidationSummary.bSuccess);

		TSharedPtr<FJsonObject> MinimumDirectionDotsObject = MakeShared<FJsonObject>();
		for (const TPair<FString, double>& Metric : ValidationSummary.MinimumDirectionDots)
		{
			MinimumDirectionDotsObject->SetNumberField(Metric.Key, Metric.Value);
		}
		ClipObject->SetObjectField(TEXT("minimumDirectionDots"), MinimumDirectionDotsObject);

		TSharedPtr<FJsonObject> MinimumHandBasisDotsObject = MakeShared<FJsonObject>();
		for (const TPair<FString, double>& Metric : ValidationSummary.MinimumHandBasisDots)
		{
			MinimumHandBasisDotsObject->SetNumberField(Metric.Key, Metric.Value);
		}
		ClipObject->SetObjectField(TEXT("minimumHandBasisDots"), MinimumHandBasisDotsObject);

		TArray<TSharedPtr<FJsonValue>> ClipIssueValues;
		for (const FString& Issue : ValidationSummary.Issues)
		{
			ClipIssueValues.Add(MakeShared<FJsonValueString>(Issue));
		}
		ClipObject->SetArrayField(TEXT("issues"), ClipIssueValues);
		AnimationValidationClipValues.Add(MakeShared<FJsonValueObject>(ClipObject));
	}
	AnimationValidationObject->SetArrayField(TEXT("clips"), AnimationValidationClipValues);
	ReportObject->SetObjectField(TEXT("animationValidation"), AnimationValidationObject);

	TArray<TSharedPtr<FJsonValue>> ErrorValues;
	for (const FString& Error : Errors)
	{
		ErrorValues.Add(MakeShared<FJsonValueString>(Error));
	}
	ReportObject->SetArrayField(TEXT("errors"), ErrorValues);

	TArray<TSharedPtr<FJsonValue>> WarningValues;
	for (const FString& Warning : Warnings)
	{
		WarningValues.Add(MakeShared<FJsonValueString>(Warning));
	}
	ReportObject->SetArrayField(TEXT("warnings"), WarningValues);
	ReportObject->SetBoolField(TEXT("success"), Errors.Num() == 0);
	FString ReportText;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ReportText);
	FJsonSerializer::Serialize(ReportObject.ToSharedRef(), Writer);
	FFileHelper::SaveStringToFile(ReportText, *(ExportDir / TEXT("ue_import_report.json")));
}

UObject* USekiroImportCommandlet::ImportViaAssetTask(const FString& FilePath, const FString& DestPackagePath, USkeleton* SkeletonOverride, bool bSaveAssetsInDestinationPath, bool bUseHumanoidPipeline, TArray<FString>* OutPipelineErrors)
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

	if (bUseHumanoidPipeline)
	{
		USekiroHumanoidImportPipeline* Pipeline = NewObject<USekiroHumanoidImportPipeline>(GetTransientPackage());
		ImportParams.OverridePipelines.Add(FSoftObjectPath(Pipeline));

		if (OutPipelineErrors)
		{
			OutPipelineErrors->Append(Pipeline->GetNormalizationErrors());
		}
	}
	else if (SkeletonOverride)
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

	if (bUseHumanoidPipeline && OutPipelineErrors)
	{
		for (const FSoftObjectPath& PipelinePath : ImportParams.OverridePipelines)
		{
			if (USekiroHumanoidImportPipeline* Pipeline = Cast<USekiroHumanoidImportPipeline>(PipelinePath.ResolveObject()))
			{
				OutPipelineErrors->Append(Pipeline->GetNormalizationErrors());
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
