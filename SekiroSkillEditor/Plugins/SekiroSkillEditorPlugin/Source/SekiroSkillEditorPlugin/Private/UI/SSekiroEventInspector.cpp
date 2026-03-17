// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

#include "UI/SSekiroEventInspector.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Styling/CoreStyle.h"

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

void SSekiroEventInspector::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.Padding(4.0f)
		[
			SAssignNew(ScrollBox, SScrollBox)
			+ SScrollBox::Slot()
			[
				SAssignNew(ContentBox, SVerticalBox)
			]
		]
	];

	RebuildContent();
}

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------

void SSekiroEventInspector::SetEvent(const FSekiroTaeEvent& InEvent)
{
	CurrentEvent = InEvent;
	bHasEvent = true;
	RebuildContent();
}

void SSekiroEventInspector::ClearEvent()
{
	CurrentEvent.Reset();
	bHasEvent = false;
	RebuildContent();
}

// ---------------------------------------------------------------------------
// Content building
// ---------------------------------------------------------------------------

void SSekiroEventInspector::RebuildContent()
{
	if (!ContentBox.IsValid())
	{
		return;
	}

	ContentBox->ClearChildren();

	if (!bHasEvent || !CurrentEvent.IsSet())
	{
		ContentBox->AddSlot()
		.AutoHeight()
		.Padding(4.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("No event selected.")))
			.ColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)))
		];
		return;
	}

	const FSekiroTaeEvent& Evt = CurrentEvent.GetValue();

	// ---- Title ----
	ContentBox->AddSlot()
	.AutoHeight()
	.Padding(4.0f, 4.0f, 4.0f, 8.0f)
	[
		SNew(STextBlock)
		.Text(FText::FromString(TEXT("Event Inspector")))
		.Font(FCoreStyle::Get().GetFontStyle("NormalFont"))
		.ColorAndOpacity(FSlateColor(FLinearColor::White))
	];

	// ---- Fixed properties ----
	int32 RowIndex = 0;

	ContentBox->AddSlot().AutoHeight()
	[
		MakePropertyRow(TEXT("Type"), FString::FromInt(Evt.Type), (RowIndex++ % 2 == 0))
	];

	ContentBox->AddSlot().AutoHeight()
	[
		MakePropertyRow(TEXT("TypeName"), Evt.TypeName, (RowIndex++ % 2 == 0))
	];

	ContentBox->AddSlot().AutoHeight()
	[
		MakePropertyRow(TEXT("Category"), Evt.Category, (RowIndex++ % 2 == 0))
	];

	ContentBox->AddSlot().AutoHeight()
	[
		MakePropertyRow(
			TEXT("Frame Range"),
			FString::Printf(TEXT("%.1f - %.1f"), Evt.StartFrame, Evt.EndFrame),
			(RowIndex++ % 2 == 0))
	];

	// ---- Separator ----
	ContentBox->AddSlot()
	.AutoHeight()
	.Padding(4.0f, 6.0f)
	[
		SNew(STextBlock)
		.Text(FText::FromString(TEXT("Parameters")))
		.Font(FCoreStyle::Get().GetFontStyle("NormalFont"))
		.ColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.8f, 0.6f)))
	];

	// ---- Parameter rows ----
	if (Evt.Parameters.Num() == 0)
	{
		ContentBox->AddSlot()
		.AutoHeight()
		.Padding(8.0f, 2.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("(none)")))
			.ColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)))
		];
	}
	else
	{
		// Sort keys for stable display order
		TArray<FString> SortedKeys;
		Evt.Parameters.GetKeys(SortedKeys);
		SortedKeys.Sort();

		for (const FString& Key : SortedKeys)
		{
			const FString& Value = Evt.Parameters[Key];
			ContentBox->AddSlot().AutoHeight()
			[
				MakePropertyRow(Key, Value, (RowIndex++ % 2 == 0))
			];
		}
	}
}

TSharedRef<SWidget> SSekiroEventInspector::MakePropertyRow(
	const FString& Label,
	const FString& Value,
	bool bAlternateColor) const
{
	const FLinearColor BgColor = bAlternateColor
		? FLinearColor(0.15f, 0.15f, 0.15f, 1.0f)
		: FLinearColor(0.12f, 0.12f, 0.12f, 1.0f);

	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(BgColor)
		.Padding(FMargin(8.0f, 2.0f))
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.FillWidth(0.4f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Label))
				.ColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.7f, 0.8f)))
			]

			+ SHorizontalBox::Slot()
			.FillWidth(0.6f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Value))
				.ColorAndOpacity(FSlateColor(FLinearColor::White))
			]
		];
}
