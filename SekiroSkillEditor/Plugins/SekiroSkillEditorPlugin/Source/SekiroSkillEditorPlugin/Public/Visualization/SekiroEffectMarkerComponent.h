// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/SekiroTaeEvent.h"
#include "SekiroEffectMarkerComponent.generated.h"

/**
 * Draws debug markers for Effect and Sound TAE events.
 * Blue spheres indicate SFX/VFX spawn points; green diamonds indicate
 * sound trigger locations. Attach to ASekiroSkillPreviewActor.
 */
UCLASS(ClassGroup = (Sekiro), meta = (BlueprintSpawnableComponent))
class SEKIROSKILLEDITORPLUGIN_API USekiroEffectMarkerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USekiroEffectMarkerComponent();

	/** TAE events of Effect/Sound categories active at the current frame. */
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Effect Visualization")
	TArray<FSekiroTaeEvent> ActiveEvents;

	/** Whether effect/sound marker drawing is enabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect Visualization")
	bool bShowMarkers = true;

	/** Base size of the debug markers in Unreal units. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect Visualization", meta = (ClampMin = "1.0"))
	float MarkerSize = 30.0f;

	/**
	 * Filters AllEvents to those active at CurrentFrame whose Category
	 * is "Effect" or "Sound", storing the result in ActiveEvents.
	 */
	UFUNCTION(BlueprintCallable, Category = "Effect Visualization")
	void UpdateActiveEvents(float CurrentFrame, const TArray<FSekiroTaeEvent>& AllEvents);

	// -- UActorComponent overrides -------------------------------------------

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};
