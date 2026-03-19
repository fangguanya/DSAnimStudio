#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "SekiroValidationCommandlet.generated.h"

/**
 * Commandlet to validate all imported Sekiro assets.
 * Checks SkeletalMesh, Skeleton, AnimSequence, Textures, Materials, and Skill Configs.
 * Usage: UnrealEditor-Cmd.exe project.uproject -run=SekiroValidation -ExportDir="E:/Sekiro/Export" [-ChrFilter=c1010]
 */
UCLASS()
class USekiroValidationCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	USekiroValidationCommandlet();
	virtual int32 Main(const FString& Params) override;

private:
	struct FCharacterValidation
	{
		FString ChrId;
		// Skeleton/Mesh
		bool bHasSkeleton = false;
		bool bHasSkeletalMesh = false;
		bool bHasStaticMesh = false; // Some characters import as StaticMesh (no bone weights)
		bool bHasPhysicsAsset = false;
		int32 BoneCount = 0;
		int32 MeshSectionCount = 0;
		int32 VertexCount = 0;
		// Animations
		int32 AnimSequenceCount = 0;
		int32 AnimsWithZeroDuration = 0;
		int32 AnimsWithZeroTracks = 0;
		int32 AnimsWithWrongSkeleton = 0;
		float TotalAnimDuration = 0.0f;
		// Textures
		int32 TextureCount = 0;
		int32 TexturesWithZeroSize = 0;
		// Materials
		int32 MaterialCount = 0;
		int32 ExpectedMaterialTextureBindings = 0;
		int32 ResolvedMaterialTextureBindings = 0;
		int32 MaterialBindingErrors = 0;
		// Skills
		bool bHasSkillConfig = false;
		bool bHasCanonicalSkillConfig = false;
		int32 SkillAnimCount = 0;
		int32 SkillEventCount = 0;
		int32 SkillEventCategories = 0;
		int32 SkillSchemaErrors = 0;
		// Skill-Animation cross-check
		int32 SkillAnimsWithMatchingAsset = 0;
		int32 SkillAnimsWithoutMatchingAsset = 0;
		int32 SkillAnimsSharedPool = 0; // a000_* shared animations (expected unmatched)

		int32 GetErrorCount() const;
		int32 GetWarningCount() const;
	};

	FCharacterValidation ValidateCharacter(const FString& ChrId, const FString& ContentBase, const FString& ExportDir);
	void ValidateSkillConfig(const FString& ChrId, const FString& SkillJsonPath, const FString& ContentBase, FCharacterValidation& Result);
};
