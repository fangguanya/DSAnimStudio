// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Data/SekiroTaeEvent.h"

/**
 * Inspector panel that displays the details of a selected FSekiroTaeEvent.
 *
 * Shows: Type, TypeName, Category, Frame Range, and all Parameters
 * in a scrollable vertical list.
 */
class SEKIROSKILLEDITORPLUGIN_API SSekiroEventInspector : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSekiroEventInspector) {}
	SLATE_END_ARGS()

	/** Construct this widget. */
	void Construct(const FArguments& InArgs);

	/** Set the event to inspect and rebuild the UI. */
	void SetEvent(const FSekiroTaeEvent& InEvent);

	/** Clear the inspector. */
	void ClearEvent();

private:
	/** Rebuild all child widgets from the current event data. */
	void RebuildContent();

	/** Helper: create a label + value row. */
	TSharedRef<SWidget> MakePropertyRow(const FString& Label, const FString& Value, bool bAlternateColor) const;

	/** The currently inspected event (may be empty). */
	TOptional<FSekiroTaeEvent> CurrentEvent;

	/** Container that holds all content rows. */
	TSharedPtr<SVerticalBox> ContentBox;

	/** The scroll box wrapping ContentBox. */
	TSharedPtr<SScrollBox> ScrollBox;

	/** Whether an event is loaded. */
	bool bHasEvent = false;
};
