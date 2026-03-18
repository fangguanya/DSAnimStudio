// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

#include "Import/SekiroAssetImporter.h"

#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Data/SekiroCharacterData.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "AssetRegistry/AssetRegistryModule.h"

namespace
{
	static FString SerializeJsonValue(const TSharedPtr<FJsonValue>& JsonValue)
	{
		FString Serialized;
		const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
			TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Serialized);
		FJsonSerializer::Serialize(JsonValue, TEXT("Value"), Writer);
		Writer->Close();
		return Serialized;
	}

	static FString GetParentPackagePath(const FString& InPackagePath)
	{
		int32 LastSlashIndex = INDEX_NONE;
		if (!InPackagePath.FindLastChar(TEXT('/'), LastSlashIndex))
		{
			return InPackagePath;
		}

		return InPackagePath.Left(LastSlashIndex);
	}

	static TSoftObjectPtr<UAnimSequence> FindAnimationAsset(const FString& AnimationsPath, const FString& AnimationFileName)
	{
		const FString TargetAssetName = FPaths::GetBaseFilename(AnimationFileName);
		if (TargetAssetName.IsEmpty())
		{
			return nullptr;
		}

		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
		TArray<FAssetData> Assets;
		AssetRegistry.GetAssetsByPath(FName(*AnimationsPath), Assets, true);

		for (const FAssetData& AssetData : Assets)
		{
			if (AssetData.AssetClassPath.GetAssetName() == FName(TEXT("AnimSequence")) && AssetData.AssetName.ToString() == TargetAssetName)
			{
				return TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(AssetData.ToSoftObjectPath()));
			}
		}

		return nullptr;
	}

	static TSoftObjectPtr<USkeletalMesh> FindSkeletalMeshAsset(const FString& MeshPath)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
		TArray<FAssetData> Assets;
		AssetRegistry.GetAssetsByPath(FName(*MeshPath), Assets, true);
		for (const FAssetData& AssetData : Assets)
		{
			if (AssetData.AssetClassPath.GetAssetName() == FName(TEXT("SkeletalMesh")))
			{
				return TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(AssetData.ToSoftObjectPath()));
			}
		}

		return nullptr;
	}

	static TSoftObjectPtr<USkeleton> FindSkeletonAsset(const FString& MeshPath)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
		TArray<FAssetData> Assets;
		AssetRegistry.GetAssetsByPath(FName(*MeshPath), Assets, true);
		for (const FAssetData& AssetData : Assets)
		{
			if (AssetData.AssetClassPath.GetAssetName() == FName(TEXT("Skeleton")))
			{
				return TSoftObjectPtr<USkeleton>(FSoftObjectPath(AssetData.ToSoftObjectPath()));
			}
		}

		return nullptr;
	}

	static TMap<int32, FSekiroParamRow> ParseParamRows(const TSharedPtr<FJsonObject>& TableObject)
	{
		TMap<int32, FSekiroParamRow> Result;
		if (!TableObject.IsValid())
		{
			return Result;
		}

		const TSharedPtr<FJsonObject>* RowsObject = nullptr;
		if (!TableObject->TryGetObjectField(TEXT("rows"), RowsObject) || !RowsObject || !RowsObject->IsValid())
		{
			return Result;
		}

		for (const auto& Pair : (*RowsObject)->Values)
		{
			const TSharedPtr<FJsonObject> RowObject = Pair.Value->AsObject();
			if (!RowObject.IsValid())
			{
				continue;
			}

			FSekiroParamRow Row;
			Row.RowId = FCString::Atoi(*Pair.Key);
			Row.Name = RowObject->GetStringField(TEXT("Name"));

			for (const auto& FieldPair : RowObject->Values)
			{
				if (FieldPair.Key == TEXT("ID") || FieldPair.Key == TEXT("Name"))
				{
					continue;
				}

				FSekiroParamField Field;
				Field.Name = FieldPair.Key;
				Field.ValueJson = SerializeJsonValue(FieldPair.Value);
				Row.Fields.Add(MoveTemp(Field));
			}

			Result.Add(Row.RowId, MoveTemp(Row));
		}

		return Result;
	}

	static FSekiroRootMotionTrack ParseRootMotionTrack(const TSharedPtr<FJsonObject>& AnimObject)
	{
		FSekiroRootMotionTrack Result;
		if (!AnimObject.IsValid())
		{
			return Result;
		}

		const TSharedPtr<FJsonObject>* RootMotionObject = nullptr;
		if (!AnimObject->TryGetObjectField(TEXT("rootMotion"), RootMotionObject) || !RootMotionObject || !RootMotionObject->IsValid())
		{
			return Result;
		}

		Result.FrameRate = static_cast<float>((*RootMotionObject)->GetNumberField(TEXT("frameRate")));
		Result.DurationSeconds = static_cast<float>((*RootMotionObject)->GetNumberField(TEXT("durationSeconds")));
		Result.TotalYawRadians = static_cast<float>((*RootMotionObject)->GetNumberField(TEXT("totalYawRadians")));

		const TSharedPtr<FJsonObject>* TotalTranslationObject = nullptr;
		if ((*RootMotionObject)->TryGetObjectField(TEXT("totalTranslation"), TotalTranslationObject) && TotalTranslationObject && TotalTranslationObject->IsValid())
		{
			Result.TotalTranslation = FVector(
				(*TotalTranslationObject)->GetNumberField(TEXT("x")),
				(*TotalTranslationObject)->GetNumberField(TEXT("y")),
				(*TotalTranslationObject)->GetNumberField(TEXT("z")));
		}

		const TArray<TSharedPtr<FJsonValue>>* SamplesArray = nullptr;
		if ((*RootMotionObject)->TryGetArrayField(TEXT("samples"), SamplesArray))
		{
			for (const TSharedPtr<FJsonValue>& SampleValue : *SamplesArray)
			{
				const TSharedPtr<FJsonObject> SampleObject = SampleValue->AsObject();
				if (!SampleObject.IsValid())
				{
					continue;
				}

				FSekiroRootMotionSample Sample;
				Sample.FrameIndex = SampleObject->GetIntegerField(TEXT("frameIndex"));
				Sample.TimeSeconds = static_cast<float>(SampleObject->GetNumberField(TEXT("timeSeconds")));
				Sample.YawRadians = static_cast<float>(SampleObject->GetNumberField(TEXT("yawRadians")));

				const TSharedPtr<FJsonObject>* TranslationObject = nullptr;
				if (SampleObject->TryGetObjectField(TEXT("translation"), TranslationObject) && TranslationObject && TranslationObject->IsValid())
				{
					Sample.Translation = FVector(
						(*TranslationObject)->GetNumberField(TEXT("x")),
						(*TranslationObject)->GetNumberField(TEXT("y")),
						(*TranslationObject)->GetNumberField(TEXT("z")));
				}

				Result.Samples.Add(MoveTemp(Sample));
			}
		}

		return Result;
	}

	static void PopulateCharacterDataFromJson(
		USekiroCharacterData* CharacterAsset,
		const TSharedPtr<FJsonObject>& CharacterObj,
		const TSharedPtr<FJsonObject>& ParamsObj,
		const FString& CharacterContentPath)
	{
		if (!CharacterAsset || !CharacterObj.IsValid())
		{
			return;
		}

		CharacterAsset->Mesh = FindSkeletalMeshAsset(CharacterContentPath / TEXT("Mesh"));
		CharacterAsset->Skeleton = FindSkeletonAsset(CharacterContentPath / TEXT("Mesh"));

		CharacterAsset->DummyPolys.Reset();
		const TArray<TSharedPtr<FJsonValue>>* DummyPolysArray = nullptr;
		if (CharacterObj->TryGetArrayField(TEXT("dummyPolys"), DummyPolysArray))
		{
			for (const TSharedPtr<FJsonValue>& DummyValue : *DummyPolysArray)
			{
				const TSharedPtr<FJsonObject> DummyObj = DummyValue->AsObject();
				if (!DummyObj.IsValid())
				{
					continue;
				}

				const TSharedPtr<FJsonObject>* PositionObj = nullptr;
				const TSharedPtr<FJsonObject>* ForwardObj = nullptr;
				const TSharedPtr<FJsonObject>* UpwardObj = nullptr;

				FSekiroDummyPoly DummyPoly;
				DummyPoly.ReferenceId = DummyObj->GetIntegerField(TEXT("referenceId"));
				DummyPoly.ParentBoneName = DummyObj->GetStringField(TEXT("parentBoneName"));
				DummyPoly.AttachBoneName = DummyObj->GetStringField(TEXT("attachBoneName"));
				DummyPoly.bUseUpwardVector = DummyObj->GetBoolField(TEXT("useUpwardVector"));

				if (DummyObj->TryGetObjectField(TEXT("position"), PositionObj) && PositionObj)
				{
					DummyPoly.LocalPosition = FVector(
						(*PositionObj)->GetNumberField(TEXT("x")),
						(*PositionObj)->GetNumberField(TEXT("y")),
						(*PositionObj)->GetNumberField(TEXT("z")));
				}

				if (DummyObj->TryGetObjectField(TEXT("forward"), ForwardObj) && ForwardObj)
				{
					DummyPoly.Forward = FVector(
						(*ForwardObj)->GetNumberField(TEXT("x")),
						(*ForwardObj)->GetNumberField(TEXT("y")),
						(*ForwardObj)->GetNumberField(TEXT("z")));
				}

				if (DummyObj->TryGetObjectField(TEXT("upward"), UpwardObj) && UpwardObj)
				{
					DummyPoly.Upward = FVector(
						(*UpwardObj)->GetNumberField(TEXT("x")),
						(*UpwardObj)->GetNumberField(TEXT("y")),
						(*UpwardObj)->GetNumberField(TEXT("z")));
				}

				CharacterAsset->DummyPolys.Add(MoveTemp(DummyPoly));
			}
		}

		if (ParamsObj.IsValid())
		{
			const TSharedPtr<FJsonObject>* AtkObject = nullptr;
			if (ParamsObj->TryGetObjectField(TEXT("AtkParam"), AtkObject) && AtkObject)
			{
				const TSharedPtr<FJsonObject>* PlayerObject = nullptr;
				const TSharedPtr<FJsonObject>* NpcObject = nullptr;
				if ((*AtkObject)->TryGetObjectField(TEXT("player"), PlayerObject) && PlayerObject)
				{
					CharacterAsset->AtkParamsPlayer = ParseParamRows(*PlayerObject);
				}
				if ((*AtkObject)->TryGetObjectField(TEXT("npc"), NpcObject) && NpcObject)
				{
					CharacterAsset->AtkParamsNpc = ParseParamRows(*NpcObject);
				}
			}

			const TSharedPtr<FJsonObject>* BehaviorObject = nullptr;
			if (ParamsObj->TryGetObjectField(TEXT("BehaviorParam"), BehaviorObject) && BehaviorObject)
			{
				const TSharedPtr<FJsonObject>* PlayerObject = nullptr;
				const TSharedPtr<FJsonObject>* NpcObject = nullptr;
				if ((*BehaviorObject)->TryGetObjectField(TEXT("player"), PlayerObject) && PlayerObject)
				{
					CharacterAsset->BehaviorParamsPlayer = ParseParamRows(*PlayerObject);
				}
				if ((*BehaviorObject)->TryGetObjectField(TEXT("npc"), NpcObject) && NpcObject)
				{
					CharacterAsset->BehaviorParamsNpc = ParseParamRows(*NpcObject);
				}
			}

			const TSharedPtr<FJsonObject>* SpEffectObject = nullptr;
			if (ParamsObj->TryGetObjectField(TEXT("SpEffectParam"), SpEffectObject) && SpEffectObject)
			{
				CharacterAsset->SpEffectParams = ParseParamRows(*SpEffectObject);
			}

			const TSharedPtr<FJsonObject>* EquipParamWeaponObject = nullptr;
			if (ParamsObj->TryGetObjectField(TEXT("EquipParamWeapon"), EquipParamWeaponObject) && EquipParamWeaponObject)
			{
				CharacterAsset->EquipParamWeapon = ParseParamRows(*EquipParamWeaponObject);
			}
		}
	}
}

