// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

#include "Visualization/SekiroEffectMarkerComponent.h"
#include "Visualization/SekiroSkillPreviewActor.h"
#include "Data/SekiroCharacterData.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"

namespace
{
	static bool TryGetStringParam(const FSekiroTaeEvent& Event, const TCHAR* Name, FString& OutValue)
	{
		if (const FSekiroEventParam* Param = Event.FindParam(Name))
		{
			OutValue = Param->ValueJson;
			return true;
		}

		if (const FString* LegacyValue = Event.Parameters.Find(Name))
		{
			OutValue = *LegacyValue;
			return true;
		}

		return false;
	}

	static bool TryGetIntParam(const FSekiroTaeEvent& Event, const TCHAR* Name, int32& OutValue)
	{
		FString Value;
		if (!TryGetStringParam(Event, Name, Value))
		{
			return false;
		}

		OutValue = FCString::Atoi(*Value);
		return true;
	}

	static bool ResolveDummyPolyTransform(const ASekiroSkillPreviewActor* PreviewActor, int32 DummyPolyId, FTransform& OutTransform)
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
		OutTransform = FTransform(BoneTransform.GetRotation(), BoneTransform.TransformPosition(DummyPoly->LocalPosition), BoneTransform.GetScale3D());
		return true;
	}

	static FVector ResolveMarkerCenter(const ASekiroSkillPreviewActor* PreviewActor, const FSekiroTaeEvent& Event, int32 EventIndex, float FallbackSize)
	{
		int32 DummyPolyId = INDEX_NONE;
		if (TryGetIntParam(Event, TEXT("DummyPolyID"), DummyPolyId)
			|| TryGetIntParam(Event, TEXT("DummypolyID"), DummyPolyId)
			|| TryGetIntParam(Event, TEXT("DummyPolyId"), DummyPolyId)
			|| TryGetIntParam(Event, TEXT("DummyPolyID1"), DummyPolyId))
		{
			FTransform DummyPolyTransform;
			if (ResolveDummyPolyTransform(PreviewActor, DummyPolyId, DummyPolyTransform))
			{
				return DummyPolyTransform.GetLocation();
			}
		}

		return (PreviewActor ? PreviewActor->GetActorLocation() : FVector::ZeroVector)
			+ FVector(static_cast<float>(EventIndex) * FallbackSize, 0.0f, FallbackSize);
	}
}

USekiroEffectMarkerComponent::USekiroEffectMarkerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	bAutoActivate = true;
}

void USekiroEffectMarkerComponent::UpdateActiveEvents(float CurrentFrame, const TArray<FSekiroTaeEvent>& AllEvents)
{
	ActiveEvents.Reset();

	for (const FSekiroTaeEvent& Evt : AllEvents)
	{
		const bool bInRange = (CurrentFrame >= Evt.StartFrame && CurrentFrame <= Evt.EndFrame);
		const bool bIsEffectOrSound = (Evt.Category == TEXT("Effect") || Evt.Category == TEXT("Sound"));

		if (bInRange && bIsEffectOrSound)
		{
			ActiveEvents.Add(Evt);
		}
	}
}

void USekiroEffectMarkerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bShowMarkers || ActiveEvents.IsEmpty())
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

	// Blue for effect/VFX markers.
	const FColor EffectColor(60, 120, 255, 200);
	// Green for sound triggers.
	const FColor SoundColor(40, 220, 80, 200);

	for (int32 Idx = 0; Idx < ActiveEvents.Num(); ++Idx)
	{
		const FSekiroTaeEvent& Evt = ActiveEvents[Idx];
		const FVector Center = ResolveMarkerCenter(PreviewActor, Evt, Idx, MarkerSize);

		if (Evt.Category == TEXT("Effect"))
		{
			// Blue sphere for effect spawn points.
			DrawDebugSphere(World, Center, MarkerSize, /*Segments=*/ 12, EffectColor, /*bPersistent=*/ false, /*LifeTime=*/ -1.0f);

			// If a trajectory direction is specified, draw a short line to indicate it.
			FString DirX;
			FString DirY;
			FString DirZ;
			if (TryGetStringParam(Evt, TEXT("DirX"), DirX)
				&& TryGetStringParam(Evt, TEXT("DirY"), DirY)
				&& TryGetStringParam(Evt, TEXT("DirZ"), DirZ))
			{
				const FVector Dir(FCString::Atof(*DirX), FCString::Atof(*DirY), FCString::Atof(*DirZ));
				if (!Dir.IsNearlyZero())
				{
					const FVector EndPoint = Center + Dir.GetSafeNormal() * MarkerSize * 3.0f;
					DrawDebugDirectionalArrow(World, Center, EndPoint, MarkerSize * 0.5f, EffectColor, /*bPersistent=*/ false, /*LifeTime=*/ -1.0f);
				}
			}
			else
			{
				int32 DummyPolyId = INDEX_NONE;
				FTransform DummyPolyTransform;
				if ((TryGetIntParam(Evt, TEXT("DummyPolyID"), DummyPolyId)
					|| TryGetIntParam(Evt, TEXT("DummypolyID"), DummyPolyId)
					|| TryGetIntParam(Evt, TEXT("DummyPolyId"), DummyPolyId))
					&& ResolveDummyPolyTransform(PreviewActor, DummyPolyId, DummyPolyTransform))
				{
					const FVector EndPoint = Center + DummyPolyTransform.GetRotation().GetForwardVector() * MarkerSize * 3.0f;
					DrawDebugDirectionalArrow(World, Center, EndPoint, MarkerSize * 0.5f, EffectColor, false, -1.0f);
				}
			}
		}
		else if (Evt.Category == TEXT("Sound"))
		{
			// Green diamond (approximated by a point + small box) for sound triggers.
			const FVector Extent(MarkerSize * 0.5f);
			DrawDebugBox(World, Center, Extent, FQuat(FRotator(45.0f, 45.0f, 0.0f)), SoundColor, /*bPersistent=*/ false, /*LifeTime=*/ -1.0f);
		}
	}
}
