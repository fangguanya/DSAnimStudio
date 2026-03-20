#pragma once

#include "CoreMinimal.h"

class USkeleton;
class USkeletalMesh;

namespace SekiroHumanoidValidation
{
    struct FVisiblePoseValidationResult
    {
        bool bValidated = false;
        TArray<FString> Errors;

        bool IsValid() const
        {
            return bValidated && Errors.Num() == 0;
        }
    };

    FVisiblePoseValidationResult ValidateImportedVisibleHumanoidPose(USkeleton* Skeleton, USkeletalMesh* SkeletalMesh);
}