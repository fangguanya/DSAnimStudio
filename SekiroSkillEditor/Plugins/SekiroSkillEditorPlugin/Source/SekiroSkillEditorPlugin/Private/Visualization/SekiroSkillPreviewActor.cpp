// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

#include "Visualization/SekiroSkillPreviewActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Data/SekiroCharacterData.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInterface.h"

ASekiroSkillPreviewActor::ASekiroSkillPreviewActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	MeshComp->bEnableUpdateRateOptimizations = false;
	SetRootComponent(MeshComp);
}

void ASekiroSkillPreviewActor::SetCharacterData(USekiroCharacterData* InCharacterData)
{
	if (!InCharacterData)
	{
		return;
	}

	CurrentCharacterData = InCharacterData;

	// Synchronously load the skeletal mesh from the soft reference.
	USkeletalMesh* LoadedMesh = InCharacterData->Mesh.LoadSynchronous();
	if (LoadedMesh)
	{
		MeshComp->SetSkeletalMesh(LoadedMesh);
	}

	// Apply materials from the character data material map.
	if (LoadedMesh)
	{
		const TArray<FSkeletalMaterial>& MeshMaterials = LoadedMesh->GetMaterials();
		for (int32 SlotIdx = 0; SlotIdx < MeshMaterials.Num(); ++SlotIdx)
		{
			const FName SlotName = MeshMaterials[SlotIdx].MaterialSlotName;
			if (const TSoftObjectPtr<UMaterialInterface>* FoundMat = InCharacterData->MaterialMap.Find(SlotName.ToString()))
			{
				UMaterialInterface* LoadedMat = FoundMat->LoadSynchronous();
				if (LoadedMat)
				{
					MeshComp->SetMaterial(SlotIdx, LoadedMat);
				}
			}
		}
	}

	// Reset animation state when character changes.
	CurrentAnim = nullptr;
	CurrentFrame = 0.0f;
	bIsPlaying = false;
}

void ASekiroSkillPreviewActor::PlayAnimation(UAnimSequence* InAnim)
{
	CurrentAnim = InAnim;
	CurrentFrame = 0.0f;

	if (MeshComp && CurrentAnim)
	{
		MeshComp->SetAnimation(CurrentAnim);
		MeshComp->SetPosition(0.0f);
		MeshComp->Play(false); // Don't loop via the component -- we drive playback manually.
		MeshComp->Stop();
		bIsPlaying = true;
	}
}

void ASekiroSkillPreviewActor::SetPlaybackPosition(float Frame)
{
	bIsPlaying = false;
	CurrentFrame = Frame;

	if (MeshComp && CurrentAnim)
	{
		const float FrameRate = CurrentAnim->GetSamplingFrameRate().AsDecimal();
		const float TimePosition = (FrameRate > UE_KINDA_SMALL_NUMBER) ? (Frame / FrameRate) : 0.0f;
		MeshComp->SetPosition(TimePosition);
	}
}

float ASekiroSkillPreviewActor::GetCurrentFrame() const
{
	return CurrentFrame;
}

void ASekiroSkillPreviewActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bIsPlaying || !CurrentAnim || !MeshComp)
	{
		return;
	}

	const float FrameRate = CurrentAnim->GetSamplingFrameRate().AsDecimal();
	if (FrameRate <= UE_KINDA_SMALL_NUMBER)
	{
		return;
	}

	// Advance the current frame by the elapsed time scaled by playback rate.
	CurrentFrame += DeltaSeconds * FrameRate * PlaybackRate;

	const float TotalFrames = CurrentAnim->GetPlayLength() * FrameRate;
	if (CurrentFrame >= TotalFrames)
	{
		CurrentFrame = 0.0f; // Loop back to start.
	}

	const float TimePosition = CurrentFrame / FrameRate;
	MeshComp->SetPosition(TimePosition);
}
