#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "SekiroImportCommandlet.generated.h"

/**
 * Commandlet to batch import Sekiro exported assets (textures, models, animations).
 * Supports FBX, glTF (via Interchange), and Collada formats.
 * Usage: UnrealEditor-Cmd.exe project.uproject -run=SekiroImport -ExportDir="E:/Sekiro/Export" [-ChrFilter=c1020,c1000]
 */
UCLASS()
class USekiroImportCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	USekiroImportCommandlet();

	virtual int32 Main(const FString& Params) override;

private:
	// Import via UE5 Interchange (for glTF/glb models - handles skeleton/skin properly)
	UObject* ImportViaAssetTask(const FString& FilePath, const FString& DestPackagePath);

	// Import via FBX factory (for FBX/DAE formats)
	UObject* ImportMeshViaFbxFactory(const FString& FilePath, const FString& DestPackagePath);
	UObject* ImportAnimationViaFbxFactory(const FString& FilePath, const FString& DestPackagePath, class USkeleton* Skeleton);

	// Import animation by parsing glTF and creating AnimSequence programmatically
	UObject* ImportAnimationFromGltf(const FString& FilePath, const FString& DestPackagePath, class USkeleton* Skeleton);

	UObject* ImportTexture(const FString& FilePath, const FString& DestPackagePath);

	void ImportCharacter(const FString& ChrId, const FString& ExportDir, const FString& ContentBase);

	FString FindBestModelFile(const FString& ModelDir, const FString& ChrId);
	class USkeleton* FindSkeletonInPackage(const FString& PackagePath);
	bool SavePackage(UObject* Asset);
};
