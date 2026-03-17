// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "Import/SekiroAssetImporter.h"

#ifdef SEKIROSKILLEDITORPLUGIN_SekiroAssetImporter_generated_h
#error "SekiroAssetImporter.generated.h already included, missing '#pragma once' in SekiroAssetImporter.h"
#endif
#define SEKIROSKILLEDITORPLUGIN_SekiroAssetImporter_generated_h

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
class USekiroSkillDataAsset;

// ********** Begin Class USekiroAssetImporter *****************************************************
#define FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Import_SekiroAssetImporter_h_20_RPC_WRAPPERS_NO_PURE_DECLS \
	DECLARE_FUNCTION(execImportSkillConfig);


struct Z_Construct_UClass_USekiroAssetImporter_Statics;
SEKIROSKILLEDITORPLUGIN_API UClass* Z_Construct_UClass_USekiroAssetImporter_NoRegister();

#define FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Import_SekiroAssetImporter_h_20_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUSekiroAssetImporter(); \
	friend struct ::Z_Construct_UClass_USekiroAssetImporter_Statics; \
	static UClass* GetPrivateStaticClass(); \
	friend SEKIROSKILLEDITORPLUGIN_API UClass* ::Z_Construct_UClass_USekiroAssetImporter_NoRegister(); \
public: \
	DECLARE_CLASS2(USekiroAssetImporter, UObject, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/SekiroSkillEditorPlugin"), Z_Construct_UClass_USekiroAssetImporter_NoRegister) \
	DECLARE_SERIALIZER(USekiroAssetImporter)


#define FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Import_SekiroAssetImporter_h_20_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API USekiroAssetImporter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
	/** Deleted move- and copy-constructors, should never be used */ \
	USekiroAssetImporter(USekiroAssetImporter&&) = delete; \
	USekiroAssetImporter(const USekiroAssetImporter&) = delete; \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, USekiroAssetImporter); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(USekiroAssetImporter); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(USekiroAssetImporter) \
	NO_API virtual ~USekiroAssetImporter();


#define FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Import_SekiroAssetImporter_h_17_PROLOG
#define FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Import_SekiroAssetImporter_h_20_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Import_SekiroAssetImporter_h_20_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Import_SekiroAssetImporter_h_20_INCLASS_NO_PURE_DECLS \
	FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Import_SekiroAssetImporter_h_20_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


class USekiroAssetImporter;

// ********** End Class USekiroAssetImporter *******************************************************

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Import_SekiroAssetImporter_h

PRAGMA_ENABLE_DEPRECATION_WARNINGS
