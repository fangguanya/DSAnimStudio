// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Animation/AnimSequence.h"
#include "SekiroSkillPreviewActor.generated.h"

class USkeletalMeshComponent;
class USekiroCharacterData;

/**
 * Preview actor that displays a Sekiro character's skeletal mesh
 * and drives animation playback for the skill editor viewport.
 */
UCLASS(NotPlaceable)
class SEKIROSKILLEDITORPLUGIN_API ASekiroSkillPreviewActor : public AActor
{
	GENERATED_BODY()

public:
	ASekiroSkillPreviewActor();

	// -- Components ----------------------------------------------------------

	/** Primary skeletal mesh component showing the character model. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Preview")
	TObjectPtr<USkeletalMeshComponent> MeshComp;

	// -- Animation state -----------------------------------------------------

	/** The animation sequence currently assigned for preview playback. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Preview|Animation")
	TObjectPtr<UAnimSequence> CurrentAnim;

	/** Character data currently driving the preview mesh and battle metadata. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Preview")
	TObjectPtr<USekiroCharacterData> CurrentCharacterData;

	/** Playback speed multiplier. 1.0 = normal speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview|Animation", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float PlaybackRate = 1.0f;

	/** Whether the animation is currently advancing each tick. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview|Animation")
	bool bIsPlaying = false;

	/** Current playback position expressed in frames. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Preview|Animation")
	float CurrentFrame = 0.0f;

	// -- Public API ----------------------------------------------------------

	/**
	 * Configures the preview mesh and materials from a character data asset.
	 * Loads soft references synchronously so the mesh is available immediately.
	 */
	UFUNCTION(BlueprintCallable, Category = "Preview")
	void SetCharacterData(USekiroCharacterData* InCharacterData);

	/**
	 * Assigns a new animation sequence to the skeletal mesh component and
	 * resets the playback position to frame 0.
	 */
	UFUNCTION(BlueprintCallable, Category = "Preview")
	void PlayAnimation(UAnimSequence* InAnim);

	/**
	 * Manually sets the animation to a specific frame.
	 * Pauses playback automatically.
	 */
	UFUNCTION(BlueprintCallable, Category = "Preview")
	void SetPlaybackPosition(float Frame);

	/** Returns the current frame position of animation playback. */
	UFUNCTION(BlueprintPure, Category = "Preview")
	float GetCurrentFrame() const;

	// -- AActor overrides ----------------------------------------------------

	virtual void Tick(float DeltaSeconds) override;
};
