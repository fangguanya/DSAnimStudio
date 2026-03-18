// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Animation/AnimSequence.h"
#include "Data/SekiroTaeEvent.h"
#include "SekiroSkillDataAsset.generated.h"

USTRUCT(BlueprintType)
struct SEKIROSKILLEDITORPLUGIN_API FSekiroRootMotionSample
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Root Motion")
	int32 FrameIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Root Motion")
	float TimeSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Root Motion")
	FVector Translation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Root Motion")
	float YawRadians = 0.0f;
};

USTRUCT(BlueprintType)
struct SEKIROSKILLEDITORPLUGIN_API FSekiroRootMotionTrack
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Root Motion")
	float FrameRate = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Root Motion")
	float DurationSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Root Motion")
	FVector TotalTranslation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Root Motion")
	float TotalYawRadians = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Root Motion")
	TArray<FSekiroRootMotionSample> Samples;
};

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

	/** Owning character asset used by the preview/editor session. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Data")
	TSoftObjectPtr<class USekiroCharacterData> CharacterData;

	/** Total number of frames in the animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Data")
	int32 FrameCount = 0;

	/** Playback frame rate. Defaults to 30 fps (FromSoftware standard). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Data")
	float FrameRate = 30.0f;

	/** All TAE events associated with this animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Data")
	TArray<FSekiroTaeEvent> Events;

	/** Canonical root-motion track exported alongside the animation clip. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Data")
	FSekiroRootMotionTrack RootMotion;

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

	UFUNCTION(BlueprintCallable, Category = "Skill Data")
	bool GetRootMotionSampleAtFrame(float Frame, FSekiroRootMotionSample& OutSample) const;
};
