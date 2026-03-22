#pragma once

#include "CoreMinimal.h"
#include "InterchangeGenericAssetsPipeline.h"
#include "SekiroHumanoidImportPipeline.generated.h"

class UInterchangeBaseNodeContainer;
class UInterchangeSceneNode;

struct FSekiroHumanoidSceneNodeData
{
	FString NodeUid;
	FString BoneName;
	FString ParentNodeUid;
	FString ParentBoneName;
	FTransform LocalBindTransform = FTransform::Identity;
	FTransform GlobalBindTransform = FTransform::Identity;
	FString NormalizedParentBoneName;
	FTransform NormalizedLocalBindTransform = FTransform::Identity;
	FTransform NormalizedGlobalBindTransform = FTransform::Identity;
};

struct FSekiroHumanoidNormalizationData
{
	bool bValid = false;
	FMatrix GlobalNormalizationMatrix = FMatrix::Identity;
	FString RootBoneName;
	TArray<FString> BoneOrder;
	TArray<FString> SkinnedMeshAssetUids;
	TMap<FString, FString> BoneNameByNodeUid;
	TMap<FString, FString> TargetParentByBone;
	TMap<FString, FQuat> ComponentRotationRebaseByBone;
	TMap<FString, FTransform> MeshBindTransformByMeshUid;
	TMap<FString, FTransform> NormalizedMeshBindTransformByMeshUid;
	TMap<FString, FSekiroHumanoidSceneNodeData> BoneDataByName;
};

namespace SekiroHumanoidImport
{
	SEKIROSKILLEDITORPLUGIN_API bool CollectJointSceneNodes(
		const UInterchangeBaseNodeContainer* BaseNodeContainer,
		TMap<FString, const UInterchangeSceneNode*>& OutNodesByBoneName,
		TArray<FString>& OutErrors);

	SEKIROSKILLEDITORPLUGIN_API bool BuildNormalizationData(
		const UInterchangeBaseNodeContainer* BaseNodeContainer,
		FSekiroHumanoidNormalizationData& OutData,
		TArray<FString>& OutErrors);
}

UCLASS()
class SEKIROSKILLEDITORPLUGIN_API USekiroHumanoidImportPipeline : public UInterchangeGenericAssetsPipeline
{
	GENERATED_BODY()

public:
	const TArray<FString>& GetNormalizationErrors() const
	{
		return NormalizationErrors;
	}

protected:
	virtual void ExecutePipeline(UInterchangeBaseNodeContainer* InBaseNodeContainer, const TArray<UInterchangeSourceData*>& InSourceDatas, const FString& ContentBasePath) override;

private:
	TArray<FString> NormalizationErrors;
};