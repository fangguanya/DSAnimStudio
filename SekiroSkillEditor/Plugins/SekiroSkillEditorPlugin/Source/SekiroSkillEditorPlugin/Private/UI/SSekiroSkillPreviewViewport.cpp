// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

#include "UI/SSekiroSkillPreviewViewport.h"

#include "AdvancedPreviewScene.h"
#include "EditorViewportClient.h"
#include "Engine/World.h"
#include "Data/SekiroSkillDataAsset.h"
#include "Visualization/SekiroSkillPreviewActor.h"

namespace
{
	class FSekiroSkillPreviewViewportClient : public FEditorViewportClient
	{
	public:
		FSekiroSkillPreviewViewportClient(FPreviewScene& InPreviewScene, const TSharedRef<SEditorViewport>& InViewport)
			: FEditorViewportClient(nullptr, &InPreviewScene, InViewport)
		{
			SetViewMode(VMI_Lit);
			SetViewportType(LVT_Perspective);
			SetRealtime(true);
			SetViewLocation(FVector(-200.0f, 120.0f, 120.0f));
			SetViewRotation(FRotator(-10.0f, -25.0f, 0.0f));
			bSetListenerPosition = false;
		}

		virtual void Tick(float DeltaSeconds) override
		{
			FEditorViewportClient::Tick(DeltaSeconds);
			if (PreviewScene && PreviewScene->GetWorld())
			{
				PreviewScene->GetWorld()->Tick(LEVELTICK_All, DeltaSeconds);
			}
		}
	};
}

void SSekiroSkillPreviewViewport::Construct(const FArguments& InArgs)
{
	PreviewScene = MakeUnique<FAdvancedPreviewScene>(FPreviewScene::ConstructionValues());
	SEditorViewport::Construct(SEditorViewport::FArguments());
	EnsurePreviewActor();
}

void SSekiroSkillPreviewViewport::SetSkillData(USekiroSkillDataAsset* InSkillData)
{
	CurrentSkillData = InSkillData;
	EnsurePreviewActor();
	if (PreviewActor)
	{
		PreviewActor->SetSkillData(InSkillData);
	}
}

void SSekiroSkillPreviewViewport::SetCurrentFrame(float InFrame)
{
	if (PreviewActor)
	{
		PreviewActor->SetPlaybackPosition(InFrame);
	}
}

void SSekiroSkillPreviewViewport::SetPlaybackEnabled(bool bInIsPlaying)
{
	if (PreviewActor)
	{
		PreviewActor->bIsPlaying = bInIsPlaying;
	}
}

void SSekiroSkillPreviewViewport::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(PreviewActor);
	if (CurrentSkillData.IsValid())
	{
		USekiroSkillDataAsset* CurrentSkillDataPtr = CurrentSkillData.Get();
		Collector.AddReferencedObject(CurrentSkillDataPtr);
	}
}

FString SSekiroSkillPreviewViewport::GetReferencerName() const
{
	return TEXT("SSekiroSkillPreviewViewport");
}

TSharedRef<FEditorViewportClient> SSekiroSkillPreviewViewport::MakeEditorViewportClient()
{
	ViewportClient = MakeShared<FSekiroSkillPreviewViewportClient>(*PreviewScene, SharedThis(this));
	return ViewportClient.ToSharedRef();
}

void SSekiroSkillPreviewViewport::BindCommands()
{
	SEditorViewport::BindCommands();
}

void SSekiroSkillPreviewViewport::EnsurePreviewActor()
{
	if (PreviewActor || !PreviewScene.IsValid() || !PreviewScene->GetWorld())
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.ObjectFlags = RF_Transient;
	PreviewActor = PreviewScene->GetWorld()->SpawnActor<ASekiroSkillPreviewActor>(SpawnParameters);
}