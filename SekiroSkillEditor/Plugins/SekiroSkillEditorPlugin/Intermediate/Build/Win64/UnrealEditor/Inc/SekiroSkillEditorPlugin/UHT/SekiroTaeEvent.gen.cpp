// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Data/SekiroTaeEvent.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeSekiroTaeEvent() {}

// ********** Begin Cross Module References ********************************************************
SEKIROSKILLEDITORPLUGIN_API UScriptStruct* Z_Construct_UScriptStruct_FSekiroTaeEvent();
UPackage* Z_Construct_UPackage__Script_SekiroSkillEditorPlugin();
// ********** End Cross Module References **********************************************************

// ********** Begin ScriptStruct FSekiroTaeEvent ***************************************************
struct Z_Construct_UScriptStruct_FSekiroTaeEvent_Statics
{
	static inline consteval int32 GetStructSize() { return sizeof(FSekiroTaeEvent); }
	static inline consteval int16 GetStructAlignment() { return alignof(FSekiroTaeEvent); }
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * Represents a single TAE (TimeAct Event) from a Sekiro animation file.\n * Each event has a type, category, time range, and parameter map.\n */" },
#endif
		{ "ModuleRelativePath", "Public/Data/SekiroTaeEvent.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Represents a single TAE (TimeAct Event) from a Sekiro animation file.\nEach event has a type, category, time range, and parameter map." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Type_MetaData[] = {
		{ "Category", "TAE Event" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Numeric event type ID from the TAE file. */" },
#endif
		{ "ModuleRelativePath", "Public/Data/SekiroTaeEvent.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Numeric event type ID from the TAE file." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_TypeName_MetaData[] = {
		{ "Category", "TAE Event" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Human-readable name for this event type. */" },
#endif
		{ "ModuleRelativePath", "Public/Data/SekiroTaeEvent.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Human-readable name for this event type." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Category_MetaData[] = {
		{ "Category", "TAE Event" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Category grouping (e.g. Attack, Effect, Sound, Movement). */" },
#endif
		{ "ModuleRelativePath", "Public/Data/SekiroTaeEvent.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Category grouping (e.g. Attack, Effect, Sound, Movement)." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_StartFrame_MetaData[] = {
		{ "Category", "TAE Event" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Frame at which the event begins. */" },
#endif
		{ "ModuleRelativePath", "Public/Data/SekiroTaeEvent.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Frame at which the event begins." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_EndFrame_MetaData[] = {
		{ "Category", "TAE Event" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Frame at which the event ends. */" },
#endif
		{ "ModuleRelativePath", "Public/Data/SekiroTaeEvent.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Frame at which the event ends." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Parameters_MetaData[] = {
		{ "Category", "TAE Event" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Arbitrary key-value parameter map parsed from the TAE event data. */" },
#endif
		{ "ModuleRelativePath", "Public/Data/SekiroTaeEvent.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Arbitrary key-value parameter map parsed from the TAE event data." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin ScriptStruct FSekiroTaeEvent constinit property declarations *******************
	static const UECodeGen_Private::FIntPropertyParams NewProp_Type;
	static const UECodeGen_Private::FStrPropertyParams NewProp_TypeName;
	static const UECodeGen_Private::FStrPropertyParams NewProp_Category;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_StartFrame;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_EndFrame;
	static const UECodeGen_Private::FStrPropertyParams NewProp_Parameters_ValueProp;
	static const UECodeGen_Private::FStrPropertyParams NewProp_Parameters_Key_KeyProp;
	static const UECodeGen_Private::FMapPropertyParams NewProp_Parameters;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End ScriptStruct FSekiroTaeEvent constinit property declarations *********************
	static void* NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FSekiroTaeEvent>();
	}
	static const UECodeGen_Private::FStructParams StructParams;
}; // struct Z_Construct_UScriptStruct_FSekiroTaeEvent_Statics
static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_FSekiroTaeEvent;
class UScriptStruct* FSekiroTaeEvent::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_FSekiroTaeEvent.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_FSekiroTaeEvent.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FSekiroTaeEvent, (UObject*)Z_Construct_UPackage__Script_SekiroSkillEditorPlugin(), TEXT("SekiroTaeEvent"));
	}
	return Z_Registration_Info_UScriptStruct_FSekiroTaeEvent.OuterSingleton;
	}

// ********** Begin ScriptStruct FSekiroTaeEvent Property Definitions ******************************
const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FSekiroTaeEvent_Statics::NewProp_Type = { "Type", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FSekiroTaeEvent, Type), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Type_MetaData), NewProp_Type_MetaData) };
const UECodeGen_Private::FStrPropertyParams Z_Construct_UScriptStruct_FSekiroTaeEvent_Statics::NewProp_TypeName = { "TypeName", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FSekiroTaeEvent, TypeName), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_TypeName_MetaData), NewProp_TypeName_MetaData) };
const UECodeGen_Private::FStrPropertyParams Z_Construct_UScriptStruct_FSekiroTaeEvent_Statics::NewProp_Category = { "Category", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FSekiroTaeEvent, Category), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Category_MetaData), NewProp_Category_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FSekiroTaeEvent_Statics::NewProp_StartFrame = { "StartFrame", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FSekiroTaeEvent, StartFrame), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_StartFrame_MetaData), NewProp_StartFrame_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FSekiroTaeEvent_Statics::NewProp_EndFrame = { "EndFrame", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FSekiroTaeEvent, EndFrame), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_EndFrame_MetaData), NewProp_EndFrame_MetaData) };
const UECodeGen_Private::FStrPropertyParams Z_Construct_UScriptStruct_FSekiroTaeEvent_Statics::NewProp_Parameters_ValueProp = { "Parameters", nullptr, (EPropertyFlags)0x0000000000000001, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 1, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams Z_Construct_UScriptStruct_FSekiroTaeEvent_Statics::NewProp_Parameters_Key_KeyProp = { "Parameters_Key", nullptr, (EPropertyFlags)0x0000000000000001, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FMapPropertyParams Z_Construct_UScriptStruct_FSekiroTaeEvent_Statics::NewProp_Parameters = { "Parameters", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Map, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FSekiroTaeEvent, Parameters), EMapPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Parameters_MetaData), NewProp_Parameters_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FSekiroTaeEvent_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FSekiroTaeEvent_Statics::NewProp_Type,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FSekiroTaeEvent_Statics::NewProp_TypeName,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FSekiroTaeEvent_Statics::NewProp_Category,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FSekiroTaeEvent_Statics::NewProp_StartFrame,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FSekiroTaeEvent_Statics::NewProp_EndFrame,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FSekiroTaeEvent_Statics::NewProp_Parameters_ValueProp,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FSekiroTaeEvent_Statics::NewProp_Parameters_Key_KeyProp,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FSekiroTaeEvent_Statics::NewProp_Parameters,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FSekiroTaeEvent_Statics::PropPointers) < 2048);
// ********** End ScriptStruct FSekiroTaeEvent Property Definitions ********************************
const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FSekiroTaeEvent_Statics::StructParams = {
	(UObject* (*)())Z_Construct_UPackage__Script_SekiroSkillEditorPlugin,
	nullptr,
	&NewStructOps,
	"SekiroTaeEvent",
	Z_Construct_UScriptStruct_FSekiroTaeEvent_Statics::PropPointers,
	UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FSekiroTaeEvent_Statics::PropPointers),
	sizeof(FSekiroTaeEvent),
	alignof(FSekiroTaeEvent),
	RF_Public|RF_Transient|RF_MarkAsNative,
	EStructFlags(0x00000201),
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FSekiroTaeEvent_Statics::Struct_MetaDataParams), Z_Construct_UScriptStruct_FSekiroTaeEvent_Statics::Struct_MetaDataParams)
};
UScriptStruct* Z_Construct_UScriptStruct_FSekiroTaeEvent()
{
	if (!Z_Registration_Info_UScriptStruct_FSekiroTaeEvent.InnerSingleton)
	{
		UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_FSekiroTaeEvent.InnerSingleton, Z_Construct_UScriptStruct_FSekiroTaeEvent_Statics::StructParams);
	}
	return CastChecked<UScriptStruct>(Z_Registration_Info_UScriptStruct_FSekiroTaeEvent.InnerSingleton);
}
// ********** End ScriptStruct FSekiroTaeEvent *****************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Data_SekiroTaeEvent_h__Script_SekiroSkillEditorPlugin_Statics
{
	static constexpr FStructRegisterCompiledInInfo ScriptStructInfo[] = {
		{ FSekiroTaeEvent::StaticStruct, Z_Construct_UScriptStruct_FSekiroTaeEvent_Statics::NewStructOps, TEXT("SekiroTaeEvent"),&Z_Registration_Info_UScriptStruct_FSekiroTaeEvent, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FSekiroTaeEvent), 2260859055U) },
	};
}; // Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Data_SekiroTaeEvent_h__Script_SekiroSkillEditorPlugin_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Data_SekiroTaeEvent_h__Script_SekiroSkillEditorPlugin_781999390{
	TEXT("/Script/SekiroSkillEditorPlugin"),
	nullptr, 0,
	Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Data_SekiroTaeEvent_h__Script_SekiroSkillEditorPlugin_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Data_SekiroTaeEvent_h__Script_SekiroSkillEditorPlugin_Statics::ScriptStructInfo),
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
