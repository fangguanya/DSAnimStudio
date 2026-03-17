// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

#include "SekiroSkillEditorPluginModule.h"
#include "LevelEditor.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "SekiroSkillEditorPlugin"

static const FName SekiroSkillEditorTabName(TEXT("SekiroSkillEditorTab"));

// ---------------------------------------------------------------------------
// IModuleInterface
// ---------------------------------------------------------------------------

void FSekiroSkillEditorPluginModule::StartupModule()
{
	RegisterEditorTab();

	// Defer toolbar extension until ToolMenus is ready.
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FSekiroSkillEditorPluginModule::RegisterToolbarExtension));
}

void FSekiroSkillEditorPluginModule::ShutdownModule()
{
	// Unregister the nomad tab spawner.
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(SekiroSkillEditorTabName);

	// Clean up toolbar extension.
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
	ToolbarExtender.Reset();
}

// ---------------------------------------------------------------------------
// Editor tab
// ---------------------------------------------------------------------------

void FSekiroSkillEditorPluginModule::RegisterEditorTab()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		SekiroSkillEditorTabName,
		FOnSpawnTab::CreateLambda([](const FSpawnTabArgs& /*Args*/) -> TSharedRef<SDockTab>
		{
			return SNew(SDockTab)
				.TabRole(ETabRole::NomadTab)
				.Label(LOCTEXT("TabTitle", "Sekiro Skill Editor"))
				[
					SNew(STextBlock)
						.Text(LOCTEXT("Placeholder", "Sekiro Skill Editor — content will be populated by the UI module."))
				];
		}))
		.SetDisplayName(LOCTEXT("TabDisplayName", "Sekiro Skill Editor"))
		.SetMenuType(ETabSpawnerMenuType::Enabled)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Viewports"));
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
					FGlobalTabmanager::Get()->TryInvokeTab(SekiroSkillEditorTabName);
				})),
			LOCTEXT("ToolbarButtonLabel", "Sekiro Skills"),
			LOCTEXT("ToolbarButtonTooltip", "Open the Sekiro Skill Editor tab"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Viewports")));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSekiroSkillEditorPluginModule, SekiroSkillEditorPlugin)
