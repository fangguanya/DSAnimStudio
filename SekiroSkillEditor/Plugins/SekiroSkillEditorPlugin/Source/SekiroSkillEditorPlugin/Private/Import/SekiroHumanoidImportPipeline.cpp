#include "Import/SekiroHumanoidImportPipeline.h"

#include "InterchangeMeshNode.h"
#include "InterchangeSceneNode.h"
#include "Nodes/InterchangeBaseNodeContainer.h"
#include "Math/RotationMatrix.h"

namespace
{
	static bool IsJointSceneNode(const UInterchangeSceneNode* SceneNode)
	{
		return SceneNode != nullptr
			&& SceneNode->IsSpecializedTypeContains(UE::Interchange::FSceneNodeStaticData::GetJointSpecializeTypeString());
	}

	static FTransform GetBestLocalBindTransform(const UInterchangeSceneNode* SceneNode)
	{
		FTransform Transform = FTransform::Identity;
		if (SceneNode->GetCustomBindPoseLocalTransform(Transform))
		{
			return Transform;
		}

		if (SceneNode->GetCustomTimeZeroLocalTransform(Transform))
		{
			return Transform;
		}

		SceneNode->GetCustomLocalTransform(Transform);
		return Transform;
	}

	static FTransform GetBestGlobalBindTransform(const UInterchangeBaseNodeContainer* BaseNodeContainer, const UInterchangeSceneNode* SceneNode)
	{
		FTransform Transform = FTransform::Identity;
		if (SceneNode->GetCustomBindPoseGlobalTransform(BaseNodeContainer, FTransform::Identity, Transform, true))
		{
			return Transform;
		}

		if (SceneNode->GetCustomTimeZeroGlobalTransform(BaseNodeContainer, FTransform::Identity, Transform, true))
		{
			return Transform;
		}

		SceneNode->GetCustomGlobalTransform(BaseNodeContainer, FTransform::Identity, Transform, true);
		return Transform;
	}

	static FString ResolveBoneName(const UInterchangeSceneNode* SceneNode)
	{
		return SceneNode ? SceneNode->GetDisplayLabel() : FString();
	}

	static FString ResolveExistingBone(const TSet<FString>& BoneNames, std::initializer_list<const TCHAR*> Candidates)
	{
		for (const TCHAR* Candidate : Candidates)
		{
			if (BoneNames.Contains(Candidate))
			{
				return Candidate;
			}
		}

		return FString();
	}

	static FVector ResolveWorldLocation(const TMap<FString, FSekiroHumanoidSceneNodeData>& BoneDataByName, std::initializer_list<const TCHAR*> Candidates)
	{
		for (const TCHAR* Candidate : Candidates)
		{
			if (const FSekiroHumanoidSceneNodeData* BoneData = BoneDataByName.Find(Candidate))
			{
				return BoneData->GlobalBindTransform.GetLocation();
			}
		}

		return FVector::ZeroVector;
	}

	static bool BuildGlobalNormalizationMatrix(const TMap<FString, FSekiroHumanoidSceneNodeData>& BoneDataByName, FMatrix& OutMatrix, TArray<FString>& OutErrors)
	{
		const FVector Left = ResolveWorldLocation(BoneDataByName, { TEXT("L_Hand"), TEXT("L_Shoulder"), TEXT("L_UpperArm") });
		const FVector Right = ResolveWorldLocation(BoneDataByName, { TEXT("R_Hand"), TEXT("R_Shoulder"), TEXT("R_UpperArm") });
		const FVector Pelvis = ResolveWorldLocation(BoneDataByName, { TEXT("Pelvis") });
		const FVector Head = ResolveWorldLocation(BoneDataByName, { TEXT("Head"), TEXT("Neck") });

		FVector LateralAxis = Left - Right;
		if (!LateralAxis.Normalize())
		{
			OutErrors.Add(TEXT("unable to analyze source lateral axis from hand or shoulder bind positions"));
			return false;
		}

		FVector UpAxis = Head - Pelvis;
		if (!UpAxis.Normalize())
		{
			UpAxis = FVector::UpVector;
		}

		const FMatrix SourceBasis = FRotationMatrix::MakeFromXZ(LateralAxis, UpAxis);
		const FMatrix BasisCorrection = SourceBasis.InverseFast();
		const FScaleMatrix MirrorX(FVector(-1.0, 1.0, 1.0));
		const FMatrix ObjectSpaceYaw180 = FRotationMatrix(FRotator(0.0, 180.0, 0.0));
		OutMatrix = BasisCorrection * MirrorX * ObjectSpaceYaw180;
		auto MatrixRowToString = [&OutMatrix](int32 RowIndex)
		{
			return FString::Printf(TEXT("X=%0.6f Y=%0.6f Z=%0.6f W=%0.6f"), OutMatrix.M[RowIndex][0], OutMatrix.M[RowIndex][1], OutMatrix.M[RowIndex][2], OutMatrix.M[RowIndex][3]);
		};
		UE_LOG(LogTemp, Display, TEXT("SekiroHumanoidImport: source bind Left=%s Right=%s Pelvis=%s Head=%s LateralAxis=%s UpAxis=%s"),
			*Left.ToString(), *Right.ToString(), *Pelvis.ToString(), *Head.ToString(), *LateralAxis.ToString(), *UpAxis.ToString());
		UE_LOG(LogTemp, Display, TEXT("SekiroHumanoidImport: appended final object-space yaw correction of 180 degrees around UE Z"));
		UE_LOG(LogTemp, Display, TEXT("SekiroHumanoidImport: normalization matrix rows=[%s] [%s] [%s] [%s]"),
			*MatrixRowToString(0),
			*MatrixRowToString(1),
			*MatrixRowToString(2),
			*MatrixRowToString(3));
		return true;
	}

