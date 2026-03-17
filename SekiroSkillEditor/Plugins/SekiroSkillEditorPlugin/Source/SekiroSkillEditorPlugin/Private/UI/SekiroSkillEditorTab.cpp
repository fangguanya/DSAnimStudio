// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

#include "UI/SekiroSkillEditorTab.h"
#include "UI/SSekiroSkillTimeline.h"
#include "UI/SSekiroEventInspector.h"
#include "UI/SSekiroSkillBrowser.h"
#include "Data/SekiroSkillDataAsset.h"
#include "Data/SekiroTaeEvent.h"

#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"
#include "Styling/AppStyle.h"
#include "Textures/SlateIcon.h"

// ---------------------------------------------------------------------------
// Statics
// ---------------------------------------------------------------------------

const FName FSekiroSkillEditorTab::TabId(TEXT("SekiroSkillEditor"));

TSharedPtr<SSekiroSkillTimeline>  FSekiroSkillEditorTab::Timeline;
TSharedPtr<SSekiroEventInspector> FSekiroSkillEditorTab::Inspector;
TSharedPtr<SSekiroSkillBrowser>   FSekiroSkillEditorTab::Browser;

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------

void FSekiroSkillEditorTab::RegisterTab()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		TabId,
		FOnSpawnTab::CreateStatic(&FSekiroSkillEditorTab::SpawnTab))
		.SetDisplayName(FText::FromString(TEXT("Sekiro Skill Editor")))
		.SetTooltipText(FText::FromString(TEXT("Open the Sekiro Skill / TAE Event timeline editor.")))
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
}

void FSekiroSkillEditorTab::UnregisterTab()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TabId);

	Timeline.Reset();
	Inspector.Reset();
	Browser.Reset();
}

void FSekiroSkillEditorTab::InvokeTab()
{
	FGlobalTabmanager::Get()->TryInvokeTab(TabId);
}

// ---------------------------------------------------------------------------
// Tab spawning
// ---------------------------------------------------------------------------

TSharedRef<SDockTab> FSekiroSkillEditorTab::SpawnTab(const FSpawnTabArgs& SpawnTabArgs)
{
	// ---- Build child widgets ----

	SAssignNew(Browser, SSekiroSkillBrowser)
		.OnSkillSelected_Static(&FSekiroSkillEditorTab::OnSkillSelected);

	SAssignNew(Inspector, SSekiroEventInspector);

	SAssignNew(Timeline, SSekiroSkillTimeline)
		.OnEventSelected_Static(&FSekiroSkillEditorTab::OnEventSelected);

	// ---- Viewport placeholder ----
	TSharedRef<SWidget> ViewportPlaceholder =
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.08f, 0.08f, 0.10f, 1.0f))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("3D Viewport (placeholder)")))
			.ColorAndOpacity(FSlateColor(FLinearColor(0.4f, 0.4f, 0.4f)))
		];

	// ---- Assemble the 3-panel layout via SSplitter ----
	TSharedRef<SWidget> TabContent =
		SNew(SSplitter)
		.Orientation(Orient_Horizontal)

		// Left panel: Skill Browser (20%)
		+ SSplitter::Slot()
		.Value(0.2f)
		[
			Browser.ToSharedRef()
		]

		// Center panel: Viewport + Timeline (60%)
		+ SSplitter::Slot()
		.Value(0.6f)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.FillHeight(0.6f)
			[
				ViewportPlaceholder
			]

			+ SVerticalBox::Slot()
			.FillHeight(0.4f)
			[
				SNew(SScrollBox)
				.Orientation(Orient_Horizontal)
				+ SScrollBox::Slot()
				[
					Timeline.ToSharedRef()
				]
			]
		]

		// Right panel: Event Inspector (20%)
		+ SSplitter::Slot()
		.Value(0.2f)
		[
			Inspector.ToSharedRef()
		];

	// ---- Create the dock tab ----
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(FText::FromString(TEXT("Sekiro Skill Editor")))
		[
			TabContent
		];
}

// ---------------------------------------------------------------------------
// Delegate wiring
// ---------------------------------------------------------------------------

void FSekiroSkillEditorTab::OnSkillSelected(USekiroSkillDataAsset* InSkillData)
{
	if (Timeline.IsValid())
	{
		Timeline->SetSkillData(InSkillData);
	}

	// Clear the inspector when switching skills
	if (Inspector.IsValid())
	{
		Inspector->ClearEvent();
	}
}

void FSekiroSkillEditorTab::OnEventSelected(const FSekiroTaeEvent& InEvent)
{
	if (Inspector.IsValid())
	{
		Inspector->SetEvent(InEvent);
	}
}
