// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

#include "UI/SSekiroSkillBrowser.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STreeView.h"
#include "Styling/CoreStyle.h"

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

void SSekiroSkillBrowser::Construct(const FArguments& InArgs)
{
	OnSkillSelected = InArgs._OnSkillSelected;

	RefreshAssetList();

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.Padding(2.0f)
		[
			SNew(SVerticalBox)

			// Header
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Skill Browser")))
				.Font(FCoreStyle::Get().GetFontStyle("NormalFont"))
				.ColorAndOpacity(FSlateColor(FLinearColor::White))
			]

			// Tree
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SAssignNew(TreeView, STreeView<TSharedPtr<FSekiroSkillBrowserItem>>)
				.TreeItemsSource(&RootItems)
				.OnGenerateRow(this, &SSekiroSkillBrowser::OnGenerateRow)
				.OnGetChildren(this, &SSekiroSkillBrowser::OnGetChildren)
				.OnSelectionChanged(this, &SSekiroSkillBrowser::OnSelectionChanged)
				.SelectionMode(ESelectionMode::Single)
			]
		]
	];

	// Expand all root items by default so character groups are visible
	if (TreeView.IsValid())
	{
		for (const TSharedPtr<FSekiroSkillBrowserItem>& Root : RootItems)
		{
			TreeView->SetItemExpansion(Root, true);
		}
	}
}

// ---------------------------------------------------------------------------
// Asset scanning
// ---------------------------------------------------------------------------

void SSekiroSkillBrowser::RefreshAssetList()
{
	RootItems.Empty();

	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// Find all USekiroSkillDataAsset assets in the project
	TArray<FAssetData> AssetDataList;
	AssetRegistry.GetAssetsByClass(
		USekiroSkillDataAsset::StaticClass()->GetClassPathName(),
		AssetDataList,
		/*bSearchSubClasses=*/ true);

	// Group by CharacterId
	TMap<FString, TSharedPtr<FSekiroSkillBrowserItem>> GroupMap;

	for (const FAssetData& AssetData : AssetDataList)
	{
		USekiroSkillDataAsset* SkillAsset = Cast<USekiroSkillDataAsset>(AssetData.GetAsset());
		if (!SkillAsset)
		{
			continue;
		}

		const FString& CharId = SkillAsset->CharacterId;

		// Find or create group
		TSharedPtr<FSekiroSkillBrowserItem>& GroupItem = GroupMap.FindOrAdd(CharId);
		if (!GroupItem.IsValid())
		{
			GroupItem = MakeShared<FSekiroSkillBrowserItem>();
			GroupItem->DisplayName = CharId.IsEmpty() ? TEXT("(Unknown)") : CharId;
			GroupItem->bIsGroup = true;
			RootItems.Add(GroupItem);
		}

		// Create leaf item
		TSharedPtr<FSekiroSkillBrowserItem> LeafItem = MakeShared<FSekiroSkillBrowserItem>();
		LeafItem->DisplayName = SkillAsset->AnimationName;
		LeafItem->SkillAsset = SkillAsset;
		LeafItem->bIsGroup = false;

		GroupItem->Children.Add(LeafItem);
	}

	// Sort root groups by name
	RootItems.Sort([](const TSharedPtr<FSekiroSkillBrowserItem>& A, const TSharedPtr<FSekiroSkillBrowserItem>& B)
	{
		return A->DisplayName < B->DisplayName;
	});

	// Sort children within each group
	for (TSharedPtr<FSekiroSkillBrowserItem>& Group : RootItems)
	{
		Group->Children.Sort([](const TSharedPtr<FSekiroSkillBrowserItem>& A, const TSharedPtr<FSekiroSkillBrowserItem>& B)
		{
			return A->DisplayName < B->DisplayName;
		});
	}

	if (TreeView.IsValid())
	{
		TreeView->RequestTreeRefresh();
	}
}

// ---------------------------------------------------------------------------
// STreeView callbacks
// ---------------------------------------------------------------------------

TSharedRef<ITableRow> SSekiroSkillBrowser::OnGenerateRow(
	TSharedPtr<FSekiroSkillBrowserItem> Item,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	const bool bIsGroup = Item.IsValid() && Item->bIsGroup;
	const FString DisplayText = Item.IsValid() ? Item->DisplayName : TEXT("???");

	const FLinearColor TextColor = bIsGroup
		? FLinearColor(0.9f, 0.8f, 0.4f)   // Gold for groups
		: FLinearColor(0.85f, 0.85f, 0.85f); // Light gray for leaves

	return SNew(STableRow<TSharedPtr<FSekiroSkillBrowserItem>>, OwnerTable)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(bIsGroup ? 2.0f : 16.0f, 2.0f, 4.0f, 2.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(DisplayText))
				.ColorAndOpacity(FSlateColor(TextColor))
			]
		];
}

void SSekiroSkillBrowser::OnGetChildren(
	TSharedPtr<FSekiroSkillBrowserItem> Item,
	TArray<TSharedPtr<FSekiroSkillBrowserItem>>& OutChildren)
{
	if (Item.IsValid())
	{
		OutChildren = Item->Children;
	}
}

void SSekiroSkillBrowser::OnSelectionChanged(
	TSharedPtr<FSekiroSkillBrowserItem> Item,
	ESelectInfo::Type SelectInfo)
{
	if (!Item.IsValid() || Item->bIsGroup)
	{
		return;
	}

	USekiroSkillDataAsset* Asset = Item->SkillAsset.Get();
	if (Asset)
	{
		OnSkillSelected.ExecuteIfBound(Asset);
	}
}
