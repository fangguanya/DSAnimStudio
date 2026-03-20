// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

#include "UI/SSekiroEventInspector.h"
#include "Data/SekiroCharacterData.h"
#include "Data/SekiroSkillDataAsset.h"
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

void SSekiroEventInspector::SetCharacterData(USekiroCharacterData* InCharacterData)
{
	CharacterData = InCharacterData;
	if (bHasEvent)
	{
		RebuildContent();
	}
}

void SSekiroEventInspector::SetSkillData(USekiroSkillDataAsset* InSkillData)
{
	SkillData = InSkillData;
	if (bHasEvent)
	{
		RebuildContent();
	}
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

		ContentBox->AddSlot()
		.AutoHeight()
		.Padding(4.0f, 6.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Structured Params")))
			.Font(FCoreStyle::Get().GetFontStyle("NormalFont"))
			.ColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.85f, 0.7f)))
		];

		if (Evt.Params.Num() == 0)
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
			for (const FSekiroEventParam& Param : Evt.Params)
			{
				const FString Label = FString::Printf(TEXT("%s [%s @ %d]"), *Param.Name, *Param.DataType, Param.ByteOffset);
				const FString Value = Param.Source.IsEmpty()
					? Param.ValueJson
					: FString::Printf(TEXT("%s | source=%s"), *Param.ValueJson, *Param.Source);
				ContentBox->AddSlot().AutoHeight()
				[
					MakePropertyRow(Label, Value, (RowIndex++ % 2 == 0))
				];
			}
		}

		ContentBox->AddSlot()
		.AutoHeight()
		.Padding(4.0f, 6.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Semantic Links")))
			.Font(FCoreStyle::Get().GetFontStyle("NormalFont"))
			.ColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.75f, 0.55f)))
		];

		if (Evt.SemanticLinks.Num() == 0)
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
			for (const FSekiroSemanticLink& Link : Evt.SemanticLinks)
			{
				ContentBox->AddSlot().AutoHeight()
				[
					MakePropertyRow(Link.Name, Link.ValueJson, (RowIndex++ % 2 == 0))
				];
			}
		}
	}

	// ---- Resolved DummyPoly ----
	BuildResolvedDummyPolySection(Evt, RowIndex);

	// ---- Resolved Parameter Chain ----
	BuildResolvedParamChainSection(Evt, RowIndex);

	// ---- Root-Motion ----
	BuildRootMotionSection(Evt, RowIndex);
}

// ---------------------------------------------------------------------------
// Resolved semantic sections
// ---------------------------------------------------------------------------

void SSekiroEventInspector::BuildResolvedDummyPolySection(const FSekiroTaeEvent& Evt, int32& RowIndex)
{
	// Look for DummyPoly references in semantic links
	TArray<int32> DummyPolyIds;
	for (const FSekiroSemanticLink& Link : Evt.SemanticLinks)
	{
		if (Link.Name.Contains(TEXT("DummyPoly")) || Link.Name.Contains(TEXT("dummyPoly")))
		{
			int32 Id = FCString::Atoi(*Link.ValueJson);
			if (Id != 0 || Link.ValueJson == TEXT("0"))
			{
				DummyPolyIds.Add(Id);
			}
		}
	}

	if (DummyPolyIds.Num() == 0)
	{
		return;
	}

	ContentBox->AddSlot()
	.AutoHeight()
	.Padding(4.0f, 6.0f)
	[
		SNew(STextBlock)
		.Text(FText::FromString(TEXT("Resolved DummyPoly")))
		.Font(FCoreStyle::Get().GetFontStyle("NormalFont"))
		.ColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.9f, 0.6f)))
	];

	USekiroCharacterData* CharData = CharacterData.Get();
	for (int32 DpId : DummyPolyIds)
	{
		if (CharData)
		{
			const FSekiroDummyPoly* Dp = CharData->FindDummyPoly(DpId);
			if (Dp)
			{
				ContentBox->AddSlot().AutoHeight()
				[
					MakePropertyRow(
						FString::Printf(TEXT("DP#%d Bone"), DpId),
						Dp->ParentBoneName,
						(RowIndex++ % 2 == 0))
				];
				ContentBox->AddSlot().AutoHeight()
				[
					MakePropertyRow(
						FString::Printf(TEXT("DP#%d Position"), DpId),
						FString::Printf(TEXT("(%.2f, %.2f, %.2f)"), Dp->LocalPosition.X, Dp->LocalPosition.Y, Dp->LocalPosition.Z),
						(RowIndex++ % 2 == 0))
				];
			}
			else
			{
				ContentBox->AddSlot().AutoHeight()
				[
					MakePropertyRow(
						FString::Printf(TEXT("DP#%d"), DpId),
						TEXT("(not found in character data)"),
						(RowIndex++ % 2 == 0))
				];
			}
		}
		else
		{
			ContentBox->AddSlot().AutoHeight()
			[
				MakePropertyRow(
					FString::Printf(TEXT("DP#%d"), DpId),
					TEXT("(no character data loaded)"),
					(RowIndex++ % 2 == 0))
			];
		}
	}
}

