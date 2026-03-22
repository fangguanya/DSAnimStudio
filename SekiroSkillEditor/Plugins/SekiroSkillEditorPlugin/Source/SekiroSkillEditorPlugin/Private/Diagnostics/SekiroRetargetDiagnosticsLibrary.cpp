#include "Diagnostics/SekiroRetargetDiagnosticsLibrary.h"

#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/Skeleton.h"
#include "AnimationBlueprintLibrary.h"
#include "Dom/JsonObject.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "RetargetEditor/IKRetargeterController.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
	TSharedPtr<FJsonObject> MakeVectorObject(const FVector& Value)
	{
		TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
		JsonObject->SetNumberField(TEXT("x"), Value.X);
		JsonObject->SetNumberField(TEXT("y"), Value.Y);
		JsonObject->SetNumberField(TEXT("z"), Value.Z);
		return JsonObject;
	}

	TSharedPtr<FJsonObject> MakeQuatObject(const FQuat& Value)
	{
		TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
		JsonObject->SetNumberField(TEXT("x"), Value.X);
		JsonObject->SetNumberField(TEXT("y"), Value.Y);
		JsonObject->SetNumberField(TEXT("z"), Value.Z);
		JsonObject->SetNumberField(TEXT("w"), Value.W);
		return JsonObject;
	}

	TSharedPtr<FJsonObject> MakeTransformObject(const FTransform& Value)
	{
		TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
		JsonObject->SetObjectField(TEXT("translation"), MakeVectorObject(Value.GetTranslation()));
		JsonObject->SetObjectField(TEXT("rotation"), MakeQuatObject(Value.GetRotation()));
		JsonObject->SetObjectField(TEXT("scale"), MakeVectorObject(Value.GetScale3D()));
		return JsonObject;
	}

	FString SerializeJsonObject(const TSharedPtr<FJsonObject>& JsonObject)
	{
		FString Output;
		TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Output);
		FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
		return Output;
	}

	void CollectBoneClosure(const FReferenceSkeleton& ReferenceSkeleton, const TArray<FName>& BoneNames, TSet<int32>& OutBoneIndices)
	{
		for (const FName BoneName : BoneNames)
		{
			int32 BoneIndex = ReferenceSkeleton.FindBoneIndex(BoneName);
			while (BoneIndex != INDEX_NONE)
			{
				OutBoneIndices.Add(BoneIndex);
				BoneIndex = ReferenceSkeleton.GetParentIndex(BoneIndex);
			}
		}
	}

	TSharedPtr<FJsonObject> MakeErrorJson(const FString& ErrorMessage)
	{
		TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
		JsonObject->SetStringField(TEXT("error"), ErrorMessage);
		return JsonObject;
	}
}

