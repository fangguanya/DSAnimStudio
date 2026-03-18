// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AdvancedPreviewScene.h"
#include "SEditorViewport.h"
#include "UObject/GCObject.h"

class FEditorViewportClient;
class USekiroSkillDataAsset;
class ASekiroSkillPreviewActor;

class SEKIROSKILLEDITORPLUGIN_API SSekiroSkillPreviewViewport : public SEditorViewport, public FGCObject
{
public:
	SLATE_BEGIN_ARGS(SSekiroSkillPreviewViewport) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetSkillData(USekiroSkillDataAsset* InSkillData);
	void SetCurrentFrame(float InFrame);
	void SetPlaybackEnabled(bool bInIsPlaying);

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override;

protected:
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
	virtual void BindCommands() override;

private:
	void EnsurePreviewActor();

	TUniquePtr<FAdvancedPreviewScene> PreviewScene;
	TSharedPtr<FEditorViewportClient> ViewportClient;
	TObjectPtr<ASekiroSkillPreviewActor> PreviewActor;
	TWeakObjectPtr<USekiroSkillDataAsset> CurrentSkillData;
};