#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Retargeter/IKRetargeter.h"
#include "SekiroRetargetDiagnosticsLibrary.generated.h"

UCLASS()
class USekiroRetargetDiagnosticsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Sekiro|Diagnostics")
	static FString DumpAnimationBoneSamplesToJson(UAnimSequence* AnimationSequence, const TArray<FName>& BoneNames, const TArray<int32>& Frames, bool bExtractRootMotion = true);

	UFUNCTION(BlueprintCallable, Category = "Sekiro|Diagnostics")
	static FString DumpSkeletalMeshReferencePoseToJson(USkeletalMesh* SkeletalMesh, const TArray<FName>& BoneNames);

	UFUNCTION(BlueprintCallable, Category = "Sekiro|Diagnostics")
	static FString DumpSkeletonBranchToJson(USkeleton* Skeleton, FName RootBone);

	UFUNCTION(BlueprintCallable, Category = "Sekiro|Diagnostics")
	static FString DumpRetargetPoseToJson(UIKRetargeter* Retargeter, ERetargetSourceOrTarget SourceOrTarget, const TArray<FName>& BoneNames);
};