FString USekiroRetargetDiagnosticsLibrary::DumpAnimationBoneSamplesToJson(UAnimSequence* AnimationSequence, const TArray<FName>& BoneNames, const TArray<int32>& Frames, bool bExtractRootMotion)
{
	if (!AnimationSequence)
	{
		return SerializeJsonObject(MakeErrorJson(TEXT("AnimationSequence was null.")));
	}

	USkeleton* Skeleton = AnimationSequence->GetSkeleton();
	if (!Skeleton)
	{
		return SerializeJsonObject(MakeErrorJson(TEXT("AnimationSequence has no skeleton.")));
	}

	const FReferenceSkeleton& ReferenceSkeleton = Skeleton->GetReferenceSkeleton();
	if (BoneNames.IsEmpty())
	{
		return SerializeJsonObject(MakeErrorJson(TEXT("BoneNames was empty.")));
	}

	TSet<int32> RequiredBoneIndices;
	CollectBoneClosure(ReferenceSkeleton, BoneNames, RequiredBoneIndices);

	TArray<int32> SortedBoneIndices = RequiredBoneIndices.Array();
	SortedBoneIndices.Sort();

	TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
	RootObject->SetStringField(TEXT("animation"), AnimationSequence->GetPathName());
	RootObject->SetStringField(TEXT("skeleton"), Skeleton->GetPathName());

	TArray<TSharedPtr<FJsonValue>> BoneArray;
	for (const FName BoneName : BoneNames)
	{
		BoneArray.Add(MakeShared<FJsonValueString>(BoneName.ToString()));
	}
	RootObject->SetArrayField(TEXT("requestedBones"), BoneArray);

	TArray<TSharedPtr<FJsonValue>> FrameSamples;
	for (const int32 FrameIndex : Frames)
	{
		TMap<int32, FTransform> LocalTransforms;
		for (const int32 BoneIndex : SortedBoneIndices)
		{
			FTransform LocalTransform = FTransform::Identity;
			UAnimationBlueprintLibrary::GetBonePoseForFrame(AnimationSequence, ReferenceSkeleton.GetBoneName(BoneIndex), FrameIndex, bExtractRootMotion, LocalTransform);
			LocalTransforms.Add(BoneIndex, LocalTransform);
		}

		TMap<int32, FTransform> ComponentTransforms;
		TFunction<FTransform(int32)> ResolveComponentTransform = [&](int32 BoneIndex) -> FTransform
		{
			if (const FTransform* CachedTransform = ComponentTransforms.Find(BoneIndex))
			{
				return *CachedTransform;
			}

			const FTransform& LocalTransform = LocalTransforms.FindChecked(BoneIndex);
			const int32 ParentIndex = ReferenceSkeleton.GetParentIndex(BoneIndex);
			const FTransform ComponentTransform = ParentIndex == INDEX_NONE
				? LocalTransform
				: LocalTransform * ResolveComponentTransform(ParentIndex);

			ComponentTransforms.Add(BoneIndex, ComponentTransform);
			return ComponentTransform;
		};

		TSharedPtr<FJsonObject> FrameObject = MakeShared<FJsonObject>();
		FrameObject->SetNumberField(TEXT("frame"), FrameIndex);

		TArray<TSharedPtr<FJsonValue>> SampleArray;
		for (const FName BoneName : BoneNames)
		{
			const int32 BoneIndex = ReferenceSkeleton.FindBoneIndex(BoneName);
			if (BoneIndex == INDEX_NONE)
			{
				TSharedPtr<FJsonObject> MissingBoneObject = MakeShared<FJsonObject>();
				MissingBoneObject->SetStringField(TEXT("bone"), BoneName.ToString());
				MissingBoneObject->SetStringField(TEXT("error"), TEXT("bone_not_found"));
				SampleArray.Add(MakeShared<FJsonValueObject>(MissingBoneObject));
				continue;
			}

			TSharedPtr<FJsonObject> SampleObject = MakeShared<FJsonObject>();
			SampleObject->SetStringField(TEXT("bone"), BoneName.ToString());
			SampleObject->SetNumberField(TEXT("boneIndex"), BoneIndex);
			SampleObject->SetStringField(TEXT("parentBone"), ReferenceSkeleton.GetParentIndex(BoneIndex) == INDEX_NONE
				? FString()
				: ReferenceSkeleton.GetBoneName(ReferenceSkeleton.GetParentIndex(BoneIndex)).ToString());
			SampleObject->SetObjectField(TEXT("local"), MakeTransformObject(LocalTransforms.FindChecked(BoneIndex)));
			SampleObject->SetObjectField(TEXT("component"), MakeTransformObject(ResolveComponentTransform(BoneIndex)));
			SampleArray.Add(MakeShared<FJsonValueObject>(SampleObject));
		}

		FrameObject->SetArrayField(TEXT("samples"), SampleArray);
		FrameSamples.Add(MakeShared<FJsonValueObject>(FrameObject));
	}

	RootObject->SetArrayField(TEXT("frames"), FrameSamples);
	return SerializeJsonObject(RootObject);
}

