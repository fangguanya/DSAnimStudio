// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

#include "SekiroSkillEditorPluginModule.h"
#include "Editor/SekiroSkillAssetEditor.h"
#include "Data/SekiroSkillDataAsset.h"
#include "UI/SekiroSkillEditorTab.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "AssetTypeActions_Base.h"
#include "LevelEditor.h"
#include "ToolMenus.h"
#include "Framework/Docking/TabManager.h"
#include "Interfaces/IPluginManager.h"

namespace
{
	class FSekiroSkillAssetTypeActions : public FAssetTypeActions_Base
	{
	public:
		virtual FText GetName() const override
		{
			return NSLOCTEXT("SekiroSkillAssetTypeActions", "SkillAssetName", "Sekiro Skill");
		}

		virtual FColor GetTypeColor() const override
		{
			return FColor(188, 98, 54);
		}

		virtual UClass* GetSupportedClass() const override
		{
			return USekiroSkillDataAsset::StaticClass();
		}

		virtual uint32 GetCategories() override
		{
			return EAssetTypeCategories::Misc;
		}

		virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor) override
		{
			for (UObject* Object : InObjects)
			{
				if (USekiroSkillDataAsset* SkillAsset = Cast<USekiroSkillDataAsset>(Object))
				{
					const TSharedRef<FSekiroSkillAssetEditor> Editor = MakeShared<FSekiroSkillAssetEditor>();
					Editor->InitSkillAssetEditor(EToolkitMode::Standalone, EditWithinLevelEditor, SkillAsset);
				}
			}
		}
	};
}

#define LOCTEXT_NAMESPACE "SekiroSkillEditorPlugin"

// ---------------------------------------------------------------------------
// IModuleInterface
// ---------------------------------------------------------------------------

void FSekiroSkillEditorPluginModule::StartupModule()
{
	FSekiroSkillEditorTab::RegisterTab();

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	const TSharedRef<IAssetTypeActions> SkillAssetTypeActions = MakeShared<FSekiroSkillAssetTypeActions>();
	AssetToolsModule.Get().RegisterAssetTypeActions(SkillAssetTypeActions);
	RegisteredAssetTypeActions.Add(SkillAssetTypeActions);

	// Defer toolbar extension until ToolMenus is ready.
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FSekiroSkillEditorPluginModule::RegisterToolbarExtension));
}

void FSekiroSkillEditorPluginModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
		for (const TSharedPtr<IAssetTypeActions>& AssetTypeActions : RegisteredAssetTypeActions)
		{
			if (AssetTypeActions.IsValid())
			{
				AssetToolsModule.Get().UnregisterAssetTypeActions(AssetTypeActions.ToSharedRef());
			}
		}
	}
	RegisteredAssetTypeActions.Reset();

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
