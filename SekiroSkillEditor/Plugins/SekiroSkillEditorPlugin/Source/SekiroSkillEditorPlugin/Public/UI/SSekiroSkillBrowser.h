// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Views/STreeView.h"
#include "Data/SekiroSkillDataAsset.h"

/**
 * Delegate fired when a skill asset is selected in the browser.
 */
DECLARE_DELEGATE_OneParam(FOnSkillSelected, USekiroSkillDataAsset* /*SelectedSkill*/);

/**
 * Delegate fired when a skill asset is double-clicked in the browser.
 * Used to open the dedicated asset editor.
 */
DECLARE_DELEGATE_OneParam(FOnSkillDoubleClicked, USekiroSkillDataAsset* /*DoubleClickedSkill*/);

/**
 * A tree-structured item used in the skill browser.
 * Root-level items are character groups; children are individual skills.
 */
struct FSekiroSkillBrowserItem
{
	/** Display name (CharacterId for groups, AnimationName for leaves). */
	FString DisplayName;

	/** If this is a leaf node, the asset it points to. */
	TWeakObjectPtr<USekiroSkillDataAsset> SkillAsset;

	/** Child items (non-empty only for group nodes). */
	TArray<TSharedPtr<FSekiroSkillBrowserItem>> Children;

	/** Is this a group header (true) or a leaf skill (false)? */
	bool bIsGroup = false;
};

/**
 * Browser widget that scans the project for USekiroSkillDataAsset assets
 * and displays them in a tree grouped by CharacterId.
 */
class SEKIROSKILLEDITORPLUGIN_API SSekiroSkillBrowser : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSekiroSkillBrowser) {}
		SLATE_EVENT(FOnSkillSelected, OnSkillSelected)
		SLATE_EVENT(FOnSkillDoubleClicked, OnSkillDoubleClicked)
	SLATE_END_ARGS()

	/** Construct this widget. */
	void Construct(const FArguments& InArgs);

	/** Refresh the asset list from the Asset Registry. */
	void RefreshAssetList();

private:
	// ---- STreeView callbacks ----

	TSharedRef<ITableRow> OnGenerateRow(
		TSharedPtr<FSekiroSkillBrowserItem> Item,
		const TSharedRef<STableViewBase>& OwnerTable);

	void OnGetChildren(
		TSharedPtr<FSekiroSkillBrowserItem> Item,
		TArray<TSharedPtr<FSekiroSkillBrowserItem>>& OutChildren);

	void OnSelectionChanged(
		TSharedPtr<FSekiroSkillBrowserItem> Item,
		ESelectInfo::Type SelectInfo);

	void OnItemDoubleClicked(
		TSharedPtr<FSekiroSkillBrowserItem> Item);

	// ---- Data ----

	/** Root items in the tree (one per CharacterId). */
	TArray<TSharedPtr<FSekiroSkillBrowserItem>> RootItems;

	/** The tree view widget. */
	TSharedPtr<STreeView<TSharedPtr<FSekiroSkillBrowserItem>>> TreeView;

	/** Selection delegate. */
	FOnSkillSelected OnSkillSelected;

	/** Double-click delegate. */
	FOnSkillDoubleClicked OnSkillDoubleClicked;
};
