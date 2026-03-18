// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

#include "UI/SSekiroSkillTimeline.h"
#include "Widgets/SOverlay.h"
#include "Rendering/DrawElements.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

void SSekiroSkillTimeline::Construct(const FArguments& InArgs)
{
	SkillData     = InArgs._SkillData;
	CurrentFrame  = InArgs._CurrentFrame;
	Zoom          = FMath::Max(InArgs._Zoom, 0.01f);
	ScrollOffset  = InArgs._ScrollOffset;
	OnEventSelected = InArgs._OnEventSelected;
	OnFrameScrubbed = InArgs._OnFrameScrubbed;

	RebuildCategoryList();

	ChildSlot
	[
		SNew(SOverlay)
		// The timeline is entirely owner-drawn via OnPaint.
		// The SOverlay simply provides a hit-testable area.
	];
}

// ---------------------------------------------------------------------------
// Public accessors
// ---------------------------------------------------------------------------

void SSekiroSkillTimeline::SetSkillData(USekiroSkillDataAsset* InSkillData)
{
	SkillData = InSkillData;
	CurrentFrame = 0.0f;
	RebuildCategoryList();
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SSekiroSkillTimeline::SetCurrentFrame(float InFrame)
{
	CurrentFrame = FMath::Max(InFrame, 0.0f);
	Invalidate(EInvalidateWidgetReason::Paint);
	OnFrameScrubbed.ExecuteIfBound(CurrentFrame);
}

void SSekiroSkillTimeline::SetZoom(float InZoom)
{
	Zoom = FMath::Max(InZoom, 0.01f);
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SSekiroSkillTimeline::SetScrollOffset(float InScrollOffset)
{
	ScrollOffset = InScrollOffset;
	Invalidate(EInvalidateWidgetReason::Paint);
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

float SSekiroSkillTimeline::FrameToPixel(float Frame) const
{
	return LabelWidth + (Frame * PixelsPerFrame * Zoom) - ScrollOffset;
}

float SSekiroSkillTimeline::PixelToFrame(float PixelX) const
{
	return (PixelX - LabelWidth + ScrollOffset) / (PixelsPerFrame * Zoom);
}

void SSekiroSkillTimeline::RebuildCategoryList()
{
	Categories.Empty();

	if (!SkillData.IsValid())
	{
		return;
	}

	TSet<FString> UniqueCategories;
	for (const FSekiroTaeEvent& Evt : SkillData->Events)
	{
		UniqueCategories.Add(Evt.Category);
	}

	Categories = UniqueCategories.Array();
	Categories.Sort();
}

bool SSekiroSkillTimeline::HitTestEventBar(const FVector2D& LocalPos, FSekiroTaeEvent& OutEvent) const
{
	if (!SkillData.IsValid())
	{
		return false;
	}

	const float ContentTop = RulerHeight + HeaderHeight;

	for (int32 CatIdx = 0; CatIdx < Categories.Num(); ++CatIdx)
	{
		const float TrackTop = ContentTop + CatIdx * TrackHeight;
		const float TrackBottom = TrackTop + TrackHeight;

		if (LocalPos.Y < TrackTop || LocalPos.Y > TrackBottom)
		{
			continue;
		}

		for (const FSekiroTaeEvent& Evt : SkillData->Events)
		{
			if (Evt.Category != Categories[CatIdx])
			{
				continue;
			}

			const float BarLeft  = FrameToPixel(Evt.StartFrame);
			const float BarRight = FrameToPixel(Evt.EndFrame);

			if (LocalPos.X >= BarLeft && LocalPos.X <= BarRight)
			{
				OutEvent = Evt;
				return true;
			}
		}
	}

	return false;
}

// ---------------------------------------------------------------------------
// OnPaint
// ---------------------------------------------------------------------------

int32 SSekiroSkillTimeline::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	const bool bEnabled = ShouldBeEnabled(bParentEnabled);
	const ESlateDrawEffect DrawEffects = bEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("WhiteBrush");
	const FSlateFontInfo SmallFont = FCoreStyle::Get().GetFontStyle("SmallFont");

	// ------------------------------------------------------------------
	// 1. Background
	// ------------------------------------------------------------------
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(),
		WhiteBrush,
		DrawEffects,
		FLinearColor(0.12f, 0.12f, 0.12f, 1.0f));
	++LayerId;

	if (!SkillData.IsValid())
	{
		return LayerId;
	}

	const int32 TotalFrames = SkillData->FrameCount;
	const float ContentTop = RulerHeight + HeaderHeight;

	// ------------------------------------------------------------------
	// 2. Vertical grid lines every 10 frames
	// ------------------------------------------------------------------
	for (int32 Frame = 0; Frame <= TotalFrames; Frame += 10)
	{
		const float X = FrameToPixel(static_cast<float>(Frame));
		if (X < LabelWidth || X > LocalSize.X)
		{
			continue;
		}

		TArray<FVector2D> GridLine;
		GridLine.Add(FVector2D(X, 0.0f));
		GridLine.Add(FVector2D(X, LocalSize.Y));

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			GridLine,
			DrawEffects,
			FLinearColor(0.25f, 0.25f, 0.25f, 0.6f),
			false,
			1.0f);
	}
	++LayerId;

	// ------------------------------------------------------------------
	// 3. Frame number ruler at top
	// ------------------------------------------------------------------
	{
		// Ruler background
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(FVector2D(LocalSize.X, RulerHeight), FSlateLayoutTransform(FVector2D::ZeroVector)),
			WhiteBrush,
			DrawEffects,
			FLinearColor(0.08f, 0.08f, 0.08f, 1.0f));

		// Frame numbers every 10 frames
		for (int32 Frame = 0; Frame <= TotalFrames; Frame += 10)
		{
			const float X = FrameToPixel(static_cast<float>(Frame));
			if (X < LabelWidth || X > LocalSize.X)
			{
				continue;
			}

			const FString FrameStr = FString::FromInt(Frame);
			FSlateDrawElement::MakeText(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(FVector2D(40.0f, RulerHeight), FSlateLayoutTransform(FVector2D(X, 0.0f))),
				FrameStr,
				SmallFont,
				DrawEffects,
				FLinearColor(0.7f, 0.7f, 0.7f, 1.0f));
		}
	}
	++LayerId;

	// ------------------------------------------------------------------
	// 4. Header bar (between ruler and tracks)
	// ------------------------------------------------------------------
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(
			FVector2D(LocalSize.X, HeaderHeight),
			FSlateLayoutTransform(FVector2D(0.0f, RulerHeight))),
		WhiteBrush,
		DrawEffects,
		FLinearColor(0.15f, 0.15f, 0.18f, 1.0f));
	++LayerId;

	// ------------------------------------------------------------------
	// 5. Category tracks: labels + event bars
	// ------------------------------------------------------------------
	for (int32 CatIdx = 0; CatIdx < Categories.Num(); ++CatIdx)
	{
		const FString& CatName = Categories[CatIdx];
		const float TrackTop = ContentTop + CatIdx * TrackHeight;
		const FLinearColor CatColor = FSekiroTaeEvent::GetCategoryColor(CatName);

		// Alternating track background
		const float BgAlpha = (CatIdx % 2 == 0) ? 0.05f : 0.10f;
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(
				FVector2D(LocalSize.X, TrackHeight),
				FSlateLayoutTransform(FVector2D(0.0f, TrackTop))),
			WhiteBrush,
			DrawEffects,
			FLinearColor(1.0f, 1.0f, 1.0f, BgAlpha));

		// Category label on the left
		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(
				FVector2D(LabelWidth, TrackHeight),
				FSlateLayoutTransform(FVector2D(4.0f, TrackTop + 2.0f))),
			CatName,
			SmallFont,
			DrawEffects,
			FLinearColor(0.85f, 0.85f, 0.85f, 1.0f));

		// Event bars
		for (const FSekiroTaeEvent& Evt : SkillData->Events)
		{
			if (Evt.Category != CatName)
			{
				continue;
			}

			const float BarLeft  = FMath::Max(FrameToPixel(Evt.StartFrame), LabelWidth);
			const float BarRight = FMath::Min(FrameToPixel(Evt.EndFrame), LocalSize.X);

			if (BarRight <= BarLeft)
			{
				continue;
			}

			const float BarWidth = BarRight - BarLeft;
			const float BarTop   = TrackTop + 2.0f;
			const float BarH     = TrackHeight - 4.0f;

			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId + 1,
				AllottedGeometry.ToPaintGeometry(
					FVector2D(BarWidth, BarH),
					FSlateLayoutTransform(FVector2D(BarLeft, BarTop))),
				WhiteBrush,
				DrawEffects,
				CatColor);

			// Event type label inside bar (if bar is wide enough)
			if (BarWidth > 30.0f)
			{
				FSlateDrawElement::MakeText(
					OutDrawElements,
					LayerId + 2,
					AllottedGeometry.ToPaintGeometry(
						FVector2D(BarWidth - 4.0f, BarH),
						FSlateLayoutTransform(FVector2D(BarLeft + 2.0f, BarTop + 1.0f))),
					Evt.TypeName,
					SmallFont,
					DrawEffects,
					FLinearColor::Black);
			}
		}
	}
	LayerId += 3;

	// ------------------------------------------------------------------
	// 6. Playhead (red vertical line)
	// ------------------------------------------------------------------
	{
		const float PlayheadX = FrameToPixel(CurrentFrame);

		if (PlayheadX >= LabelWidth && PlayheadX <= LocalSize.X)
		{
			TArray<FVector2D> PlayheadLine;
			PlayheadLine.Add(FVector2D(PlayheadX, 0.0f));
			PlayheadLine.Add(FVector2D(PlayheadX, LocalSize.Y));

			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(),
				PlayheadLine,
				DrawEffects,
				FLinearColor::Red,
				false,
				2.0f);

			// Small triangle indicator at the top
			TArray<FSlateVertex> TriVerts;
			TArray<SlateIndex> TriIndices;

			const float TriSize = 6.0f;
			const FVector2f P0(PlayheadX, 0.0f);
			const FVector2f P1(PlayheadX - TriSize, -TriSize);
			const FVector2f P2(PlayheadX + TriSize, -TriSize);

			// We draw a simple filled box instead for simplicity
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(
					FVector2D(TriSize * 2.0f, TriSize),
					FSlateLayoutTransform(FVector2D(PlayheadX - TriSize, 0.0f))),
				WhiteBrush,
				DrawEffects,
				FLinearColor::Red);
		}
	}
	++LayerId;

	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
}

