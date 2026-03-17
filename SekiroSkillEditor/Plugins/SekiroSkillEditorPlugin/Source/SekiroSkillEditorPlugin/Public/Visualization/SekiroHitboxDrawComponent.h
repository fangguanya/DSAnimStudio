// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/SekiroTaeEvent.h"
#include "SekiroHitboxDrawComponent.generated.h"

/**
 * Draws debug visualizations for attack hitbox TAE events.
 * Attach to ASekiroSkillPreviewActor to overlay red spheres/capsules
 * for every active attack event at the current frame.
 */
UCLASS(ClassGroup = (Sekiro), meta = (BlueprintSpawnableComponent))
class SEKIROSKILLEDITORPLUGIN_API USekiroHitboxDrawComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USekiroHitboxDrawComponent();

	/** The set of TAE events currently active at the playback position. */
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Hitbox Visualization")
	TArray<FSekiroTaeEvent> ActiveEvents;

	/** Whether hitbox debug drawing is enabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hitbox Visualization")
	bool bShowHitboxes = true;

	/** Radius used for debug sphere/capsule drawing (in Unreal units). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hitbox Visualization", meta = (ClampMin = "1.0"))
	float HitboxRadius = 50.0f;

	/**
	 * Filters AllEvents to those whose StartFrame <= CurrentFrame <= EndFrame
	 * and whose Category equals "Attack", storing the result in ActiveEvents.
	 */
	UFUNCTION(BlueprintCallable, Category = "Hitbox Visualization")
	void UpdateActiveEvents(float CurrentFrame, const TArray<FSekiroTaeEvent>& AllEvents);

	// -- UActorComponent overrides -------------------------------------------

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};
