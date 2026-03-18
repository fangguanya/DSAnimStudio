// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class IAssetTypeActions;

/**
 * Main module for the Sekiro Skill Editor plugin.
 * Registers the editor tab, toolbar extension, and associated types.
 */
class FSekiroSkillEditorPluginModule : public IModuleInterface
{
public:
	// -- IModuleInterface overrides ------------------------------------------

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	/** Adds a toolbar button that opens the Sekiro Skill Editor tab. */
	void RegisterToolbarExtension();

	/** Handle returned by the toolbar extender registration; used for cleanup. */
	TSharedPtr<FExtender> ToolbarExtender;
	TArray<TSharedPtr<IAssetTypeActions>> RegisteredAssetTypeActions;
};