FString USekiroRetargetDiagnosticsLibrary::DumpSkeletalMeshReferencePoseToJson(USkeletalMesh* SkeletalMesh, const TArray<FName>& BoneNames)
{
	if (!SkeletalMesh)
	{
		return SerializeJsonObject(MakeErrorJson(TEXT("SkeletalMesh was null.")));
	}

	const FReferenceSkeleton& ReferenceSkeleton = SkeletalMesh->GetRefSkeleton();
	if (ReferenceSkeleton.GetNum() == 0)
	{
		return SerializeJsonObject(MakeErrorJson(TEXT("SkeletalMesh reference skeleton was empty.")));
	}

	TArray<int32> SortedBoneIndices;
	if (BoneNames.IsEmpty())
	{
		SortedBoneIndices.Reserve(ReferenceSkeleton.GetNum());
		for (int32 BoneIndex = 0; BoneIndex < ReferenceSkeleton.GetNum(); ++BoneIndex)
		{
			SortedBoneIndices.Add(BoneIndex);
		}
	}
	else
	{
		TSet<int32> RequiredBoneIndices;
		CollectBoneClosure(ReferenceSkeleton, BoneNames, RequiredBoneIndices);
		SortedBoneIndices = RequiredBoneIndices.Array();
		SortedBoneIndices.Sort();
	}

	const TArray<FTransform>& RefPose = ReferenceSkeleton.GetRefBonePose();
	TArray<FTransform> ComponentPose;
	ComponentPose.SetNum(ReferenceSkeleton.GetNum());
	for (int32 BoneIndex = 0; BoneIndex < ReferenceSkeleton.GetNum(); ++BoneIndex)
	{
		const int32 ParentIndex = ReferenceSkeleton.GetParentIndex(BoneIndex);
		ComponentPose[BoneIndex] = ParentIndex == INDEX_NONE
			? RefPose[BoneIndex]
			: RefPose[BoneIndex] * ComponentPose[ParentIndex];
	}

	TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
	RootObject->SetStringField(TEXT("skeletalMesh"), SkeletalMesh->GetPathName());

	TArray<TSharedPtr<FJsonValue>> BoneArray;
	for (const int32 BoneIndex : SortedBoneIndices)
	{
		TSharedPtr<FJsonObject> BoneObject = MakeShared<FJsonObject>();
		BoneObject->SetNumberField(TEXT("boneIndex"), BoneIndex);
		BoneObject->SetStringField(TEXT("bone"), ReferenceSkeleton.GetBoneName(BoneIndex).ToString());
		BoneObject->SetStringField(TEXT("parentBone"), ReferenceSkeleton.GetParentIndex(BoneIndex) == INDEX_NONE
			? FString()
			: ReferenceSkeleton.GetBoneName(ReferenceSkeleton.GetParentIndex(BoneIndex)).ToString());
		BoneObject->SetObjectField(TEXT("refLocal"), MakeTransformObject(RefPose[BoneIndex]));
		BoneObject->SetObjectField(TEXT("refComponent"), MakeTransformObject(ComponentPose[BoneIndex]));
		BoneArray.Add(MakeShared<FJsonValueObject>(BoneObject));
	}

	RootObject->SetArrayField(TEXT("bones"), BoneArray);
	return SerializeJsonObject(RootObject);
}