void SSekiroEventInspector::BuildResolvedParamChainSection(const FSekiroTaeEvent& Evt, int32& RowIndex)
{
	// Look for parameter references in semantic links (BehaviorParam, AtkParam, SpEffect, EquipParamWeapon)
	struct FParamRef
	{
		FString ParamType;
		int32 RowId;
	};
	TArray<FParamRef> ParamRefs;

	for (const FSekiroSemanticLink& Link : Evt.SemanticLinks)
	{
		if (Link.Name.Contains(TEXT("BehaviorParam")) || Link.Name.Contains(TEXT("AtkParam")) ||
			Link.Name.Contains(TEXT("SpEffect")) || Link.Name.Contains(TEXT("EquipParamWeapon")))
		{
			int32 Id = FCString::Atoi(*Link.ValueJson);
			if (Id != 0 || Link.ValueJson == TEXT("0"))
			{
				FParamRef Ref;
				if (Link.Name.Contains(TEXT("BehaviorParam"))) Ref.ParamType = TEXT("BehaviorParam");
				else if (Link.Name.Contains(TEXT("AtkParam"))) Ref.ParamType = TEXT("AtkParam");
				else if (Link.Name.Contains(TEXT("SpEffect"))) Ref.ParamType = TEXT("SpEffect");
				else Ref.ParamType = TEXT("EquipParamWeapon");
				Ref.RowId = Id;
				ParamRefs.Add(Ref);
			}
		}
	}

	if (ParamRefs.Num() == 0)
	{
		return;
	}

	ContentBox->AddSlot()
	.AutoHeight()
	.Padding(4.0f, 6.0f)
	[
		SNew(STextBlock)
		.Text(FText::FromString(TEXT("Resolved Parameter Chain")))
		.Font(FCoreStyle::Get().GetFontStyle("NormalFont"))
		.ColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.75f, 0.95f)))
	];

	USekiroCharacterData* CharData = CharacterData.Get();
	for (const FParamRef& Ref : ParamRefs)
	{
		if (!CharData)
		{
			ContentBox->AddSlot().AutoHeight()
			[
				MakePropertyRow(
					FString::Printf(TEXT("%s[%d]"), *Ref.ParamType, Ref.RowId),
					TEXT("(no character data loaded)"),
					(RowIndex++ % 2 == 0))
			];
			continue;
		}

		// Look up the param row in the appropriate cache
		const TMap<int32, FSekiroParamRow>* ParamMap = nullptr;
		if (Ref.ParamType == TEXT("BehaviorParam"))
			ParamMap = &CharData->BehaviorParamsPlayer;
		else if (Ref.ParamType == TEXT("AtkParam"))
			ParamMap = &CharData->AtkParamsPlayer;
		else if (Ref.ParamType == TEXT("SpEffect"))
			ParamMap = &CharData->SpEffectParams;
		else if (Ref.ParamType == TEXT("EquipParamWeapon"))
			ParamMap = &CharData->EquipParamWeapon;

		if (ParamMap)
		{
			const FSekiroParamRow* Row = ParamMap->Find(Ref.RowId);
			if (Row)
			{
				ContentBox->AddSlot().AutoHeight()
				[
					MakePropertyRow(
						FString::Printf(TEXT("%s[%d]"), *Ref.ParamType, Ref.RowId),
						Row->Name,
						(RowIndex++ % 2 == 0))
				];
				for (const FSekiroParamField& Field : Row->Fields)
				{
					ContentBox->AddSlot().AutoHeight()
					[
						MakePropertyRow(
							FString::Printf(TEXT("  %s"), *Field.Name),
							Field.ValueJson,
							(RowIndex++ % 2 == 0))
					];
				}
			}
			else
			{
				ContentBox->AddSlot().AutoHeight()
				[
					MakePropertyRow(
						FString::Printf(TEXT("%s[%d]"), *Ref.ParamType, Ref.RowId),
						TEXT("(row not found)"),
						(RowIndex++ % 2 == 0))
				];
			}
		}
	}
}

