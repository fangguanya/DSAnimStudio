// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

#include "Visualization/SekiroSkillPreviewActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Data/SekiroCharacterData.h"
#include "Data/SekiroSkillDataAsset.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInterface.h"
#include "DrawDebugHelpers.h"

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
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("SekiroSkillPreviewActor: material slot '%s' has unresolvable formal reference"), *SlotName.ToString());
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("SekiroSkillPreviewActor: material slot '%s' has no formal manifest binding"), *SlotName.ToString());
			}
		}
	}

	// Reset animation state when character changes.
	CurrentAnim = nullptr;
	CurrentFrame = 0.0f;
	bIsPlaying = false;
}

void ASekiroSkillPreviewActor::SetSkillData(USekiroSkillDataAsset* InSkillData)
{
	CurrentSkillData = InSkillData;
	if (!CurrentSkillData)
	{
		CurrentAnim = nullptr;
		CurrentFrame = 0.0f;
		bIsPlaying = false;
		SetActorLocation(FVector::ZeroVector);
		SetActorRotation(FRotator::ZeroRotator);
		return;
	}

	if (USekiroCharacterData* LoadedCharacter = CurrentSkillData->CharacterData.LoadSynchronous())
	{
		SetCharacterData(LoadedCharacter);
	}

	if (UAnimSequence* LoadedAnimation = CurrentSkillData->Animation.LoadSynchronous())
	{
		PlayAnimation(LoadedAnimation);
		bIsPlaying = false;
		SetPlaybackPosition(0.0f);
	}
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

	ApplyCurrentRootMotion();
	UpdateAttackVisualization();
	UpdateEffectSoundVisualization();
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
	ApplyCurrentRootMotion();
	UpdateAttackVisualization();
	UpdateEffectSoundVisualization();
}

void ASekiroSkillPreviewActor::ApplyCurrentRootMotion()
{
	if (!CurrentSkillData)
	{
		SetActorLocation(FVector::ZeroVector);
		SetActorRotation(FRotator::ZeroRotator);
		return;
	}

	FSekiroRootMotionSample Sample;
	if (!CurrentSkillData->GetRootMotionSampleAtFrame(CurrentFrame, Sample))
	{
		SetActorLocation(FVector::ZeroVector);
		SetActorRotation(FRotator::ZeroRotator);
		return;
	}

	SetActorLocation(Sample.Translation);
	SetActorRotation(FRotator(0.0f, FMath::RadiansToDegrees(Sample.YawRadians), 0.0f));
}

bool ASekiroSkillPreviewActor::ResolveDummyPolyWorldPosition(int32 DummyPolyId, FVector& OutWorldPos) const
{
	if (!CurrentCharacterData || !MeshComp || !MeshComp->GetSkeletalMeshAsset())
	{
		return false;
	}

	const FSekiroDummyPoly* Dp = CurrentCharacterData->FindDummyPoly(DummyPolyId);
	if (!Dp)
	{
		return false;
	}

	// Find the bone index for the attach bone
	const FName BoneName(*Dp->AttachBoneName);
	const int32 BoneIdx = MeshComp->GetBoneIndex(BoneName);
	if (BoneIdx == INDEX_NONE)
	{
		return false;
	}

	// Get the bone's world transform and apply the DummyPoly's local offset
	const FTransform BoneWorldTransform = MeshComp->GetBoneTransform(BoneIdx);
	OutWorldPos = BoneWorldTransform.TransformPosition(Dp->LocalPosition);
	return true;
}