	static void ResolveTargetParents(const TSet<FString>& BoneNames, TMap<FString, FString>& OutTargetParentByBone)
	{
		const FString RootBone = ResolveExistingBone(BoneNames, { TEXT("Master"), TEXT("Root"), TEXT("root") });
		const FString PelvisParent = !RootBone.IsEmpty() ? RootBone : FString();
		const FString SpineAttachment = ResolveExistingBone(BoneNames, { TEXT("UpperChest"), TEXT("Chest"), TEXT("Spine2"), TEXT("Spine1"), TEXT("Spine") });

		auto SetParent = [&OutTargetParentByBone, &BoneNames](const TCHAR* BoneName, const FString& ParentName)
		{
			if (BoneNames.Contains(BoneName))
			{
				OutTargetParentByBone.Add(BoneName, ParentName);
			}
		};

		SetParent(TEXT("Pelvis"), PelvisParent);
		SetParent(TEXT("Spine"), ResolveExistingBone(BoneNames, { TEXT("Pelvis") }));
		SetParent(TEXT("Spine1"), ResolveExistingBone(BoneNames, { TEXT("Spine") }));
		SetParent(TEXT("Spine2"), ResolveExistingBone(BoneNames, { TEXT("Spine1"), TEXT("Spine") }));
		SetParent(TEXT("Chest"), ResolveExistingBone(BoneNames, { TEXT("Spine2"), TEXT("Spine1"), TEXT("Spine") }));
		SetParent(TEXT("UpperChest"), ResolveExistingBone(BoneNames, { TEXT("Chest"), TEXT("Spine2"), TEXT("Spine1") }));
		SetParent(TEXT("Neck"), ResolveExistingBone(BoneNames, { TEXT("UpperChest"), TEXT("Chest"), TEXT("Spine2"), TEXT("Spine1") }));
		SetParent(TEXT("Head"), ResolveExistingBone(BoneNames, { TEXT("Neck") }));
		SetParent(TEXT("L_Clavicle"), SpineAttachment);
		SetParent(TEXT("R_Clavicle"), SpineAttachment);
		SetParent(TEXT("L_Shoulder"), ResolveExistingBone(BoneNames, { TEXT("L_Clavicle") }));
		SetParent(TEXT("R_Shoulder"), ResolveExistingBone(BoneNames, { TEXT("R_Clavicle") }));
		SetParent(TEXT("L_UpperArm"), ResolveExistingBone(BoneNames, { TEXT("L_Clavicle"), TEXT("L_Shoulder") }));
		SetParent(TEXT("R_UpperArm"), ResolveExistingBone(BoneNames, { TEXT("R_Clavicle"), TEXT("R_Shoulder") }));
		SetParent(TEXT("L_Forearm"), ResolveExistingBone(BoneNames, { TEXT("L_UpperArm") }));
		SetParent(TEXT("R_Forearm"), ResolveExistingBone(BoneNames, { TEXT("R_UpperArm") }));
		SetParent(TEXT("L_Hand"), ResolveExistingBone(BoneNames, { TEXT("L_Forearm") }));
		SetParent(TEXT("R_Hand"), ResolveExistingBone(BoneNames, { TEXT("R_Forearm") }));
		SetParent(TEXT("L_Thigh"), ResolveExistingBone(BoneNames, { TEXT("Pelvis") }));
		SetParent(TEXT("R_Thigh"), ResolveExistingBone(BoneNames, { TEXT("Pelvis") }));
		SetParent(TEXT("L_Knee"), ResolveExistingBone(BoneNames, { TEXT("L_Thigh") }));
		SetParent(TEXT("R_Knee"), ResolveExistingBone(BoneNames, { TEXT("R_Thigh") }));
		SetParent(TEXT("L_Calf"), ResolveExistingBone(BoneNames, { TEXT("L_Thigh"), TEXT("L_Knee") }));
		SetParent(TEXT("R_Calf"), ResolveExistingBone(BoneNames, { TEXT("R_Thigh"), TEXT("R_Knee") }));
		SetParent(TEXT("L_Foot"), ResolveExistingBone(BoneNames, { TEXT("L_Calf"), TEXT("L_Knee"), TEXT("L_Thigh") }));
		SetParent(TEXT("R_Foot"), ResolveExistingBone(BoneNames, { TEXT("R_Calf"), TEXT("R_Knee"), TEXT("R_Thigh") }));
		SetParent(TEXT("L_Toe0"), ResolveExistingBone(BoneNames, { TEXT("L_Foot") }));
		SetParent(TEXT("R_Toe0"), ResolveExistingBone(BoneNames, { TEXT("R_Foot") }));
		SetParent(TEXT("face_root"), ResolveExistingBone(BoneNames, { TEXT("Head"), TEXT("Neck") }));
	}