void SSekiroEventInspector::BuildRootMotionSection(const FSekiroTaeEvent& Evt, int32& RowIndex)
{
	USekiroSkillDataAsset* Skill = SkillData.Get();
	if (!Skill || Skill->RootMotion.Samples.Num() == 0)
	{
		return;
	}

	// Only show root-motion section for Movement category events or if frame range covers motion
	if (Evt.Category != TEXT("Movement"))
	{
		return;
	}

	ContentBox->AddSlot()
	.AutoHeight()
	.Padding(4.0f, 6.0f)
	[
		SNew(STextBlock)
		.Text(FText::FromString(TEXT("Root Motion (in event range)")))
		.Font(FCoreStyle::Get().GetFontStyle("NormalFont"))
		.ColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.85f, 0.4f)))
	];

	// Sample root-motion at start and end of event
	FSekiroRootMotionSample StartSample, EndSample;
	const bool bHasStart = Skill->GetRootMotionSampleAtFrame(Evt.StartFrame, StartSample);
	const bool bHasEnd = Skill->GetRootMotionSampleAtFrame(Evt.EndFrame, EndSample);

	if (bHasStart && bHasEnd)
	{
		const FVector Delta = EndSample.Translation - StartSample.Translation;
		const float YawDelta = EndSample.YawRadians - StartSample.YawRadians;

		ContentBox->AddSlot().AutoHeight()
		[
			MakePropertyRow(TEXT("Start Translation"), FString::Printf(TEXT("(%.2f, %.2f, %.2f)"), StartSample.Translation.X, StartSample.Translation.Y, StartSample.Translation.Z), (RowIndex++ % 2 == 0))
		];
		ContentBox->AddSlot().AutoHeight()
		[
			MakePropertyRow(TEXT("End Translation"), FString::Printf(TEXT("(%.2f, %.2f, %.2f)"), EndSample.Translation.X, EndSample.Translation.Y, EndSample.Translation.Z), (RowIndex++ % 2 == 0))
		];
		ContentBox->AddSlot().AutoHeight()
		[
			MakePropertyRow(TEXT("Delta Translation"), FString::Printf(TEXT("(%.2f, %.2f, %.2f)"), Delta.X, Delta.Y, Delta.Z), (RowIndex++ % 2 == 0))
		];
		ContentBox->AddSlot().AutoHeight()
		[
			MakePropertyRow(TEXT("Yaw Delta (rad)"), FString::Printf(TEXT("%.4f"), YawDelta), (RowIndex++ % 2 == 0))
		];
	}
	else
	{
		ContentBox->AddSlot().AutoHeight()
		[
			MakePropertyRow(TEXT("Root Motion"), TEXT("(samples not available for this frame range)"), (RowIndex++ % 2 == 0))
		];
	}
}

// ---------------------------------------------------------------------------
// Row helper
// ---------------------------------------------------------------------------

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
