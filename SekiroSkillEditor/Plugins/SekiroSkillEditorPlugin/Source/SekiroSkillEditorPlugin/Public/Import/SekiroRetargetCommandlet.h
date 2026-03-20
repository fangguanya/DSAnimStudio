#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "SekiroRetargetCommandlet.generated.h"

/**
 * Commandlet to create humanoid IK rig assets and retarget selected Sekiro animations onto a target mesh.
 * Usage: UnrealEditor-Cmd.exe project.uproject -run=SekiroRetarget
 */
UCLASS()
class USekiroRetargetCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	USekiroRetargetCommandlet();

	virtual int32 Main(const FString& Params) override;
};