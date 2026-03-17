// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "Data/SekiroCharacterData.h"

#ifdef SEKIROSKILLEDITORPLUGIN_SekiroCharacterData_generated_h
#error "SekiroCharacterData.generated.h already included, missing '#pragma once' in SekiroCharacterData.h"
#endif
#define SEKIROSKILLEDITORPLUGIN_SekiroCharacterData_generated_h

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

// ********** Begin Class USekiroCharacterData *****************************************************
struct Z_Construct_UClass_USekiroCharacterData_Statics;
SEKIROSKILLEDITORPLUGIN_API UClass* Z_Construct_UClass_USekiroCharacterData_NoRegister();

#define FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Data_SekiroCharacterData_h_20_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUSekiroCharacterData(); \
	friend struct ::Z_Construct_UClass_USekiroCharacterData_Statics; \
	static UClass* GetPrivateStaticClass(); \
	friend SEKIROSKILLEDITORPLUGIN_API UClass* ::Z_Construct_UClass_USekiroCharacterData_NoRegister(); \
public: \
	DECLARE_CLASS2(USekiroCharacterData, UDataAsset, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/SekiroSkillEditorPlugin"), Z_Construct_UClass_USekiroCharacterData_NoRegister) \
	DECLARE_SERIALIZER(USekiroCharacterData)


#define FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Data_SekiroCharacterData_h_20_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API USekiroCharacterData(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
	/** Deleted move- and copy-constructors, should never be used */ \
	USekiroCharacterData(USekiroCharacterData&&) = delete; \
	USekiroCharacterData(const USekiroCharacterData&) = delete; \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, USekiroCharacterData); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(USekiroCharacterData); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(USekiroCharacterData) \
	NO_API virtual ~USekiroCharacterData();


#define FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Data_SekiroCharacterData_h_17_PROLOG
#define FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Data_SekiroCharacterData_h_20_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Data_SekiroCharacterData_h_20_INCLASS_NO_PURE_DECLS \
	FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Data_SekiroCharacterData_h_20_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


class USekiroCharacterData;

// ********** End Class USekiroCharacterData *******************************************************

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Data_SekiroCharacterData_h

PRAGMA_ENABLE_DEPRECATION_WARNINGS