void ASekiroSkillPreviewActor::UpdateAttackVisualization()
{
	if (!bShowAttackHitboxes || !CurrentSkillData || !CurrentCharacterData)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Get all events active at the current frame
	TArray<FSekiroTaeEvent> ActiveEvents = CurrentSkillData->GetEventAtFrame(CurrentFrame);

	for (const FSekiroTaeEvent& Evt : ActiveEvents)
	{
		// Only visualize Attack-category events
		if (Evt.Category != TEXT("Attack"))
		{
			continue;
		}

		// Resolve DummyPoly references from semantic links
		for (const FSekiroSemanticLink& Link : Evt.SemanticLinks)
		{
			if (!Link.Name.Contains(TEXT("DummyPoly")) && !Link.Name.Contains(TEXT("dummyPoly")))
			{
				continue;
			}

			int32 DpId = FCString::Atoi(*Link.ValueJson);
			FVector WorldPos;
			if (ResolveDummyPolyWorldPosition(DpId, WorldPos))
			{
				// Determine radius from AtkParam if available
				float HitRadius = 30.0f; // Default radius in cm

				// Try to find atkId from event params
				const FString* AtkIdStr = Evt.Parameters.Find(TEXT("atkId"));
				if (AtkIdStr)
				{
					int32 AtkId = FCString::Atoi(**AtkIdStr);
					// Check player AtkParams first, then NPC
					const FSekiroParamRow* AtkRow = CurrentCharacterData->AtkParamsPlayer.Find(AtkId);
					if (!AtkRow)
					{
						AtkRow = CurrentCharacterData->AtkParamsNpc.Find(AtkId);
					}
					if (AtkRow)
					{
						for (const FSekiroParamField& Field : AtkRow->Fields)
						{
							if (Field.Name == TEXT("hit0_Radius") || Field.Name == TEXT("hitRadius"))
							{
								HitRadius = FCString::Atof(*Field.ValueJson);
								if (HitRadius < 1.0f) HitRadius = 30.0f;
								break;
							}
						}
					}
				}

				// Draw debug sphere at resolved DummyPoly position
				DrawDebugSphere(
					World,
					WorldPos,
					HitRadius,
					12,
					FColor::Red,
					false, // persistent
					-1.0f, // lifetime (single frame)
					0,     // depth priority
					2.0f   // thickness
				);
			}
		}

		// Also check direct behaviorJudgeId → BehaviorParam → DummyPoly chain
		const FString* BehaviorIdStr = Evt.Parameters.Find(TEXT("behaviorJudgeId"));
		if (BehaviorIdStr)
		{
			int32 BehaviorId = FCString::Atoi(**BehaviorIdStr);
			const FSekiroParamRow* BhvRow = CurrentCharacterData->BehaviorParamsPlayer.Find(BehaviorId);
			if (!BhvRow)
			{
				BhvRow = CurrentCharacterData->BehaviorParamsNpc.Find(BehaviorId);
			}
			if (BhvRow)
			{
				// Extract refId (DummyPoly reference) from behavior param
				for (const FSekiroParamField& Field : BhvRow->Fields)
				{
					if (Field.Name == TEXT("refId") || Field.Name == TEXT("dmypolyId"))
					{
						int32 DpId = FCString::Atoi(*Field.ValueJson);
						FVector WorldPos;
						if (ResolveDummyPolyWorldPosition(DpId, WorldPos))
						{
							DrawDebugSphere(
								World,
								WorldPos,
								25.0f,
								8,
								FColor::Yellow,
								false,
								-1.0f,
								0,
								1.5f
							);
						}
					}
				}
			}
		}
	}
}

void ASekiroSkillPreviewActor::UpdateEffectSoundVisualization()
{
	if (!bShowEffectSoundPoints || !CurrentSkillData || !CurrentCharacterData)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<FSekiroTaeEvent> ActiveEvents = CurrentSkillData->GetEventAtFrame(CurrentFrame);

	for (const FSekiroTaeEvent& Evt : ActiveEvents)
	{
		FColor MarkerColor;
		if (Evt.Category == TEXT("Effect"))
		{
			MarkerColor = FColor::Cyan;
		}
		else if (Evt.Category == TEXT("Sound"))
		{
			MarkerColor = FColor::Green;
		}
		else if (Evt.Category == TEXT("WeaponArt") || Evt.Category == TEXT("Prosthetic"))
		{
			MarkerColor = FColor::Magenta;
		}
		else
		{
			continue;
		}

		for (const FSekiroSemanticLink& Link : Evt.SemanticLinks)
		{
			if (!Link.Name.Contains(TEXT("DummyPoly")) && !Link.Name.Contains(TEXT("dummyPoly")))
			{
				continue;
			}

			int32 DpId = FCString::Atoi(*Link.ValueJson);
			FVector WorldPos;
			if (ResolveDummyPolyWorldPosition(DpId, WorldPos))
			{
				DrawDebugPoint(World, WorldPos, 12.0f, MarkerColor, false, -1.0f);

				if (Evt.Category == TEXT("Effect"))
				{
					const FSekiroDummyPoly* Dp = CurrentCharacterData->FindDummyPoly(DpId);
					if (Dp)
					{
						const FName BoneName(*Dp->AttachBoneName);
						const int32 BoneIdx = MeshComp->GetBoneIndex(BoneName);
						if (BoneIdx != INDEX_NONE)
						{
							const FTransform BoneWorldTransform = MeshComp->GetBoneTransform(BoneIdx);
							FVector ForwardDir = BoneWorldTransform.TransformVector(Dp->Forward);
							DrawDebugDirectionalArrow(
								World, WorldPos, WorldPos + ForwardDir * 20.0f,
								5.0f, MarkerColor, false, -1.0f, 0, 1.0f);
						}
					}
				}
			}
		}
	}
}
