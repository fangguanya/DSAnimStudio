// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class SSekiroSkillTimeline;
class SSekiroEventInspector;
class SSekiroSkillBrowser;
class SSekiroSkillPreviewViewport;
class USekiroSkillDataAsset;
struct FSekiroTaeEvent;

/**
 * Manages the registration and spawning of the main Sekiro Skill Editor tab.
 *
 * Registers a nomad tab spawner with FGlobalTabmanager under the ID
 * "SekiroSkillEditor". The tab contains a 3-panel layout:
 *   Left   (20%) : SSekiroSkillBrowser
 *   Center (60%) : viewport placeholder + SSekiroSkillTimeline
 *   Right  (20%) : SSekiroEventInspector
 *
 * Call RegisterTab() once during module startup and UnregisterTab() on shutdown.
 */
class SEKIROSKILLEDITORPLUGIN_API FSekiroSkillEditorTab
{
public:
	/** Tab identifier used with FGlobalTabmanager. */
	static const FName TabId;

	/** Register the tab spawner. Call from module StartupModule(). */
	static void RegisterTab();

	/** Unregister the tab spawner. Call from module ShutdownModule(). */
	static void UnregisterTab();

	/** Programmatically open / focus the tab. */
	static void InvokeTab();

private:
	/** Factory callback for FGlobalTabmanager. */
	static TSharedRef<SDockTab> SpawnTab(const FSpawnTabArgs& SpawnTabArgs);

	/** Delegate wiring: browser selection -> timeline. */
	static void OnSkillSelected(USekiroSkillDataAsset* InSkillData);

	/** Delegate wiring: timeline event selection -> inspector. */
	static void OnEventSelected(const FSekiroTaeEvent& InEvent);

	/** Delegate wiring: timeline scrubbing -> preview actor. */
	static void OnFrameScrubbed(float InFrame);

	/** Delegate wiring: browser double-click -> open asset editor. */
	static void OnSkillDoubleClicked(USekiroSkillDataAsset* InSkillData);

	// ---- Persistent widget pointers (valid while tab is open) ----

	static TSharedPtr<SSekiroSkillTimeline>   Timeline;
	static TSharedPtr<SSekiroEventInspector>  Inspector;
	static TSharedPtr<SSekiroSkillBrowser>    Browser;
	static TSharedPtr<SSekiroSkillPreviewViewport> PreviewViewport;
};
