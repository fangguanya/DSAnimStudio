// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Data/SekiroTaeEvent.h"
#include "Data/SekiroSkillDataAsset.h"

/**
 * Delegate fired when a TAE event is selected on the timeline.
 */
DECLARE_DELEGATE_OneParam(FOnEventSelected, const FSekiroTaeEvent& /*SelectedEvent*/);

/**
 * Custom Slate widget that renders a multi-track timeline for Sekiro TAE events.
 *
 * Layout (top to bottom):
 *   - Frame number ruler          (RulerHeight px)
 *   - One track per event category (TrackHeight px each)
 *
 * Features:
 *   - Background grid lines every 10 frames
 *   - Colored event bars per category (via FSekiroTaeEvent::GetCategoryColor)
 *   - Red playhead line at CurrentFrame
 *   - Click/drag to scrub playhead, click event bar to select event
 */
class SEKIROSKILLEDITORPLUGIN_API SSekiroSkillTimeline : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSekiroSkillTimeline)
		: _SkillData(nullptr)
		, _CurrentFrame(0.0f)
		, _Zoom(1.0f)
		, _ScrollOffset(0.0f)
	{}
		/** The skill data asset to visualize. */
		SLATE_ARGUMENT(USekiroSkillDataAsset*, SkillData)

		/** Initial playhead position in frames. */
		SLATE_ARGUMENT(float, CurrentFrame)

		/** Horizontal zoom factor. 1.0 = default. */
		SLATE_ARGUMENT(float, Zoom)

		/** Horizontal scroll offset in pixels. */
		SLATE_ARGUMENT(float, ScrollOffset)

		/** Called when a TAE event bar is clicked. */
		SLATE_EVENT(FOnEventSelected, OnEventSelected)
	SLATE_END_ARGS()

	/** Construct this widget. */
	void Construct(const FArguments& InArgs);

	// ---- Accessors ----

	/** Replace the skill data asset displayed in the timeline. */
	void SetSkillData(USekiroSkillDataAsset* InSkillData);

	/** Move the playhead to the given frame. */
	void SetCurrentFrame(float InFrame);

	/** Get the current playhead position. */
	float GetCurrentFrame() const { return CurrentFrame; }

	/** Set horizontal zoom factor. */
	void SetZoom(float InZoom);

	/** Set horizontal scroll offset. */
	void SetScrollOffset(float InScrollOffset);

	// ---- SWidget overrides ----

	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;

private:
	// ---- Internal helpers ----

	/** Convert a frame number to a pixel X coordinate. */
	float FrameToPixel(float Frame) const;

	/** Convert a pixel X coordinate to a frame number. */
	float PixelToFrame(float PixelX) const;

	/** Rebuild the sorted list of unique category names from SkillData. */
	void RebuildCategoryList();

	/** Attempt to find which event bar lives under the given local position. */
	bool HitTestEventBar(const FVector2D& LocalPos, FSekiroTaeEvent& OutEvent) const;

	// ---- Layout constants ----

	static constexpr float RulerHeight   = 20.0f;
	static constexpr float HeaderHeight  = 30.0f;
	static constexpr float TrackHeight   = 24.0f;
	static constexpr float LabelWidth    = 100.0f;
	static constexpr float PixelsPerFrame = 8.0f;

	// ---- State ----

	TWeakObjectPtr<USekiroSkillDataAsset> SkillData;
	float CurrentFrame  = 0.0f;
	float Zoom          = 1.0f;
	float ScrollOffset  = 0.0f;

	/** Whether the user is currently scrubbing the playhead. */
	bool bIsScrubbing = false;

	/** Sorted unique category names derived from SkillData->Events. */
	TArray<FString> Categories;

	/** Delegate for event selection. */
	FOnEventSelected OnEventSelected;
};
