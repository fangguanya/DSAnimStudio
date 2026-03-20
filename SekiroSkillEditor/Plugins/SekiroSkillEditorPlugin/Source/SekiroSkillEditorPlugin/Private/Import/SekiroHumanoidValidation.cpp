#include "Import/SekiroHumanoidValidation.h"

#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"
#include "ReferenceSkeleton.h"

namespace
{
    static int32 FindBoneIndexAny(const FReferenceSkeleton& RefSkeleton, std::initializer_list<const TCHAR*> BoneNames)
    {
        for (const TCHAR* BoneName : BoneNames)
        {
            const int32 BoneIndex = RefSkeleton.FindBoneIndex(FName(BoneName));
            if (BoneIndex != INDEX_NONE)
            {
                return BoneIndex;
            }
        }

        return INDEX_NONE;
    }

    static void BuildReferencePoseWorldTransforms(const FReferenceSkeleton& RefSkeleton, TArray<FTransform>& OutWorldTransforms)
    {
        const TArray<FTransform>& LocalPoses = RefSkeleton.GetRefBonePose();
        const int32 BoneCount = RefSkeleton.GetNum();
        OutWorldTransforms.SetNum(BoneCount);

        for (int32 BoneIndex = 0; BoneIndex < BoneCount; ++BoneIndex)
        {
            const int32 ParentIndex = RefSkeleton.GetParentIndex(BoneIndex);
            if (ParentIndex == INDEX_NONE)
            {
                OutWorldTransforms[BoneIndex] = LocalPoses[BoneIndex];
            }
            else
            {
                OutWorldTransforms[BoneIndex] = LocalPoses[BoneIndex] * OutWorldTransforms[ParentIndex];
            }
        }
    }

    static int32 FindBestArmRoot(const FReferenceSkeleton& RefSkeleton, bool bRightSide)
    {
        return bRightSide
            ? FindBoneIndexAny(RefSkeleton, { TEXT("R_Shoulder"), TEXT("R_Clavicle"), TEXT("R_UpperArm") })
            : FindBoneIndexAny(RefSkeleton, { TEXT("L_Shoulder"), TEXT("L_Clavicle"), TEXT("L_UpperArm") });
    }

    static bool TryGetMinFootHeight(const FReferenceSkeleton& RefSkeleton, const TArray<FTransform>& WorldTransforms, float& OutMinFootZ)
    {
        TArray<int32> FootBoneIndices;
        auto AddIfPresent = [&RefSkeleton, &FootBoneIndices](const TCHAR* BoneName)
        {
            const int32 BoneIndex = RefSkeleton.FindBoneIndex(FName(BoneName));
            if (BoneIndex != INDEX_NONE)
            {
                FootBoneIndices.Add(BoneIndex);
            }
        };

        AddIfPresent(TEXT("L_Foot"));
        AddIfPresent(TEXT("R_Foot"));
        AddIfPresent(TEXT("L_Toe0"));
        AddIfPresent(TEXT("R_Toe0"));

        if (FootBoneIndices.Num() == 0)
        {
            return false;
        }

        OutMinFootZ = TNumericLimits<float>::Max();
        for (const int32 BoneIndex : FootBoneIndices)
        {
            OutMinFootZ = FMath::Min(OutMinFootZ, WorldTransforms[BoneIndex].GetTranslation().Z);
        }

        return true;
    }
}

