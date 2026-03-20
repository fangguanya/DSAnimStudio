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
		OutMatrix = BasisCorrection * MirrorX;
		auto MatrixRowToString = [&OutMatrix](int32 RowIndex)
		{
			return FString::Printf(TEXT("X=%0.6f Y=%0.6f Z=%0.6f W=%0.6f"), OutMatrix.M[RowIndex][0], OutMatrix.M[RowIndex][1], OutMatrix.M[RowIndex][2], OutMatrix.M[RowIndex][3]);
		};
		UE_LOG(LogTemp, Display, TEXT("SekiroHumanoidImport: source bind Left=%s Right=%s Pelvis=%s Head=%s LateralAxis=%s UpAxis=%s"),
			*Left.ToString(), *Right.ToString(), *Pelvis.ToString(), *Head.ToString(), *LateralAxis.ToString(), *UpAxis.ToString());
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

		const FString ArmRoot = ResolveExistingBone(BoneNames, { TEXT("UpperChest"), TEXT("Chest"), TEXT("Spine2"), TEXT("Spine1") });
		SetParent(TEXT("L_Shoulder"), ArmRoot);
		SetParent(TEXT("L_UpperArm"), ResolveExistingBone(BoneNames, { TEXT("L_Shoulder") }));
		SetParent(TEXT("L_Forearm"), ResolveExistingBone(BoneNames, { TEXT("L_UpperArm") }));
		SetParent(TEXT("L_Hand"), ResolveExistingBone(BoneNames, { TEXT("L_Forearm") }));

		SetParent(TEXT("R_Shoulder"), ArmRoot);
		SetParent(TEXT("R_UpperArm"), ResolveExistingBone(BoneNames, { TEXT("R_Shoulder") }));
		SetParent(TEXT("R_Forearm"), ResolveExistingBone(BoneNames, { TEXT("R_UpperArm") }));
		SetParent(TEXT("R_Hand"), ResolveExistingBone(BoneNames, { TEXT("R_Forearm") }));

		SetParent(TEXT("L_Thigh"), ResolveExistingBone(BoneNames, { TEXT("Pelvis") }));
		SetParent(TEXT("L_Calf"), ResolveExistingBone(BoneNames, { TEXT("L_Thigh") }));
		SetParent(TEXT("L_Foot"), ResolveExistingBone(BoneNames, { TEXT("L_Calf") }));
		SetParent(TEXT("L_Toe0"), ResolveExistingBone(BoneNames, { TEXT("L_Foot") }));

		SetParent(TEXT("R_Thigh"), ResolveExistingBone(BoneNames, { TEXT("Pelvis") }));
		SetParent(TEXT("R_Calf"), ResolveExistingBone(BoneNames, { TEXT("R_Thigh") }));
		SetParent(TEXT("R_Foot"), ResolveExistingBone(BoneNames, { TEXT("R_Calf") }));
		SetParent(TEXT("R_Toe0"), ResolveExistingBone(BoneNames, { TEXT("R_Foot") }));
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
			if (const FTransform* MeshBindTransform = Data.MeshBindTransformByMeshUid.Find(MeshUid))
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
			const FMatrix BoneWorldMatrix = ComputeNormalizedWorldMatrix(BoneName, Data, NormalizedWorldMatrices);
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

			FMatrix LocalMatrix = BoneWorldMatrix;

			if (!SelectedParentName.IsEmpty())
			{
				const UInterchangeSceneNode* ParentSceneNode = SceneNodesByBoneName.FindRef(SelectedParentName);
				if (ParentSceneNode)
				{
					const FMatrix ParentWorldMatrix = ComputeNormalizedWorldMatrix(SelectedParentName, Data, NormalizedWorldMatrices);
					LocalMatrix = BoneWorldMatrix * ParentWorldMatrix.InverseFast();
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
			SceneNode->SetCustomBindPoseLocalTransform(BaseNodeContainer, BoneData.LocalBindTransform, false);
			SceneNode->SetCustomTimeZeroLocalTransform(BaseNodeContainer, LocalTransform, false);
			SceneNode->SetCustomHasBindPose(false);
			SceneNode->SetCustomGlobalMatrixForT0Rebinding(BoneWorldMatrix);
			SceneNode->SetGlobalBindPoseReferenceForMeshUIDs(MeshBindReferenceByUid);

			for (const FString& MeshUid : Data.SkinnedMeshAssetUids)
			{
				const FString AttributeKey = TEXT("JointBindPosePerMesh_") + MeshUid;
				SceneNode->RegisterAttribute<FMatrix>(UE::Interchange::FAttributeKey(AttributeKey), BoneData.GlobalBindTransform.ToMatrixWithScale());
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