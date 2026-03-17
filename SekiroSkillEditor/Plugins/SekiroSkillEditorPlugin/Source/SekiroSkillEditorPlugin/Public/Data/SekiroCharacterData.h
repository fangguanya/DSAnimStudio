// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/Skeleton.h"
#include "Materials/MaterialInterface.h"
#include "Data/SekiroSkillDataAsset.h"
#include "SekiroCharacterData.generated.h"

USTRUCT(BlueprintType)
struct SEKIROSKILLEDITORPLUGIN_API FSekiroDummyPoly
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Data")
	int32 ReferenceId = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Data")
	FString ParentBoneName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Data")
	FString AttachBoneName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Data")
	FVector LocalPosition = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Data")
	FVector Forward = FVector::ForwardVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Data")
	FVector Upward = FVector::UpVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Data")
	bool bUseUpwardVector = false;
};

USTRUCT(BlueprintType)
struct SEKIROSKILLEDITORPLUGIN_API FSekiroParamField
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Data")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Data")
	FString ValueJson;
};

USTRUCT(BlueprintType)
struct SEKIROSKILLEDITORPLUGIN_API FSekiroParamRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Data")
	int32 RowId = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Data")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Data")
	TArray<FSekiroParamField> Fields;
};

/**
 * Data asset holding character-level data: mesh, skeleton, materials,
 * and references to all associated skill data assets.
 */
UCLASS(BlueprintType)
class SEKIROSKILLEDITORPLUGIN_API USekiroCharacterData : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Unique identifier for this character (e.g. "c0000" for the player). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Data")
	FString CharacterId;

	/** Soft reference to the character's skeletal mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Data")
	TSoftObjectPtr<USkeletalMesh> Mesh;

	/** Soft reference to the character's skeleton asset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Data")
	TSoftObjectPtr<USkeleton> Skeleton;

	/** Named material slots mapped to soft material references. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Data")
	TMap<FString, TSoftObjectPtr<UMaterialInterface>> MaterialMap;

	/** Soft references to all skill data assets for this character. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Data")
	TArray<TSoftObjectPtr<USekiroSkillDataAsset>> Skills;

	/** Dummy poly definitions exported from FLVER for viewport reconstruction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Data")
	TArray<FSekiroDummyPoly> DummyPolys;

	/** Structured parameter caches used by the importer, validator, and viewport. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Data")
	TMap<int32, FSekiroParamRow> BehaviorParamsNpc;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Data")
	TMap<int32, FSekiroParamRow> BehaviorParamsPlayer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Data")
	TMap<int32, FSekiroParamRow> AtkParamsNpc;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Data")
	TMap<int32, FSekiroParamRow> AtkParamsPlayer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Data")
	TMap<int32, FSekiroParamRow> SpEffectParams;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Data")
	TMap<int32, FSekiroParamRow> EquipParamWeapon;

	const FSekiroDummyPoly* FindDummyPoly(int32 InReferenceId) const;
};
