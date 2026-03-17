// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

#include "Visualization/SekiroHitboxDrawComponent.h"
#include "Visualization/SekiroSkillPreviewActor.h"
#include "Data/SekiroCharacterData.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"

namespace
{
	static bool TryGetIntParam(const FSekiroTaeEvent& Event, const TCHAR* Name, int32& OutValue)
	{
		if (const FSekiroEventParam* Param = Event.FindParam(Name))
		{
			OutValue = FCString::Atoi(*Param->ValueJson);
			return true;
		}

		if (const FString* LegacyValue = Event.Parameters.Find(Name))
		{
			OutValue = FCString::Atoi(**LegacyValue);
			return true;
		}

		return false;
	}

	static bool TryGetFloatParam(const FSekiroTaeEvent& Event, const TCHAR* Name, float& OutValue)
	{
		if (const FSekiroEventParam* Param = Event.FindParam(Name))
		{
			OutValue = FCString::Atof(*Param->ValueJson);
			return true;
		}

		if (const FString* LegacyValue = Event.Parameters.Find(Name))
		{
			OutValue = FCString::Atof(**LegacyValue);
			return true;
		}

		return false;
	}

	static bool ResolveDummyPolyWorldPosition(const ASekiroSkillPreviewActor* PreviewActor, int32 DummyPolyId, FVector& OutWorldPosition)
	{
		if (!PreviewActor || !PreviewActor->CurrentCharacterData || !PreviewActor->MeshComp)
		{
			return false;
		}

		const FSekiroDummyPoly* DummyPoly = PreviewActor->CurrentCharacterData->FindDummyPoly(DummyPolyId);
		if (!DummyPoly)
		{
			return false;
		}

		const FName BoneName = !DummyPoly->AttachBoneName.IsEmpty() ? FName(*DummyPoly->AttachBoneName) : FName(*DummyPoly->ParentBoneName);
		const int32 BoneIndex = PreviewActor->MeshComp->GetBoneIndex(BoneName);
		if (!PreviewActor->MeshComp->DoesSocketExist(BoneName) && BoneIndex == INDEX_NONE)
		{
			return false;
		}

		const FTransform BoneTransform = PreviewActor->MeshComp->DoesSocketExist(BoneName)
			? PreviewActor->MeshComp->GetSocketTransform(BoneName)
			: PreviewActor->MeshComp->GetBoneTransform(BoneIndex);

		OutWorldPosition = BoneTransform.TransformPosition(DummyPoly->LocalPosition);
		return true;
	}

	static FVector ResolveHitboxCenter(const ASekiroSkillPreviewActor* PreviewActor, const FSekiroTaeEvent& Event, int32 EventIndex, float FallbackRadius)
	{
		int32 DummyPolyId = INDEX_NONE;
		if (TryGetIntParam(Event, TEXT("DummyPolyID"), DummyPolyId)
			|| TryGetIntParam(Event, TEXT("DummypolyID"), DummyPolyId)
			|| TryGetIntParam(Event, TEXT("DummyPolyId"), DummyPolyId)
			|| TryGetIntParam(Event, TEXT("DummyPolyID1"), DummyPolyId)
			|| TryGetIntParam(Event, TEXT("DummyPolyBladeBaseID"), DummyPolyId))
		{
			FVector DummyPolyPosition;
			if (ResolveDummyPolyWorldPosition(PreviewActor, DummyPolyId, DummyPolyPosition))
			{
				return DummyPolyPosition;
			}
		}

		const FVector ActorLocation = PreviewActor ? PreviewActor->GetActorLocation() : FVector::ZeroVector;
		return ActorLocation + FVector(0.0f, 0.0f, static_cast<float>(EventIndex) * FallbackRadius * 0.5f);
	}
}

USekiroHitboxDrawComponent::USekiroHitboxDrawComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	bAutoActivate = true;
}

void USekiroHitboxDrawComponent::UpdateActiveEvents(float CurrentFrame, const TArray<FSekiroTaeEvent>& AllEvents)
{
	ActiveEvents.Reset();

	for (const FSekiroTaeEvent& Evt : AllEvents)
	{
		if (Evt.Category == TEXT("Attack") && CurrentFrame >= Evt.StartFrame && CurrentFrame <= Evt.EndFrame)
		{
			ActiveEvents.Add(Evt);
		}
	}
}

void USekiroHitboxDrawComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bShowHitboxes || ActiveEvents.IsEmpty())
	{
		return;
	}

	const ASekiroSkillPreviewActor* PreviewActor = Cast<ASekiroSkillPreviewActor>(GetOwner());
	if (!PreviewActor)
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Semi-transparent red for attack hitboxes.
	const FColor HitboxColor(255, 40, 40, 128);

	for (int32 Idx = 0; Idx < ActiveEvents.Num(); ++Idx)
	{
		const FSekiroTaeEvent& Evt = ActiveEvents[Idx];

		// If the event parameters specify a radius, prefer it.
		float DrawRadius = HitboxRadius;
		if (TryGetFloatParam(Evt, TEXT("Radius"), DrawRadius) && DrawRadius <= UE_KINDA_SMALL_NUMBER)
		{
			DrawRadius = HitboxRadius;
		}

		FVector BaseCenter = ResolveHitboxCenter(PreviewActor, Evt, Idx, HitboxRadius);
		int32 BladeTipDummyPolyId = INDEX_NONE;
		if (TryGetIntParam(Evt, TEXT("DummyPolyBladeTipID"), BladeTipDummyPolyId))
		{
			FVector TipCenter;
			if (ResolveDummyPolyWorldPosition(PreviewActor, BladeTipDummyPolyId, TipCenter))
			{
				const FVector Segment = TipCenter - BaseCenter;
				const float SegmentLength = Segment.Size();
				if (SegmentLength > UE_KINDA_SMALL_NUMBER)
				{
					const FVector CapsuleCenter = BaseCenter + (Segment * 0.5f);
					const FQuat CapsuleRotation = FRotationMatrix::MakeFromZ(Segment.GetSafeNormal()).ToQuat();
					DrawDebugCapsule(World, CapsuleCenter, (SegmentLength * 0.5f) + DrawRadius, DrawRadius, CapsuleRotation, HitboxColor, false, -1.0f);
					continue;
				}
			}
		}

		DrawDebugSphere(World, BaseCenter, DrawRadius, 16, HitboxColor, false, -1.0f);
	}
}