	static FMatrix ComputeNormalizedWorldMatrix(
		const FString& BoneName,
		const FSekiroHumanoidNormalizationData& Data,
		TMap<FString, FMatrix>& InOutWorldMatrices)
	{
		if (const FMatrix* Existing = InOutWorldMatrices.Find(BoneName))
		{
			return *Existing;
		}

		const FSekiroHumanoidSceneNodeData& BoneData = Data.BoneDataByName[BoneName];
		const FMatrix SourceWorldMatrix = BoneData.GlobalBindTransform.ToMatrixWithScale();
		const FMatrix NormalizedWorldMatrix = SourceWorldMatrix * Data.GlobalNormalizationMatrix;
		InOutWorldMatrices.Add(BoneName, NormalizedWorldMatrix);
		return NormalizedWorldMatrix;
	}

	static bool TryGetNormalizedWorldLocation(const FSekiroHumanoidNormalizationData& Data, const FString& BoneName, FVector& OutLocation)
	{
		if (const FSekiroHumanoidSceneNodeData* BoneData = Data.BoneDataByName.Find(BoneName))
		{
			OutLocation = BoneData->NormalizedGlobalBindTransform.GetLocation();
			return true;
		}

		return false;
	}

	static void BuildChildBonesByParent(const TMap<FString, FSekiroHumanoidSceneNodeData>& BoneDataByName, TMap<FString, TArray<FString>>& OutChildBonesByParent)
	{
		OutChildBonesByParent.Reset();
		for (const TPair<FString, FSekiroHumanoidSceneNodeData>& Pair : BoneDataByName)
		{
			const FString& ParentBoneName = Pair.Value.ParentBoneName;
			if (!ParentBoneName.IsEmpty())
			{
				OutChildBonesByParent.FindOrAdd(ParentBoneName).Add(Pair.Key);
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

	static void ApplyComponentRotationRebasesToWorldMap(const FSekiroHumanoidNormalizationData& Data, TMap<FString, FTransform>& InOutWorldByBone)
	{
		TMap<FString, TArray<FString>> ChildBonesByParent;
		BuildChildBonesByParent(Data.BoneDataByName, ChildBonesByParent);
		for (const TPair<FString, FQuat>& Pair : Data.ComponentRotationRebaseByBone)
		{
			ApplyComponentRotationRebaseToSubtree(Pair.Key, Pair.Value, ChildBonesByParent, InOutWorldByBone);
		}
	}

	static void RebuildNormalizedLocalAndGlobalFromWorldMap(FSekiroHumanoidNormalizationData& Data, const TMap<FString, FTransform>& WorldByBone, TArray<FString>& OutErrors)
	{
		for (const FString& BoneName : Data.BoneOrder)
		{
			FSekiroHumanoidSceneNodeData* BoneData = Data.BoneDataByName.Find(BoneName);
			if (!BoneData)
			{
				continue;
			}

			const FTransform* BoneWorldTransform = WorldByBone.Find(BoneName);
			if (!BoneWorldTransform)
			{
				OutErrors.Add(FString::Printf(TEXT("normalized world transform for bone '%s' was missing during bind reconstruction"), *BoneName));
				return;
			}

			const FString SelectedParentName = Data.TargetParentByBone.Contains(BoneName)
				? Data.TargetParentByBone[BoneName]
				: BoneData->ParentBoneName;
			BoneData->NormalizedParentBoneName = SelectedParentName;

			FTransform LocalTransform = *BoneWorldTransform;
			if (!SelectedParentName.IsEmpty())
			{
				const FTransform* ParentWorldTransform = WorldByBone.Find(SelectedParentName);
				if (!ParentWorldTransform)
				{
					OutErrors.Add(FString::Printf(TEXT("normalized bind pose parent '%s' for bone '%s' was not present during bind reconstruction"), *SelectedParentName, *BoneName));
					return;
				}

				LocalTransform = FTransform(BoneWorldTransform->ToMatrixWithScale() * ParentWorldTransform->ToMatrixWithScale().InverseFast());
			}

			BoneData->NormalizedGlobalBindTransform = *BoneWorldTransform;
			BoneData->NormalizedLocalBindTransform = LocalTransform;
		}
	}

	static void BuildHandComponentRotationRebases(FSekiroHumanoidNormalizationData& Data)
	{
		struct FHandRebaseSpec
		{
			const TCHAR* HandBone;
			const TCHAR* IndexBone;
			const TCHAR* MiddleBone;
			const TCHAR* PinkyBone;
			bool bFlipForward;
			bool bFlipLateral;
		};

		static const FHandRebaseSpec Specs[] = {
			{ TEXT("L_Hand"), TEXT("L_Finger1"), TEXT("L_Finger2"), TEXT("L_Finger4"), false, true },
			{ TEXT("R_Hand"), TEXT("R_Finger1"), TEXT("R_Finger2"), TEXT("R_Finger4"), false, false },
		};

		Data.ComponentRotationRebaseByBone.Reset();
		for (const FHandRebaseSpec& Spec : Specs)
		{
			FVector HandLocation = FVector::ZeroVector;
			FVector IndexLocation = FVector::ZeroVector;
			FVector MiddleLocation = FVector::ZeroVector;
			FVector PinkyLocation = FVector::ZeroVector;
			if (!TryGetNormalizedWorldLocation(Data, Spec.HandBone, HandLocation)
				|| !TryGetNormalizedWorldLocation(Data, Spec.IndexBone, IndexLocation)
				|| !TryGetNormalizedWorldLocation(Data, Spec.MiddleBone, MiddleLocation)
				|| !TryGetNormalizedWorldLocation(Data, Spec.PinkyBone, PinkyLocation))
			{
				continue;
			}

			FVector PalmForward = (MiddleLocation - HandLocation).GetSafeNormal();
			const FVector PalmLateralSeed = (IndexLocation - PinkyLocation).GetSafeNormal();
			const FVector PalmNormal = FVector::CrossProduct(PalmForward, PalmLateralSeed).GetSafeNormal();
			FVector PalmLateral = FVector::CrossProduct(PalmNormal, PalmForward).GetSafeNormal();
			if (Spec.bFlipForward)
			{
				PalmForward *= -1.0;
			}
			if (Spec.bFlipLateral)
			{
				PalmLateral *= -1.0;
			}
			if (PalmForward.IsNearlyZero() || PalmLateral.IsNearlyZero() || PalmNormal.IsNearlyZero())
			{
				continue;
			}

			const FQuat DesiredComponentRotation = FRotationMatrix::MakeFromXY(PalmForward, PalmLateral).ToQuat();
			if (FSekiroHumanoidSceneNodeData* HandBoneData = Data.BoneDataByName.Find(Spec.HandBone))
			{
				const FQuat CurrentComponentRotation = HandBoneData->NormalizedGlobalBindTransform.GetRotation();
				const FQuat ComponentRotationRebase = (DesiredComponentRotation * CurrentComponentRotation.Inverse()).GetNormalized();
				Data.ComponentRotationRebaseByBone.Add(Spec.HandBone, ComponentRotationRebase);
			}
		}
	}

	static bool BuildNormalizedBindPoseData(FSekiroHumanoidNormalizationData& Data, TArray<FString>& OutErrors)
	{
		TMap<FString, FMatrix> NormalizedWorldMatrices;
		for (const FString& BoneName : Data.BoneOrder)
		{
			if (!Data.BoneDataByName.Contains(BoneName))
			{
				continue;
			}

			ComputeNormalizedWorldMatrix(BoneName, Data, NormalizedWorldMatrices);
		}

		TMap<FString, FTransform> NormalizedWorldByBone;
		for (const FString& BoneName : Data.BoneOrder)
		{
			if (const FMatrix* BoneWorldMatrix = NormalizedWorldMatrices.Find(BoneName))
			{
				NormalizedWorldByBone.Add(BoneName, FTransform(*BoneWorldMatrix));
			}
		}

		const int32 ErrorCountBefore = OutErrors.Num();
		RebuildNormalizedLocalAndGlobalFromWorldMap(Data, NormalizedWorldByBone, OutErrors);
		if (OutErrors.Num() > ErrorCountBefore)
		{
			return false;
		}

		BuildHandComponentRotationRebases(Data);
		ApplyComponentRotationRebasesToWorldMap(Data, NormalizedWorldByBone);
		const int32 ErrorCountBeforeSecond = OutErrors.Num();
		RebuildNormalizedLocalAndGlobalFromWorldMap(Data, NormalizedWorldByBone, OutErrors);
		if (OutErrors.Num() > ErrorCountBeforeSecond)
		{
			return false;
		}

		Data.NormalizedMeshBindTransformByMeshUid.Reset();
		for (const TPair<FString, FTransform>& Pair : Data.MeshBindTransformByMeshUid)
		{
			const FMatrix NormalizedMeshBindMatrix = Pair.Value.ToMatrixWithScale() * Data.GlobalNormalizationMatrix;
			Data.NormalizedMeshBindTransformByMeshUid.Add(Pair.Key, FTransform(NormalizedMeshBindMatrix));
		}

		return true;
	}

	static void ApplyNormalizedHierarchy(UInterchangeBaseNodeContainer* BaseNodeContainer, const FSekiroHumanoidNormalizationData& Data, TArray<FString>& OutErrors)
	{
		TMap<FString, const UInterchangeSceneNode*> SceneNodesByBoneName;
		if (!SekiroHumanoidImport::CollectJointSceneNodes(BaseNodeContainer, SceneNodesByBoneName, OutErrors))
		{
			return;
		}

		TMap<FString, FMatrix> NormalizedWorldMatrices;
		TMap<FString, FMatrix> MeshBindReferenceByUid;
		for (const FString& MeshUid : Data.SkinnedMeshAssetUids)
		{
			if (const FTransform* MeshBindTransform = Data.NormalizedMeshBindTransformByMeshUid.Find(MeshUid))
			{
				MeshBindReferenceByUid.Add(MeshUid, MeshBindTransform->ToMatrixWithScale());
			}
		}

		int32 ExplicitlyReparentedBoneCount = 0;
		int32 PreservedOriginalParentBoneCount = 0;
		int32 RootBoneCount = 0;
		for (const FString& BoneName : Data.BoneOrder)
		{
			if (!Data.BoneDataByName.Contains(BoneName))
			{
				continue;
			}

			UInterchangeSceneNode* SceneNode = const_cast<UInterchangeSceneNode*>(SceneNodesByBoneName.FindRef(BoneName));
			if (!SceneNode)
			{
				continue;
			}

			const FSekiroHumanoidSceneNodeData& BoneData = Data.BoneDataByName[BoneName];
			const FMatrix BoneWorldMatrix = BoneData.NormalizedGlobalBindTransform.ToMatrixWithScale();
			FString SelectedParentName;
			bool bParentExplicitlyRewritten = false;
			if (const FString* TargetParentName = Data.TargetParentByBone.Find(BoneName))
			{
				SelectedParentName = *TargetParentName;
				bParentExplicitlyRewritten = true;
			}
			else
			{
				SelectedParentName = BoneData.ParentBoneName;
			}

			FMatrix LocalMatrix = BoneData.NormalizedLocalBindTransform.ToMatrixWithScale();

			if (!SelectedParentName.IsEmpty())
			{
				const UInterchangeSceneNode* ParentSceneNode = SceneNodesByBoneName.FindRef(SelectedParentName);
				if (ParentSceneNode)
				{
					BaseNodeContainer->SetNodeParentUid(SceneNode->GetUniqueID(), ParentSceneNode->GetUniqueID());
					if (bParentExplicitlyRewritten)
					{
						++ExplicitlyReparentedBoneCount;
					}
					else
					{
						++PreservedOriginalParentBoneCount;
					}
				}
				else
				{
					OutErrors.Add(FString::Printf(TEXT("normalized parent '%s' for bone '%s' was not present in the translated scene"), *SelectedParentName, *BoneName));
					BaseNodeContainer->ClearNodeParentUid(SceneNode->GetUniqueID());
					++RootBoneCount;
				}
			}
			else
			{
				BaseNodeContainer->ClearNodeParentUid(SceneNode->GetUniqueID());
				++RootBoneCount;
			}

			const FTransform LocalTransform(LocalMatrix);
			SceneNode->SetDisplayLabel(BoneName);
			SceneNode->SetCustomLocalTransform(BaseNodeContainer, LocalTransform, false);
			SceneNode->SetCustomBindPoseLocalTransform(BaseNodeContainer, LocalTransform, false);
			SceneNode->SetCustomTimeZeroLocalTransform(BaseNodeContainer, LocalTransform, false);
			SceneNode->SetCustomHasBindPose(true);
			SceneNode->SetCustomGlobalMatrixForT0Rebinding(BoneWorldMatrix);
			SceneNode->SetGlobalBindPoseReferenceForMeshUIDs(MeshBindReferenceByUid);

			for (const FString& MeshUid : Data.SkinnedMeshAssetUids)
			{
				const FString AttributeKey = TEXT("JointBindPosePerMesh_") + MeshUid;
				SceneNode->RegisterAttribute<FMatrix>(UE::Interchange::FAttributeKey(AttributeKey), BoneWorldMatrix);
			}
		}

		UE_LOG(LogTemp, Display, TEXT("SekiroHumanoidImport: normalized %d bones (explicit reparent=%d, preserved original parent=%d, roots=%d, skinned meshes=%d)"), Data.BoneOrder.Num(), ExplicitlyReparentedBoneCount, PreservedOriginalParentBoneCount, RootBoneCount, Data.SkinnedMeshAssetUids.Num());

		UInterchangeSceneNode::ResetAllGlobalTransformCaches(BaseNodeContainer);
		BaseNodeContainer->ComputeChildrenCache();
	}
}

bool SekiroHumanoidImport::CollectJointSceneNodes(
	const UInterchangeBaseNodeContainer* BaseNodeContainer,
	TMap<FString, const UInterchangeSceneNode*>& OutNodesByBoneName,
	TArray<FString>& OutErrors)
{
	OutNodesByBoneName.Reset();
	if (!BaseNodeContainer)
	{
		OutErrors.Add(TEXT("interchange node container was null during humanoid normalization"));
		return false;
	}

	BaseNodeContainer->IterateNodesOfType<UInterchangeSceneNode>(
		[&OutNodesByBoneName, &OutErrors](const FString&, UInterchangeSceneNode* SceneNode)
		{
			if (!IsJointSceneNode(SceneNode))
			{
				return;
			}

			const FString BoneName = ResolveBoneName(SceneNode);
			if (BoneName.IsEmpty())
			{
				OutErrors.Add(FString::Printf(TEXT("joint scene node '%s' is missing a display label"), *SceneNode->GetUniqueID()));
				return;
			}

			OutNodesByBoneName.Add(BoneName, SceneNode);
		});

	if (OutNodesByBoneName.Num() == 0)
	{
		OutErrors.Add(TEXT("translated source contained no joint scene nodes"));
		return false;
	}

	return true;
}

bool SekiroHumanoidImport::BuildNormalizationData(
	const UInterchangeBaseNodeContainer* BaseNodeContainer,
	FSekiroHumanoidNormalizationData& OutData,
	TArray<FString>& OutErrors)
{
	OutData = FSekiroHumanoidNormalizationData();

	TMap<FString, const UInterchangeSceneNode*> SceneNodesByBoneName;
	if (!CollectJointSceneNodes(BaseNodeContainer, SceneNodesByBoneName, OutErrors))
	{
		return false;
	}

	TSet<FString> BoneNames;
	for (const TPair<FString, const UInterchangeSceneNode*>& Pair : SceneNodesByBoneName)
	{
		const UInterchangeSceneNode* SceneNode = Pair.Value;
		FSekiroHumanoidSceneNodeData BoneData;
		BoneData.NodeUid = SceneNode->GetUniqueID();
		BoneData.BoneName = Pair.Key;
		BoneData.ParentNodeUid = SceneNode->GetParentUid();
		BoneData.LocalBindTransform = GetBestLocalBindTransform(SceneNode);
		BoneData.GlobalBindTransform = GetBestGlobalBindTransform(BaseNodeContainer, SceneNode);

		if (const FString* ParentBoneName = OutData.BoneNameByNodeUid.Find(BoneData.ParentNodeUid))
		{
			BoneData.ParentBoneName = *ParentBoneName;
		}

		OutData.BoneOrder.Add(BoneData.BoneName);
		OutData.BoneNameByNodeUid.Add(BoneData.NodeUid, BoneData.BoneName);
		OutData.BoneDataByName.Add(BoneData.BoneName, BoneData);
		BoneNames.Add(BoneData.BoneName);
	}

	for (TPair<FString, FSekiroHumanoidSceneNodeData>& Pair : OutData.BoneDataByName)
	{
		if (const FString* ParentBoneName = OutData.BoneNameByNodeUid.Find(Pair.Value.ParentNodeUid))
		{
			Pair.Value.ParentBoneName = *ParentBoneName;
		}
	}

	BaseNodeContainer->IterateNodesOfType<UInterchangeSceneNode>(
		[BaseNodeContainer, &OutData, &OutErrors](const FString&, UInterchangeSceneNode* SceneNode)
		{
			FString AssetInstanceUid;
			if (!SceneNode->GetCustomAssetInstanceUid(AssetInstanceUid) || AssetInstanceUid.IsEmpty())
			{
				return;
			}

			const UInterchangeMeshNode* MeshNode = Cast<UInterchangeMeshNode>(BaseNodeContainer->GetNode(AssetInstanceUid));
			if (!MeshNode || !MeshNode->IsSkinnedMesh())
			{
				return;
			}

			const FTransform MeshBindTransform = GetBestGlobalBindTransform(BaseNodeContainer, SceneNode);
			if (const FTransform* ExistingBindTransform = OutData.MeshBindTransformByMeshUid.Find(AssetInstanceUid))
			{
				if (!ExistingBindTransform->Equals(MeshBindTransform))
				{
					OutErrors.Add(FString::Printf(TEXT("skinned mesh asset '%s' was instanced with conflicting bind transforms; explicit per-instance bind rebinding is required before formal humanoid import can continue"), *AssetInstanceUid));
				}
				return;
			}

			OutData.SkinnedMeshAssetUids.Add(AssetInstanceUid);
			OutData.MeshBindTransformByMeshUid.Add(AssetInstanceUid, MeshBindTransform);
		});

	OutData.SkinnedMeshAssetUids.Sort();

	OutData.RootBoneName = ResolveExistingBone(BoneNames, { TEXT("Master"), TEXT("Root"), TEXT("root"), TEXT("Pelvis") });
	ResolveTargetParents(BoneNames, OutData.TargetParentByBone);
	if (!BuildGlobalNormalizationMatrix(OutData.BoneDataByName, OutData.GlobalNormalizationMatrix, OutErrors))
	{
		return false;
	}
	if (!BuildNormalizedBindPoseData(OutData, OutErrors))
	{
		return false;
	}

	OutData.bValid = true;
	return true;
}

void USekiroHumanoidImportPipeline::ExecutePipeline(UInterchangeBaseNodeContainer* InBaseNodeContainer, const TArray<UInterchangeSourceData*>& InSourceDatas, const FString& ContentBasePath)
{
	NormalizationErrors.Reset();
	FSekiroHumanoidNormalizationData NormalizationData;
	if (!SekiroHumanoidImport::BuildNormalizationData(InBaseNodeContainer, NormalizationData, NormalizationErrors))
	{
		return;
	}

	ApplyNormalizedHierarchy(InBaseNodeContainer, NormalizationData, NormalizationErrors);
	Super::ExecutePipeline(InBaseNodeContainer, InSourceDatas, ContentBasePath);
}