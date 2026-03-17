// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Animation/AnimSequence.h"
#include "Data/SekiroTaeEvent.h"
#include "SekiroSkillDataAsset.generated.h"

/**
 * Data asset representing a single Sekiro skill / animation with its TAE events.
 */
UCLASS(BlueprintType)
class SEKIROSKILLEDITORPLUGIN_API USekiroSkillDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Identifier of the character this skill belongs to. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Data")
	FString CharacterId;

	/** Name of the source animation (e.g. "a000_003000"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Data")
	FString AnimationName;

	/** Canonical exported animation file name (e.g. "a000_003000.gltf"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Data")
	FString SourceFileName;

	/** Soft reference to the imported animation asset for preview playback. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Data")
	TSoftObjectPtr<UAnimSequence> Animation;

	/** Total number of frames in the animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Data")
	int32 FrameCount = 0;

	/** Playback frame rate. Defaults to 30 fps (FromSoftware standard). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Data")
	float FrameRate = 30.0f;

	/** All TAE events associated with this animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Data")
	TArray<FSekiroTaeEvent> Events;

	/**
	 * Returns all events that match the specified category.
	 * @param InCategory  Category string to filter by (case-sensitive).
	 * @return Array of matching events.
	 */
	UFUNCTION(BlueprintCallable, Category = "Skill Data")
	TArray<FSekiroTaeEvent> GetEventsByCategory(const FString& InCategory) const;

	/**
	 * Returns all events that are active at the given frame
	 * (StartFrame <= Frame <= EndFrame).
	 * @param Frame  The frame to query.
	 * @return Array of events active at that frame.
	 */
	UFUNCTION(BlueprintCallable, Category = "Skill Data")
	TArray<FSekiroTaeEvent> GetEventAtFrame(float Frame) const;
};
