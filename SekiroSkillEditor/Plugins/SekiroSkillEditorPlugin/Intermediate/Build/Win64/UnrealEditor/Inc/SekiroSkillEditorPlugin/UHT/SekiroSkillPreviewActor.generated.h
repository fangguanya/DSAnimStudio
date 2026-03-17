// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "Visualization/SekiroSkillPreviewActor.h"

#ifdef SEKIROSKILLEDITORPLUGIN_SekiroSkillPreviewActor_generated_h
#error "SekiroSkillPreviewActor.generated.h already included, missing '#pragma once' in SekiroSkillPreviewActor.h"
#endif
#define SEKIROSKILLEDITORPLUGIN_SekiroSkillPreviewActor_generated_h

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
class UAnimSequence;
class USekiroCharacterData;

// ********** Begin Class ASekiroSkillPreviewActor *************************************************
#define FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Visualization_SekiroSkillPreviewActor_h_20_RPC_WRAPPERS_NO_PURE_DECLS \
	DECLARE_FUNCTION(execGetCurrentFrame); \
	DECLARE_FUNCTION(execSetPlaybackPosition); \
	DECLARE_FUNCTION(execPlayAnimation); \
	DECLARE_FUNCTION(execSetCharacterData);


struct Z_Construct_UClass_ASekiroSkillPreviewActor_Statics;
SEKIROSKILLEDITORPLUGIN_API UClass* Z_Construct_UClass_ASekiroSkillPreviewActor_NoRegister();

#define FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Visualization_SekiroSkillPreviewActor_h_20_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesASekiroSkillPreviewActor(); \
	friend struct ::Z_Construct_UClass_ASekiroSkillPreviewActor_Statics; \
	static UClass* GetPrivateStaticClass(); \
	friend SEKIROSKILLEDITORPLUGIN_API UClass* ::Z_Construct_UClass_ASekiroSkillPreviewActor_NoRegister(); \
public: \
	DECLARE_CLASS2(ASekiroSkillPreviewActor, AActor, COMPILED_IN_FLAGS(0 | CLASS_Config), CASTCLASS_None, TEXT("/Script/SekiroSkillEditorPlugin"), Z_Construct_UClass_ASekiroSkillPreviewActor_NoRegister) \
	DECLARE_SERIALIZER(ASekiroSkillPreviewActor)


#define FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Visualization_SekiroSkillPreviewActor_h_20_ENHANCED_CONSTRUCTORS \
	/** Deleted move- and copy-constructors, should never be used */ \
	ASekiroSkillPreviewActor(ASekiroSkillPreviewActor&&) = delete; \
	ASekiroSkillPreviewActor(const ASekiroSkillPreviewActor&) = delete; \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, ASekiroSkillPreviewActor); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(ASekiroSkillPreviewActor); \
	DEFINE_DEFAULT_CONSTRUCTOR_CALL(ASekiroSkillPreviewActor) \
	NO_API virtual ~ASekiroSkillPreviewActor();


#define FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Visualization_SekiroSkillPreviewActor_h_17_PROLOG
#define FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Visualization_SekiroSkillPreviewActor_h_20_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Visualization_SekiroSkillPreviewActor_h_20_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Visualization_SekiroSkillPreviewActor_h_20_INCLASS_NO_PURE_DECLS \
	FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Visualization_SekiroSkillPreviewActor_h_20_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


class ASekiroSkillPreviewActor;

// ********** End Class ASekiroSkillPreviewActor ***************************************************

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Visualization_SekiroSkillPreviewActor_h

PRAGMA_ENABLE_DEPRECATION_WARNINGS
