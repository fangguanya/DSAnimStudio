#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "SekiroImportCommandlet.generated.h"

/**
 * Commandlet to batch import Sekiro exported assets (textures, models, animations).
 * Only glTF 2.0 (.gltf/.glb) and PNG are accepted as formal inputs.
 * Usage: UnrealEditor-Cmd.exe project.uproject -run=SekiroImport -ExportDir="E:/Sekiro/Export" [-ChrFilter=c1020,c1000] [-Canonical17Only]
 */
UCLASS()
class USekiroImportCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	USekiroImportCommandlet();

	virtual int32 Main(const FString& Params) override;

private:
	// Import via UE5 Interchange (glTF/glb - models AND animations)
	// Pass a valid Skeleton to import animations only (uses existing skeleton)
	UObject* ImportViaAssetTask(const FString& FilePath, const FString& DestPackagePath, class USkeleton* SkeletonOverride = nullptr, bool bSaveAssetsInDestinationPath = false, bool bUseHumanoidPipeline = false, TArray<FString>* OutPipelineErrors = nullptr);

	UObject* ImportTexture(const FString& FilePath, const FString& DestPackagePath);

	void ImportCharacter(const FString& ChrId, const FString& ExportDir, const FString& ContentBase, int32 AnimLimit = -1, const TArray<FString>& AnimFilters = TArray<FString>(), bool bImportAnimationsOnly = false, bool bImportModelOnly = false);

	class USkeleton* FindSkeletonInPackage(const FString& PackagePath);
	class USkeletalMesh* FindSkeletalMeshInPackage(const FString& PackagePath);
	bool SavePackage(UObject* Asset);
};
