// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

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
	/** Registers the Sekiro Skill Editor nomad tab with the global tab manager. */
	void RegisterEditorTab();

	/** Adds a toolbar button that opens the Sekiro Skill Editor tab. */
	void RegisterToolbarExtension();

	/** Handle returned by the toolbar extender registration; used for cleanup. */
	TSharedPtr<FExtender> ToolbarExtender;
};