SekiroHumanoidValidation::FVisiblePoseValidationResult SekiroHumanoidValidation::ValidateImportedVisibleHumanoidPose(USkeleton* Skeleton, USkeletalMesh* SkeletalMesh)
{
    FVisiblePoseValidationResult Result;

    if (!Skeleton)
    {
        Result.Errors.Add(TEXT("visible pose validation failed: missing Skeleton"));
        return Result;
    }

    const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();
    if (RefSkeleton.GetNum() == 0)
    {
        Result.Errors.Add(TEXT("visible pose validation failed: Skeleton has no reference bones"));
        return Result;
    }

    TArray<FTransform> WorldTransforms;
    BuildReferencePoseWorldTransforms(RefSkeleton, WorldTransforms);

    const int32 PelvisIndex = FindBoneIndexAny(RefSkeleton, { TEXT("Pelvis") });
    const int32 HeadIndex = FindBoneIndexAny(RefSkeleton, { TEXT("Head") });
    const int32 LeftHandIndex = FindBoneIndexAny(RefSkeleton, { TEXT("L_Hand") });
    const int32 RightHandIndex = FindBoneIndexAny(RefSkeleton, { TEXT("R_Hand") });
    const int32 LeftArmRootIndex = FindBestArmRoot(RefSkeleton, false);
    const int32 RightArmRootIndex = FindBestArmRoot(RefSkeleton, true);

    if (PelvisIndex == INDEX_NONE || HeadIndex == INDEX_NONE)
    {
        Result.Errors.Add(TEXT("visible pose validation failed: required torso bones 'Pelvis'/'Head' are missing"));
        return Result;
    }

    const FVector PelvisPosition = WorldTransforms[PelvisIndex].GetTranslation();
    const FVector HeadPosition = WorldTransforms[HeadIndex].GetTranslation();
    const FVector HeadUpVector = (HeadPosition - PelvisPosition).GetSafeNormal();

    if (HeadPosition.Z <= PelvisPosition.Z)
    {
        Result.Errors.Add(TEXT("visible pose validation failed: head is not above pelvis in UE reference pose"));
    }

    if (HeadUpVector.Z < 0.75f)
    {
        Result.Errors.Add(FString::Printf(TEXT("visible pose orientation mismatch: head/pelvis up vector is not aligned with UE +Z (up.Z=%.3f)"), HeadUpVector.Z));
    }

    if (LeftHandIndex != INDEX_NONE && RightHandIndex != INDEX_NONE && LeftArmRootIndex != INDEX_NONE && RightArmRootIndex != INDEX_NONE)
    {
        const FVector LeftHandPosition = WorldTransforms[LeftHandIndex].GetTranslation();
        const FVector RightHandPosition = WorldTransforms[RightHandIndex].GetTranslation();
        const FVector LeftArmRootPosition = WorldTransforms[LeftArmRootIndex].GetTranslation();
        const FVector RightArmRootPosition = WorldTransforms[RightArmRootIndex].GetTranslation();

        const float RightToRightArm = FVector::DistSquared(RightHandPosition, RightArmRootPosition);
        const float RightToLeftArm = FVector::DistSquared(RightHandPosition, LeftArmRootPosition);
        if (RightToRightArm >= RightToLeftArm)
        {
            Result.Errors.Add(TEXT("visible pose laterality mismatch: R_* arm chain resolves closer to the left arm root than the right arm root"));
        }

        const float LeftToLeftArm = FVector::DistSquared(LeftHandPosition, LeftArmRootPosition);
        const float LeftToRightArm = FVector::DistSquared(LeftHandPosition, RightArmRootPosition);
        if (LeftToLeftArm >= LeftToRightArm)
        {
            Result.Errors.Add(TEXT("visible pose laterality mismatch: L_* arm chain resolves closer to the right arm root than the left arm root"));
        }
    }

    float MinFootZ = 0.0f;
    if (TryGetMinFootHeight(RefSkeleton, WorldTransforms, MinFootZ))
    {
        const float StandingHeight = HeadPosition.Z - MinFootZ;
        if (StandingHeight <= KINDA_SMALL_NUMBER)
        {
            Result.Errors.Add(TEXT("visible pose validation failed: standing height computed from Head/Feet is non-positive"));
        }
        else
        {
            const float PelvisToFootHeight = PelvisPosition.Z - MinFootZ;
            if (PelvisToFootHeight <= 0.0f)
            {
                Result.Errors.Add(TEXT("visible pose validation failed: pelvis is not above the feet"));
            }

            float MinRootWrapperZ = TNumericLimits<float>::Max();
            int32 RootWrapperCount = 0;
            for (const TCHAR* BoneName : { TEXT("Master"), TEXT("RootPos"), TEXT("RootRotY"), TEXT("RootRotXZ") })
            {
                const int32 BoneIndex = RefSkeleton.FindBoneIndex(FName(BoneName));
                if (BoneIndex != INDEX_NONE)
                {
                    MinRootWrapperZ = FMath::Min(MinRootWrapperZ, WorldTransforms[BoneIndex].GetTranslation().Z);
                    RootWrapperCount++;
                }
            }

            if (RootWrapperCount > 0)
            {
                if (MinRootWrapperZ < MinFootZ - (StandingHeight * 0.35f))
                {
                    Result.Errors.Add(FString::Printf(TEXT("visible pose grounding mismatch: root wrapper chain extends %.3f cm below the feet"), MinFootZ - MinRootWrapperZ));
                }

                const float PelvisToRootDepth = PelvisPosition.Z - MinRootWrapperZ;
                if (PelvisToRootDepth > StandingHeight * 1.10f)
                {
                    Result.Errors.Add(FString::Printf(TEXT("visible pose grounding mismatch: pelvis sits %.3f cm above the lowest root wrapper, which exceeds the character standing height"), PelvisToRootDepth));
                }
            }

            if (SkeletalMesh)
            {
                const FBoxSphereBounds MeshBounds = SkeletalMesh->GetBounds();
                const float MeshMinZ = MeshBounds.Origin.Z - MeshBounds.BoxExtent.Z;
                if (MinFootZ > MeshMinZ + (StandingHeight * 0.35f))
                {
                    Result.Errors.Add(FString::Printf(TEXT("visible pose grounding mismatch: feet sit %.3f cm above the mesh lower bound"), MinFootZ - MeshMinZ));
                }
            }
        }
    }

    Result.bValidated = true;
    return Result;
}