FString USekiroRetargetDiagnosticsLibrary::DumpSkeletonBranchToJson(USkeleton* Skeleton, FName RootBone)
{
	if (!Skeleton)
	{
		return SerializeJsonObject(MakeErrorJson(TEXT("Skeleton was null.")));
	}

	const FReferenceSkeleton& ReferenceSkeleton = Skeleton->GetReferenceSkeleton();
	const int32 RootBoneIndex = ReferenceSkeleton.FindBoneIndex(RootBone);
	if (RootBoneIndex == INDEX_NONE)
	{
		return SerializeJsonObject(MakeErrorJson(FString::Printf(TEXT("Root bone '%s' was not found."), *RootBone.ToString())));
	}

	TMap<int32, TArray<int32>> ChildrenByParent;
	for (int32 BoneIndex = 0; BoneIndex < ReferenceSkeleton.GetNum(); ++BoneIndex)
	{
		ChildrenByParent.FindOrAdd(ReferenceSkeleton.GetParentIndex(BoneIndex)).Add(BoneIndex);
	}

	TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
	RootObject->SetStringField(TEXT("skeleton"), Skeleton->GetPathName());
	RootObject->SetStringField(TEXT("rootBone"), RootBone.ToString());

	TArray<TSharedPtr<FJsonValue>> BranchArray;
	TArray<int32> Stack = { RootBoneIndex };
	while (!Stack.IsEmpty())
	{
		const int32 BoneIndex = Stack.Pop(EAllowShrinking::No);
		TSharedPtr<FJsonObject> BoneObject = MakeShared<FJsonObject>();
		BoneObject->SetNumberField(TEXT("boneIndex"), BoneIndex);
		BoneObject->SetStringField(TEXT("bone"), ReferenceSkeleton.GetBoneName(BoneIndex).ToString());
		BoneObject->SetStringField(TEXT("parentBone"), ReferenceSkeleton.GetParentIndex(BoneIndex) == INDEX_NONE
			? FString()
			: ReferenceSkeleton.GetBoneName(ReferenceSkeleton.GetParentIndex(BoneIndex)).ToString());
		BoneObject->SetObjectField(TEXT("refLocal"), MakeTransformObject(ReferenceSkeleton.GetRefBonePose()[BoneIndex]));
		BranchArray.Add(MakeShared<FJsonValueObject>(BoneObject));

		TArray<int32> ChildIndices = ChildrenByParent.FindRef(BoneIndex);
		ChildIndices.Sort();
		for (int32 ChildArrayIndex = ChildIndices.Num() - 1; ChildArrayIndex >= 0; --ChildArrayIndex)
		{
			Stack.Add(ChildIndices[ChildArrayIndex]);
		}
	}

	RootObject->SetArrayField(TEXT("branch"), BranchArray);
	return SerializeJsonObject(RootObject);
}

FString USekiroRetargetDiagnosticsLibrary::DumpRetargetPoseToJson(UIKRetargeter* Retargeter, ERetargetSourceOrTarget SourceOrTarget, const TArray<FName>& BoneNames)
{
	if (!Retargeter)
	{
		return SerializeJsonObject(MakeErrorJson(TEXT("Retargeter was null.")));
	}

	UIKRetargeterController* Controller = UIKRetargeterController::GetController(Retargeter);
	if (!Controller)
	{
		return SerializeJsonObject(MakeErrorJson(TEXT("Failed to create retargeter controller.")));
	}

	const FName CurrentPoseName = Controller->GetCurrentRetargetPoseName(SourceOrTarget);
	const FIKRetargetPose& CurrentPose = Controller->GetCurrentRetargetPose(SourceOrTarget);

	TArray<FName> PoseBoneNames = BoneNames;
	if (PoseBoneNames.IsEmpty())
	{
		CurrentPose.GetAllDeltaRotations().GenerateKeyArray(PoseBoneNames);
		PoseBoneNames.Sort(FNameLexicalLess());
	}

	TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
	RootObject->SetStringField(TEXT("retargeter"), Retargeter->GetPathName());
	RootObject->SetStringField(TEXT("poseName"), CurrentPoseName.ToString());
	RootObject->SetStringField(TEXT("side"), SourceOrTarget == ERetargetSourceOrTarget::Source ? TEXT("source") : TEXT("target"));
	RootObject->SetObjectField(TEXT("rootOffset"), MakeVectorObject(CurrentPose.GetRootTranslationDelta()));

	TArray<TSharedPtr<FJsonValue>> BoneArray;
	for (const FName BoneName : PoseBoneNames)
	{
		TSharedPtr<FJsonObject> BoneObject = MakeShared<FJsonObject>();
		BoneObject->SetStringField(TEXT("bone"), BoneName.ToString());
		BoneObject->SetObjectField(TEXT("deltaRotation"), MakeQuatObject(CurrentPose.GetDeltaRotationForBone(BoneName)));
		BoneArray.Add(MakeShared<FJsonValueObject>(BoneObject));
	}

	RootObject->SetArrayField(TEXT("bones"), BoneArray);
	return SerializeJsonObject(RootObject);
}