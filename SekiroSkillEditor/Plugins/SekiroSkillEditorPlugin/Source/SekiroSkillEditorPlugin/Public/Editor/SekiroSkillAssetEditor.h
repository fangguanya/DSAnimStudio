// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"

class IDetailsView;
class SSekiroSkillTimeline;
class SSekiroEventInspector;
class SSekiroSkillPreviewViewport;
class USekiroSkillDataAsset;
struct FSekiroTaeEvent;

class SEKIROSKILLEDITORPLUGIN_API FSekiroSkillAssetEditor : public FAssetEditorToolkit
{
public:
	void InitSkillAssetEditor(
		const EToolkitMode::Type Mode,
		const TSharedPtr<IToolkitHost>& InitToolkitHost,
		USekiroSkillDataAsset* InSkillAsset);

	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager) override;

private:
	static const FName ViewportTabId;
	static const FName TimelineTabId;
	static const FName InspectorTabId;

	TSharedRef<class SDockTab> SpawnViewportTab(const class FSpawnTabArgs& Args);
	TSharedRef<class SDockTab> SpawnTimelineTab(const class FSpawnTabArgs& Args);
	TSharedRef<class SDockTab> SpawnInspectorTab(const class FSpawnTabArgs& Args);

	void OnEventSelected(const FSekiroTaeEvent& InEvent);
	void OnFrameScrubbed(float InFrame);
	void RefreshSkillData();

	TObjectPtr<USekiroSkillDataAsset> SkillAsset;
	TSharedPtr<IDetailsView> DetailsView;
	TSharedPtr<SSekiroSkillTimeline> Timeline;
	TSharedPtr<SSekiroEventInspector> Inspector;
	TSharedPtr<SSekiroSkillPreviewViewport> PreviewViewport;
};