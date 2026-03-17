// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "Data/SekiroSkillDataAsset.h"

#ifdef SEKIROSKILLEDITORPLUGIN_SekiroSkillDataAsset_generated_h
#error "SekiroSkillDataAsset.generated.h already included, missing '#pragma once' in SekiroSkillDataAsset.h"
#endif
#define SEKIROSKILLEDITORPLUGIN_SekiroSkillDataAsset_generated_h

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
struct FSekiroTaeEvent;

// ********** Begin Class USekiroSkillDataAsset ****************************************************
#define FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Data_SekiroSkillDataAsset_h_16_RPC_WRAPPERS_NO_PURE_DECLS \
	DECLARE_FUNCTION(execGetEventAtFrame); \
	DECLARE_FUNCTION(execGetEventsByCategory);


struct Z_Construct_UClass_USekiroSkillDataAsset_Statics;
SEKIROSKILLEDITORPLUGIN_API UClass* Z_Construct_UClass_USekiroSkillDataAsset_NoRegister();

#define FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Data_SekiroSkillDataAsset_h_16_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUSekiroSkillDataAsset(); \
	friend struct ::Z_Construct_UClass_USekiroSkillDataAsset_Statics; \
	static UClass* GetPrivateStaticClass(); \
	friend SEKIROSKILLEDITORPLUGIN_API UClass* ::Z_Construct_UClass_USekiroSkillDataAsset_NoRegister(); \
public: \
	DECLARE_CLASS2(USekiroSkillDataAsset, UDataAsset, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/SekiroSkillEditorPlugin"), Z_Construct_UClass_USekiroSkillDataAsset_NoRegister) \
	DECLARE_SERIALIZER(USekiroSkillDataAsset)


#define FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Data_SekiroSkillDataAsset_h_16_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API USekiroSkillDataAsset(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
	/** Deleted move- and copy-constructors, should never be used */ \
	USekiroSkillDataAsset(USekiroSkillDataAsset&&) = delete; \
	USekiroSkillDataAsset(const USekiroSkillDataAsset&) = delete; \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, USekiroSkillDataAsset); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(USekiroSkillDataAsset); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(USekiroSkillDataAsset) \
	NO_API virtual ~USekiroSkillDataAsset();


#define FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Data_SekiroSkillDataAsset_h_13_PROLOG
#define FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Data_SekiroSkillDataAsset_h_16_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Data_SekiroSkillDataAsset_h_16_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Data_SekiroSkillDataAsset_h_16_INCLASS_NO_PURE_DECLS \
	FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Data_SekiroSkillDataAsset_h_16_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


class USekiroSkillDataAsset;

// ********** End Class USekiroSkillDataAsset ******************************************************

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Data_SekiroSkillDataAsset_h

PRAGMA_ENABLE_DEPRECATION_WARNINGS