// ---------------------------------------------------------------------------
// Mouse interaction
// ---------------------------------------------------------------------------

FReply SSekiroSkillTimeline::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

		// Check if we hit an event bar first
		FSekiroTaeEvent HitEvent;
		if (HitTestEventBar(LocalPos, HitEvent))
		{
			OnEventSelected.ExecuteIfBound(HitEvent);
			return FReply::Handled();
		}

		// Otherwise, start scrubbing the playhead
		if (LocalPos.X >= LabelWidth)
		{
			bIsScrubbing = true;
			const float NewFrame = PixelToFrame(LocalPos.X);
			CurrentFrame = FMath::Max(NewFrame, 0.0f);
			Invalidate(EInvalidateWidgetReason::Paint);
			OnFrameScrubbed.ExecuteIfBound(CurrentFrame);
			return FReply::Handled().CaptureMouse(SharedThis(const_cast<SSekiroSkillTimeline*>(this)));
		}
	}

	return FReply::Unhandled();
}

FReply SSekiroSkillTimeline::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsScrubbing)
	{
		bIsScrubbing = false;
		return FReply::Handled().ReleaseMouseCapture();
	}

	return FReply::Unhandled();
}

FReply SSekiroSkillTimeline::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (bIsScrubbing)
	{
		const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		const float NewFrame = PixelToFrame(LocalPos.X);

		float MaxFrame = 0.0f;
		if (SkillData.IsValid())
		{
			MaxFrame = static_cast<float>(SkillData->FrameCount);
		}

		CurrentFrame = FMath::Clamp(NewFrame, 0.0f, MaxFrame);
		Invalidate(EInvalidateWidgetReason::Paint);
		OnFrameScrubbed.ExecuteIfBound(CurrentFrame);
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

FVector2D SSekiroSkillTimeline::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	float DesiredWidth = LabelWidth + 400.0f; // minimum
	float DesiredHeight = RulerHeight + HeaderHeight;

	if (SkillData.IsValid())
	{
		DesiredWidth = LabelWidth + (SkillData->FrameCount * PixelsPerFrame * Zoom);
		DesiredHeight += Categories.Num() * TrackHeight;
	}
	else
	{
		DesiredHeight += TrackHeight; // at least one track of space
	}

	return FVector2D(DesiredWidth, DesiredHeight);
}
