// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SekiroTaeEvent.generated.h"

/**
 * Represents a single TAE (TimeAct Event) from a Sekiro animation file.
 * Each event has a type, category, time range, and parameter map.
 */
USTRUCT(BlueprintType)
struct SEKIROSKILLEDITORPLUGIN_API FSekiroEventParam
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TAE Event")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TAE Event")
	FString DataType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TAE Event")
	int32 ByteOffset = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TAE Event")
	FString Source;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TAE Event")
	FString ValueJson;
};

USTRUCT(BlueprintType)
struct SEKIROSKILLEDITORPLUGIN_API FSekiroSemanticLink
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TAE Event")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TAE Event")
	FString ValueJson;
};

USTRUCT(BlueprintType)
struct SEKIROSKILLEDITORPLUGIN_API FSekiroTaeEvent
{
	GENERATED_BODY()

	/** Numeric event type ID from the TAE file. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TAE Event")
	int32 Type = 0;

	/** Human-readable name for this event type. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TAE Event")
	FString TypeName;

	/** Category grouping (e.g. Attack, Effect, Sound, Movement). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TAE Event")
	FString Category;

	/** Frame at which the event begins. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TAE Event")
	float StartFrame = 0.0f;

	/** Frame at which the event ends. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TAE Event")
	float EndFrame = 0.0f;

	/** Arbitrary key-value parameter map parsed from the TAE event data. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TAE Event")
	TMap<FString, FString> Parameters;

	/** Structured event parameters preserving type and byte-offset metadata. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TAE Event")
	TArray<FSekiroEventParam> Params;

	/** Canonical semantic links emitted by the exporter for UI and validation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TAE Event")
	TArray<FSekiroSemanticLink> SemanticLinks;

	/**
	 * Returns a display color associated with the given event category.
	 * Used for color-coding events in the timeline editor UI.
	 */
	static FLinearColor GetCategoryColor(const FString& InCategory);

	const FSekiroEventParam* FindParam(const FString& InName) const;
};