TArray<USekiroSkillDataAsset*> USekiroAssetImporter::ImportSkillConfig(
	const FString& JsonFilePath,
	const FString& OutputPackagePath)
{
	TArray<USekiroSkillDataAsset*> CreatedAssets;

	// Read the JSON file from disk.
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *JsonFilePath))
	{
		UE_LOG(LogTemp, Error, TEXT("SekiroAssetImporter: Failed to read JSON file: %s"), *JsonFilePath);
		return CreatedAssets;
	}

	// Parse JSON.
	TSharedPtr<FJsonObject> RootObject;
	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(JsonReader, RootObject) || !RootObject.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("SekiroAssetImporter: Failed to parse JSON from: %s"), *JsonFilePath);
		return CreatedAssets;
	}

	const FString DeliveryMode = RootObject->GetStringField(TEXT("deliveryMode"));
	if (DeliveryMode != TEXT("formal-only"))
	{
		UE_LOG(LogTemp, Error, TEXT("SekiroAssetImporter: skill_config.json must use deliveryMode=formal-only."));
		return CreatedAssets;
	}

	const TSharedPtr<FJsonObject>* ParamsObject = nullptr;
	if (!RootObject->TryGetObjectField(TEXT("params"), ParamsObject) || !ParamsObject || !ParamsObject->IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("SekiroAssetImporter: JSON missing canonical 'params' object."));
		return CreatedAssets;
	}

	// Iterate top-level "characters" array.
	const TArray<TSharedPtr<FJsonValue>>* CharactersArray = nullptr;
	if (!RootObject->TryGetArrayField(TEXT("characters"), CharactersArray))
	{
		UE_LOG(LogTemp, Error, TEXT("SekiroAssetImporter: JSON missing 'characters' array."));
		return CreatedAssets;
	}

	for (const TSharedPtr<FJsonValue>& CharacterValue : *CharactersArray)
	{
		const TSharedPtr<FJsonObject>& CharacterObj = CharacterValue->AsObject();
		if (!CharacterObj.IsValid())
		{
			continue;
		}

		const FString CharacterId = CharacterObj->GetStringField(TEXT("id"));
		const FString CharacterContentPath = GetParentPackagePath(OutputPackagePath);
		const FString CharacterAssetName = FString::Printf(TEXT("CHR_%s"), *CharacterId);
		const FString CharacterPackagePath = CharacterContentPath / CharacterAssetName;
		UPackage* CharacterPackage = CreatePackage(*CharacterPackagePath);
		CharacterPackage->FullyLoad();
		USekiroCharacterData* CharacterAsset = FindObject<USekiroCharacterData>(CharacterPackage, *CharacterAssetName);
		if (!CharacterAsset)
		{
			CharacterAsset = NewObject<USekiroCharacterData>(CharacterPackage, USekiroCharacterData::StaticClass(), *CharacterAssetName, RF_Public | RF_Standalone);
			FAssetRegistryModule::AssetCreated(CharacterAsset);
		}
		CharacterAsset->CharacterId = CharacterId;
		CharacterAsset->Skills.Reset();
		PopulateCharacterDataFromJson(CharacterAsset, CharacterObj, *ParamsObject, CharacterContentPath);

		// Iterate "animations" array within each character.
		const TArray<TSharedPtr<FJsonValue>>* AnimationsArray = nullptr;
		if (!CharacterObj->TryGetArrayField(TEXT("animations"), AnimationsArray))
		{
			UE_LOG(LogTemp, Warning, TEXT("SekiroAssetImporter: Character '%s' has no 'animations' array."), *CharacterId);
			continue;
		}

		for (const TSharedPtr<FJsonValue>& AnimValue : *AnimationsArray)
		{
			const TSharedPtr<FJsonObject>& AnimObj = AnimValue->AsObject();
			if (!AnimObj.IsValid())
			{
				continue;
			}

			const FString AnimationName = AnimObj->GetStringField(TEXT("name"));
			const FString SourceFileName = AnimObj->GetStringField(TEXT("fileName"));
			const int32 FrameCount = AnimObj->GetIntegerField(TEXT("frameCount"));
			const double FrameRate = AnimObj->GetNumberField(TEXT("frameRate"));

			// Build a sanitized asset name from character id + animation name.
			const FString AssetName = FString::Printf(TEXT("SK_%s_%s"), *CharacterId, *AnimationName);
			const FString PackagePath = FString::Printf(TEXT("%s/%s"), *OutputPackagePath, *AssetName);

			// Create the package for this asset.
			UPackage* Package = CreatePackage(*PackagePath);
			if (!Package)
			{
				UE_LOG(LogTemp, Error, TEXT("SekiroAssetImporter: Failed to create package: %s"), *PackagePath);
				continue;
			}
			Package->FullyLoad();

			// Create the data asset inside the package.
			USekiroSkillDataAsset* SkillAsset = NewObject<USekiroSkillDataAsset>(
				Package,
				USekiroSkillDataAsset::StaticClass(),
				*AssetName,
				RF_Public | RF_Standalone);

			if (!SkillAsset)
			{
				UE_LOG(LogTemp, Error, TEXT("SekiroAssetImporter: Failed to create data asset: %s"), *AssetName);
				continue;
			}

			// Populate fields.
			SkillAsset->CharacterId = CharacterId;
			SkillAsset->AnimationName = AnimationName;
			SkillAsset->SourceFileName = SourceFileName;
			SkillAsset->Animation = FindAnimationAsset(CharacterContentPath / TEXT("Animations"), SourceFileName);
			SkillAsset->CharacterData = TSoftObjectPtr<USekiroCharacterData>(CharacterAsset);
			SkillAsset->FrameCount = FrameCount;
			SkillAsset->FrameRate = static_cast<float>(FrameRate);
			SkillAsset->RootMotion = ParseRootMotionTrack(AnimObj);
			SkillAsset->Events = ParseTaeEventsFromJson(AnimObj);
			CharacterAsset->Skills.Add(TSoftObjectPtr<USekiroSkillDataAsset>(SkillAsset));

			// Mark dirty and notify the asset registry.
			SkillAsset->MarkPackageDirty();

			FAssetRegistryModule::AssetCreated(SkillAsset);

			// Save the package to disk.
			const FString PackageFilename = FPackageName::LongPackageNameToFilename(
				PackagePath, FPackageName::GetAssetPackageExtension());

			FSavePackageArgs SaveArgs;
			SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
			UPackage::SavePackage(Package, SkillAsset, *PackageFilename, SaveArgs);

			CreatedAssets.Add(SkillAsset);

			UE_LOG(LogTemp, Log, TEXT("SekiroAssetImporter: Created asset '%s' with %d events."),
				*AssetName, SkillAsset->Events.Num());
		}

		CharacterAsset->MarkPackageDirty();
		const FString CharacterPackageFilename = FPackageName::LongPackageNameToFilename(
			CharacterPackagePath, FPackageName::GetAssetPackageExtension());
		FSavePackageArgs CharacterSaveArgs;
		CharacterSaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		UPackage::SavePackage(CharacterPackage, CharacterAsset, *CharacterPackageFilename, CharacterSaveArgs);
	}

	UE_LOG(LogTemp, Log, TEXT("SekiroAssetImporter: Import complete. Created %d assets from '%s'."),
		CreatedAssets.Num(), *JsonFilePath);

	return CreatedAssets;
}

