// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

#include "SekiroSkillEditorPluginModule.h"
#include "UI/SekiroSkillEditorTab.h"
#include "LevelEditor.h"
#include "ToolMenus.h"
#include "Framework/Docking/TabManager.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "SekiroSkillEditorPlugin"

// ---------------------------------------------------------------------------
// IModuleInterface
// ---------------------------------------------------------------------------

void FSekiroSkillEditorPluginModule::StartupModule()
{
	FSekiroSkillEditorTab::RegisterTab();

	// Defer toolbar extension until ToolMenus is ready.
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FSekiroSkillEditorPluginModule::RegisterToolbarExtension));
}

void FSekiroSkillEditorPluginModule::ShutdownModule()
{
	FSekiroSkillEditorTab::UnregisterTab();

	// Clean up toolbar extension.
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
	ToolbarExtender.Reset();
}

// ---------------------------------------------------------------------------
// Toolbar extension
// ---------------------------------------------------------------------------

void FSekiroSkillEditorPluginModule::RegisterToolbarExtension()
{
	UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
	if (!ToolbarMenu)
	{
		return;
	}

	FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("SekiroSkillEditor");
	Section.AddEntry(
		FToolMenuEntry::InitToolBarButton(
			"OpenSekiroSkillEditor",
			FUIAction(
				FExecuteAction::CreateLambda([]()
				{
					FSekiroSkillEditorTab::InvokeTab();
				})),
			LOCTEXT("ToolbarButtonLabel", "Sekiro Skills"),
			LOCTEXT("ToolbarButtonTooltip", "Open the Sekiro Skill Editor tab"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Viewports")));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSekiroSkillEditorPluginModule, SekiroSkillEditorPlugin)
