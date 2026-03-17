// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "SekiroAssetFactory.generated.h"

class USekiroSkillDataAsset;

/**
 * UFactory implementation that allows .json skill config files to be
 * imported through the Content Browser or drag-and-drop into the editor.
 */
UCLASS()
class SEKIROSKILLEDITORPLUGIN_API USekiroSkillConfigFactory : public UFactory
{
	GENERATED_BODY()

public:
	USekiroSkillConfigFactory();

	//~ Begin UFactory Interface
	virtual UObject* FactoryCreateFile(
		UClass* InClass,
		UObject* InParent,
		FName InName,
		EObjectFlags Flags,
		const FString& Filename,
		const TCHAR* Parms,
		FFeedbackContext* Warn,
		bool& bOutOperationCanceled
	) override;
	//~ End UFactory Interface
};
