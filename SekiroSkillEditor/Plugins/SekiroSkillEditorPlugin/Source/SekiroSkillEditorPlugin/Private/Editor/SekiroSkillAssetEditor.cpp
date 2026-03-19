// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

#include "Editor/SekiroSkillAssetEditor.h"

#include "Data/SekiroSkillDataAsset.h"
#include "UI/SSekiroEventInspector.h"
#include "UI/SSekiroSkillPreviewViewport.h"
#include "UI/SSekiroSkillTimeline.h"

#include "Framework/Docking/TabManager.h"
#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Styling/AppStyle.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"

#define LOCTEXT_NAMESPACE "SekiroSkillAssetEditor"

const FName FSekiroSkillAssetEditor::ViewportTabId(TEXT("SekiroSkillAssetEditor_Viewport"));
const FName FSekiroSkillAssetEditor::TimelineTabId(TEXT("SekiroSkillAssetEditor_Timeline"));
const FName FSekiroSkillAssetEditor::InspectorTabId(TEXT("SekiroSkillAssetEditor_Inspector"));

void FSekiroSkillAssetEditor::InitSkillAssetEditor(
	const EToolkitMode::Type Mode,
	const TSharedPtr<IToolkitHost>& InitToolkitHost,
	USekiroSkillDataAsset* InSkillAsset)
{
	SkillAsset = InSkillAsset;

	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout(TEXT("SekiroSkillAssetEditorLayout_v1"))
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Horizontal)
			->Split
			(
				FTabManager::NewSplitter()
				->SetOrientation(Orient_Vertical)
				->Split
				(
					FTabManager::NewStack()
				->SetSizeCoefficient(0.68f)
				->AddTab(ViewportTabId, ETabState::OpenedTab)
				->SetForegroundTab(ViewportTabId)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.32f)
					->AddTab(TimelineTabId, ETabState::OpenedTab)
					->SetForegroundTab(TimelineTabId)
				)
			)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.32f)
				->AddTab(InspectorTabId, ETabState::OpenedTab)
				->SetForegroundTab(InspectorTabId)
			)
		);

	InitAssetEditor(Mode, InitToolkitHost, GetToolkitFName(), Layout, true, true, InSkillAsset);
	RefreshSkillData();
}

FName FSekiroSkillAssetEditor::GetToolkitFName() const
{
	return TEXT("SekiroSkillAssetEditor");
}

FText FSekiroSkillAssetEditor::GetBaseToolkitName() const
{
	return LOCTEXT("AppLabel", "Sekiro Skill Editor");
}

FString FSekiroSkillAssetEditor::GetWorldCentricTabPrefix() const
{
	return TEXT("SekiroSkill");
}

FLinearColor FSekiroSkillAssetEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor(0.15f, 0.15f, 0.18f, 1.0f);
}

void FSekiroSkillAssetEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(ViewportTabId, FOnSpawnTab::CreateSP(this, &FSekiroSkillAssetEditor::SpawnViewportTab))
		.SetDisplayName(LOCTEXT("ViewportTab", "Preview"))
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Viewports"));

	InTabManager->RegisterTabSpawner(TimelineTabId, FOnSpawnTab::CreateSP(this, &FSekiroSkillAssetEditor::SpawnTimelineTab))
		.SetDisplayName(LOCTEXT("TimelineTab", "Timeline"))
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelSequenceEditor.Timeline"));

	InTabManager->RegisterTabSpawner(InspectorTabId, FOnSpawnTab::CreateSP(this, &FSekiroSkillAssetEditor::SpawnInspectorTab))
		.SetDisplayName(LOCTEXT("InspectorTab", "Inspector"))
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
}

void FSekiroSkillAssetEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	InTabManager->UnregisterTabSpawner(ViewportTabId);
	InTabManager->UnregisterTabSpawner(TimelineTabId);
	InTabManager->UnregisterTabSpawner(InspectorTabId);

	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
}

TSharedRef<SDockTab> FSekiroSkillAssetEditor::SpawnViewportTab(const FSpawnTabArgs& Args)
{
	SAssignNew(PreviewViewport, SSekiroSkillPreviewViewport);
	if (SkillAsset)
	{
		PreviewViewport->SetSkillData(SkillAsset);
		PreviewViewport->SetCurrentFrame(0.0f);
	}

	return SNew(SDockTab)
		.Label(LOCTEXT("ViewportTitle", "Preview"))
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			[
				PreviewViewport.ToSharedRef()
			]
		];
}

TSharedRef<SDockTab> FSekiroSkillAssetEditor::SpawnTimelineTab(const FSpawnTabArgs& Args)
{
	SAssignNew(Timeline, SSekiroSkillTimeline)
		.OnEventSelected(FOnEventSelected::CreateSP(this, &FSekiroSkillAssetEditor::OnEventSelected))
		.OnFrameScrubbed(FOnFrameScrubbed::CreateSP(this, &FSekiroSkillAssetEditor::OnFrameScrubbed));

	if (SkillAsset)
	{
		Timeline->SetSkillData(SkillAsset);
		Timeline->SetCurrentFrame(0.0f);
	}

	return SNew(SDockTab)
		.Label(LOCTEXT("TimelineTitle", "Timeline"))
		[
			SNew(SScrollBox)
			.Orientation(Orient_Horizontal)
			+ SScrollBox::Slot()
			[
				Timeline.ToSharedRef()
			]
		];
}

TSharedRef<SDockTab> FSekiroSkillAssetEditor::SpawnInspectorTab(const FSpawnTabArgs& Args)
{
	if (!DetailsView.IsValid())
	{
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		FDetailsViewArgs DetailsArgs;
		DetailsArgs.bAllowSearch = true;
		DetailsArgs.bHideSelectionTip = true;
		DetailsArgs.bUpdatesFromSelection = false;
		DetailsArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
		DetailsArgs.ViewIdentifier = TEXT("SekiroSkillAssetEditor_Details");

		DetailsView = PropertyEditorModule.CreateDetailView(DetailsArgs);
		DetailsView->SetObject(SkillAsset);
	}

	SAssignNew(Inspector, SSekiroEventInspector);
	return SNew(SDockTab)
		.Label(LOCTEXT("InspectorTitle", "Inspector"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(0.6f)
			[
				DetailsView.ToSharedRef()
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 6.0f)
			[
				SNew(SSeparator)
			]
			+ SVerticalBox::Slot()
			.FillHeight(0.4f)
			[
				Inspector.ToSharedRef()
			]
		];
}

void FSekiroSkillAssetEditor::OnEventSelected(const FSekiroTaeEvent& InEvent)
{
	if (Inspector.IsValid())
	{
		Inspector->SetEvent(InEvent);
	}
}

void FSekiroSkillAssetEditor::OnFrameScrubbed(float InFrame)
{
	if (PreviewViewport.IsValid())
	{
		PreviewViewport->SetCurrentFrame(InFrame);
	}
}

void FSekiroSkillAssetEditor::RefreshSkillData()
{
	if (Timeline.IsValid())
	{
		Timeline->SetSkillData(SkillAsset);
		Timeline->SetCurrentFrame(0.0f);
	}

	if (PreviewViewport.IsValid())
	{
		PreviewViewport->SetSkillData(SkillAsset);
		PreviewViewport->SetCurrentFrame(0.0f);
	}

	if (Inspector.IsValid())
	{
		Inspector->ClearEvent();
	}

	if (DetailsView.IsValid())
	{
		DetailsView->SetObject(SkillAsset);
	}
}

#undef LOCTEXT_NAMESPACE