TArray<FSekiroTaeEvent> USekiroAssetImporter::ParseTaeEventsFromJson(
	const TSharedPtr<FJsonObject>& AnimJson)
{
	TArray<FSekiroTaeEvent> Events;

	if (!AnimJson.IsValid())
	{
		return Events;
	}

	const TArray<TSharedPtr<FJsonValue>>* EventsArray = nullptr;
	if (!AnimJson->TryGetArrayField(TEXT("events"), EventsArray))
	{
		return Events;
	}

	for (const TSharedPtr<FJsonValue>& EventValue : *EventsArray)
	{
		const TSharedPtr<FJsonObject>& EventObj = EventValue->AsObject();
		if (!EventObj.IsValid())
		{
			continue;
		}

		FSekiroTaeEvent TaeEvent;
		TaeEvent.Type = EventObj->GetIntegerField(TEXT("type"));
		TaeEvent.TypeName = EventObj->GetStringField(TEXT("typeName"));
		TaeEvent.Category = EventObj->GetStringField(TEXT("category"));
		TaeEvent.StartFrame = static_cast<float>(EventObj->GetNumberField(TEXT("startFrame")));
		TaeEvent.EndFrame = static_cast<float>(EventObj->GetNumberField(TEXT("endFrame")));

		const TArray<TSharedPtr<FJsonValue>>* ParamsArray = nullptr;
		if (EventObj->TryGetArrayField(TEXT("params"), ParamsArray))
		{
			for (const TSharedPtr<FJsonValue>& ParamValue : *ParamsArray)
			{
				const TSharedPtr<FJsonObject> ParamObj = ParamValue->AsObject();
				if (!ParamObj.IsValid())
				{
					continue;
				}

				FSekiroEventParam Param;
				Param.Name = ParamObj->GetStringField(TEXT("name"));
				Param.DataType = ParamObj->GetStringField(TEXT("dataType"));
				Param.ByteOffset = ParamObj->GetIntegerField(TEXT("byteOffset"));
				Param.Source = ParamObj->GetStringField(TEXT("source"));

				const TSharedPtr<FJsonValue>* ValueField = nullptr;
				if (ParamObj->TryGetField(TEXT("value"), ValueField) && ValueField)
				{
					Param.ValueJson = SerializeJsonValue(*ValueField);
					TaeEvent.Parameters.Add(Param.Name, Param.ValueJson);
				}

				TaeEvent.Params.Add(MoveTemp(Param));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("SekiroAssetImporter: Event type %d is missing canonical 'params' array."), TaeEvent.Type);
			continue;
		}

		const TSharedPtr<FJsonObject>* SemanticLinksObj = nullptr;
		if (EventObj->TryGetObjectField(TEXT("semanticLinks"), SemanticLinksObj) && SemanticLinksObj && SemanticLinksObj->IsValid())
		{
			for (const auto& Pair : (*SemanticLinksObj)->Values)
			{
				FSekiroSemanticLink Link;
				Link.Name = Pair.Key;
				Link.ValueJson = SerializeJsonValue(Pair.Value);
				TaeEvent.SemanticLinks.Add(MoveTemp(Link));
			}
		}

		Events.Add(MoveTemp(TaeEvent));
	}

	return Events;
}
