// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Data/SekiroSkillDataAsset.h"
#include "Data/SekiroTaeEvent.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeSekiroSkillDataAsset() {}

// ********** Begin Cross Module References ********************************************************
ENGINE_API UClass* Z_Construct_UClass_UDataAsset();
SEKIROSKILLEDITORPLUGIN_API UClass* Z_Construct_UClass_USekiroSkillDataAsset();
SEKIROSKILLEDITORPLUGIN_API UClass* Z_Construct_UClass_USekiroSkillDataAsset_NoRegister();
SEKIROSKILLEDITORPLUGIN_API UScriptStruct* Z_Construct_UScriptStruct_FSekiroTaeEvent();
UPackage* Z_Construct_UPackage__Script_SekiroSkillEditorPlugin();
// ********** End Cross Module References **********************************************************

// ********** Begin Class USekiroSkillDataAsset Function GetEventAtFrame ***************************
struct Z_Construct_UFunction_USekiroSkillDataAsset_GetEventAtFrame_Statics
{
	struct SekiroSkillDataAsset_eventGetEventAtFrame_Parms
	{
		float Frame;
		TArray<FSekiroTaeEvent> ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "Skill Data" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Returns all events that are active at the given frame\n\x09 * (StartFrame <= Frame <= EndFrame).\n\x09 * @param Frame  The frame to query.\n\x09 * @return Array of events active at that frame.\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/Data/SekiroSkillDataAsset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Returns all events that are active at the given frame\n(StartFrame <= Frame <= EndFrame).\n@param Frame  The frame to query.\n@return Array of events active at that frame." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Function GetEventAtFrame constinit property declarations ***********************
	static const UECodeGen_Private::FFloatPropertyParams NewProp_Frame;
	static const UECodeGen_Private::FStructPropertyParams NewProp_ReturnValue_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function GetEventAtFrame constinit property declarations *************************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function GetEventAtFrame Property Definitions **********************************
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UFunction_USekiroSkillDataAsset_GetEventAtFrame_Statics::NewProp_Frame = { "Frame", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(SekiroSkillDataAsset_eventGetEventAtFrame_Parms, Frame), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UFunction_USekiroSkillDataAsset_GetEventAtFrame_Statics::NewProp_ReturnValue_Inner = { "ReturnValue", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FSekiroTaeEvent, METADATA_PARAMS(0, nullptr) }; // 2260859055
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UFunction_USekiroSkillDataAsset_GetEventAtFrame_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(SekiroSkillDataAsset_eventGetEventAtFrame_Parms, ReturnValue), EArrayPropertyFlags::None, METADATA_PARAMS(0, nullptr) }; // 2260859055
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_USekiroSkillDataAsset_GetEventAtFrame_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_USekiroSkillDataAsset_GetEventAtFrame_Statics::NewProp_Frame,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_USekiroSkillDataAsset_GetEventAtFrame_Statics::NewProp_ReturnValue_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_USekiroSkillDataAsset_GetEventAtFrame_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_USekiroSkillDataAsset_GetEventAtFrame_Statics::PropPointers) < 2048);
// ********** End Function GetEventAtFrame Property Definitions ************************************
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_USekiroSkillDataAsset_GetEventAtFrame_Statics::FuncParams = { { (UObject*(*)())Z_Construct_UClass_USekiroSkillDataAsset, nullptr, "GetEventAtFrame", 	Z_Construct_UFunction_USekiroSkillDataAsset_GetEventAtFrame_Statics::PropPointers, 
	UE_ARRAY_COUNT(Z_Construct_UFunction_USekiroSkillDataAsset_GetEventAtFrame_Statics::PropPointers), 
sizeof(Z_Construct_UFunction_USekiroSkillDataAsset_GetEventAtFrame_Statics::SekiroSkillDataAsset_eventGetEventAtFrame_Parms),
RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_USekiroSkillDataAsset_GetEventAtFrame_Statics::Function_MetaDataParams), Z_Construct_UFunction_USekiroSkillDataAsset_GetEventAtFrame_Statics::Function_MetaDataParams)},  };
static_assert(sizeof(Z_Construct_UFunction_USekiroSkillDataAsset_GetEventAtFrame_Statics::SekiroSkillDataAsset_eventGetEventAtFrame_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_USekiroSkillDataAsset_GetEventAtFrame()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_USekiroSkillDataAsset_GetEventAtFrame_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(USekiroSkillDataAsset::execGetEventAtFrame)
{
	P_GET_PROPERTY(FFloatProperty,Z_Param_Frame);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(TArray<FSekiroTaeEvent>*)Z_Param__Result=P_THIS->GetEventAtFrame(Z_Param_Frame);
	P_NATIVE_END;
}
// ********** End Class USekiroSkillDataAsset Function GetEventAtFrame *****************************

// ********** Begin Class USekiroSkillDataAsset Function GetEventsByCategory ***********************
struct Z_Construct_UFunction_USekiroSkillDataAsset_GetEventsByCategory_Statics
{
	struct SekiroSkillDataAsset_eventGetEventsByCategory_Parms
	{
		FString InCategory;
		TArray<FSekiroTaeEvent> ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "Skill Data" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Returns all events that match the specified category.\n\x09 * @param InCategory  Category string to filter by (case-sensitive).\n\x09 * @return Array of matching events.\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/Data/SekiroSkillDataAsset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Returns all events that match the specified category.\n@param InCategory  Category string to filter by (case-sensitive).\n@return Array of matching events." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_InCategory_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA

// ********** Begin Function GetEventsByCategory constinit property declarations *******************
	static const UECodeGen_Private::FStrPropertyParams NewProp_InCategory;
	static const UECodeGen_Private::FStructPropertyParams NewProp_ReturnValue_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function GetEventsByCategory constinit property declarations *********************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function GetEventsByCategory Property Definitions ******************************
const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_USekiroSkillDataAsset_GetEventsByCategory_Statics::NewProp_InCategory = { "InCategory", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(SekiroSkillDataAsset_eventGetEventsByCategory_Parms, InCategory), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_InCategory_MetaData), NewProp_InCategory_MetaData) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UFunction_USekiroSkillDataAsset_GetEventsByCategory_Statics::NewProp_ReturnValue_Inner = { "ReturnValue", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FSekiroTaeEvent, METADATA_PARAMS(0, nullptr) }; // 2260859055
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UFunction_USekiroSkillDataAsset_GetEventsByCategory_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(SekiroSkillDataAsset_eventGetEventsByCategory_Parms, ReturnValue), EArrayPropertyFlags::None, METADATA_PARAMS(0, nullptr) }; // 2260859055
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_USekiroSkillDataAsset_GetEventsByCategory_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_USekiroSkillDataAsset_GetEventsByCategory_Statics::NewProp_InCategory,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_USekiroSkillDataAsset_GetEventsByCategory_Statics::NewProp_ReturnValue_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_USekiroSkillDataAsset_GetEventsByCategory_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_USekiroSkillDataAsset_GetEventsByCategory_Statics::PropPointers) < 2048);
// ********** End Function GetEventsByCategory Property Definitions ********************************
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_USekiroSkillDataAsset_GetEventsByCategory_Statics::FuncParams = { { (UObject*(*)())Z_Construct_UClass_USekiroSkillDataAsset, nullptr, "GetEventsByCategory", 	Z_Construct_UFunction_USekiroSkillDataAsset_GetEventsByCategory_Statics::PropPointers, 
	UE_ARRAY_COUNT(Z_Construct_UFunction_USekiroSkillDataAsset_GetEventsByCategory_Statics::PropPointers), 
sizeof(Z_Construct_UFunction_USekiroSkillDataAsset_GetEventsByCategory_Statics::SekiroSkillDataAsset_eventGetEventsByCategory_Parms),
RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_USekiroSkillDataAsset_GetEventsByCategory_Statics::Function_MetaDataParams), Z_Construct_UFunction_USekiroSkillDataAsset_GetEventsByCategory_Statics::Function_MetaDataParams)},  };
static_assert(sizeof(Z_Construct_UFunction_USekiroSkillDataAsset_GetEventsByCategory_Statics::SekiroSkillDataAsset_eventGetEventsByCategory_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_USekiroSkillDataAsset_GetEventsByCategory()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_USekiroSkillDataAsset_GetEventsByCategory_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(USekiroSkillDataAsset::execGetEventsByCategory)
{
	P_GET_PROPERTY(FStrProperty,Z_Param_InCategory);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(TArray<FSekiroTaeEvent>*)Z_Param__Result=P_THIS->GetEventsByCategory(Z_Param_InCategory);
	P_NATIVE_END;
}
// ********** End Class USekiroSkillDataAsset Function GetEventsByCategory *************************

// ********** Begin Class USekiroSkillDataAsset ****************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_USekiroSkillDataAsset;
UClass* USekiroSkillDataAsset::GetPrivateStaticClass()
{
	using TClass = USekiroSkillDataAsset;
	if (!Z_Registration_Info_UClass_USekiroSkillDataAsset.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("SekiroSkillDataAsset"),
			Z_Registration_Info_UClass_USekiroSkillDataAsset.InnerSingleton,
			StaticRegisterNativesUSekiroSkillDataAsset,
			sizeof(TClass),
			alignof(TClass),
			TClass::StaticClassFlags,
			TClass::StaticClassCastFlags(),
			TClass::StaticConfigName(),
			(UClass::ClassConstructorType)InternalConstructor<TClass>,
			(UClass::ClassVTableHelperCtorCallerType)InternalVTableHelperCtorCaller<TClass>,
			UOBJECT_CPPCLASS_STATICFUNCTIONS_FORCLASS(TClass),
			&TClass::Super::StaticClass,
			&TClass::WithinClass::StaticClass
		);
	}
	return Z_Registration_Info_UClass_USekiroSkillDataAsset.InnerSingleton;
}
UClass* Z_Construct_UClass_USekiroSkillDataAsset_NoRegister()
{
	return USekiroSkillDataAsset::GetPrivateStaticClass();
}
struct Z_Construct_UClass_USekiroSkillDataAsset_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * Data asset representing a single Sekiro skill / animation with its TAE events.\n */" },
#endif
		{ "IncludePath", "Data/SekiroSkillDataAsset.h" },
		{ "ModuleRelativePath", "Public/Data/SekiroSkillDataAsset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Data asset representing a single Sekiro skill / animation with its TAE events." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_CharacterId_MetaData[] = {
		{ "Category", "Skill Data" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Identifier of the character this skill belongs to. */" },
#endif
		{ "ModuleRelativePath", "Public/Data/SekiroSkillDataAsset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Identifier of the character this skill belongs to." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_AnimationName_MetaData[] = {
		{ "Category", "Skill Data" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Name of the source animation (e.g. \"a000_003000\"). */" },
#endif
		{ "ModuleRelativePath", "Public/Data/SekiroSkillDataAsset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Name of the source animation (e.g. \"a000_003000\")." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_FrameCount_MetaData[] = {
		{ "Category", "Skill Data" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Total number of frames in the animation. */" },
#endif
		{ "ModuleRelativePath", "Public/Data/SekiroSkillDataAsset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Total number of frames in the animation." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_FrameRate_MetaData[] = {
		{ "Category", "Skill Data" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Playback frame rate. Defaults to 30 fps (FromSoftware standard). */" },
#endif
		{ "ModuleRelativePath", "Public/Data/SekiroSkillDataAsset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Playback frame rate. Defaults to 30 fps (FromSoftware standard)." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Events_MetaData[] = {
		{ "Category", "Skill Data" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** All TAE events associated with this animation. */" },
#endif
		{ "ModuleRelativePath", "Public/Data/SekiroSkillDataAsset.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "All TAE events associated with this animation." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Class USekiroSkillDataAsset constinit property declarations ********************
	static const UECodeGen_Private::FStrPropertyParams NewProp_CharacterId;
	static const UECodeGen_Private::FStrPropertyParams NewProp_AnimationName;
	static const UECodeGen_Private::FIntPropertyParams NewProp_FrameCount;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_FrameRate;
	static const UECodeGen_Private::FStructPropertyParams NewProp_Events_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_Events;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class USekiroSkillDataAsset constinit property declarations **********************
	static constexpr UE::CodeGen::FClassNativeFunction Funcs[] = {
		{ .NameUTF8 = UTF8TEXT("GetEventAtFrame"), .Pointer = &USekiroSkillDataAsset::execGetEventAtFrame },
		{ .NameUTF8 = UTF8TEXT("GetEventsByCategory"), .Pointer = &USekiroSkillDataAsset::execGetEventsByCategory },
	};
	static UObject* (*const DependentSingletons[])();
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_USekiroSkillDataAsset_GetEventAtFrame, "GetEventAtFrame" }, // 1733831706
		{ &Z_Construct_UFunction_USekiroSkillDataAsset_GetEventsByCategory, "GetEventsByCategory" }, // 869027289
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<USekiroSkillDataAsset>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_USekiroSkillDataAsset_Statics

// ********** Begin Class USekiroSkillDataAsset Property Definitions *******************************
const UECodeGen_Private::FStrPropertyParams Z_Construct_UClass_USekiroSkillDataAsset_Statics::NewProp_CharacterId = { "CharacterId", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(USekiroSkillDataAsset, CharacterId), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_CharacterId_MetaData), NewProp_CharacterId_MetaData) };
const UECodeGen_Private::FStrPropertyParams Z_Construct_UClass_USekiroSkillDataAsset_Statics::NewProp_AnimationName = { "AnimationName", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(USekiroSkillDataAsset, AnimationName), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_AnimationName_MetaData), NewProp_AnimationName_MetaData) };
const UECodeGen_Private::FIntPropertyParams Z_Construct_UClass_USekiroSkillDataAsset_Statics::NewProp_FrameCount = { "FrameCount", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(USekiroSkillDataAsset, FrameCount), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_FrameCount_MetaData), NewProp_FrameCount_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_USekiroSkillDataAsset_Statics::NewProp_FrameRate = { "FrameRate", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(USekiroSkillDataAsset, FrameRate), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_FrameRate_MetaData), NewProp_FrameRate_MetaData) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_USekiroSkillDataAsset_Statics::NewProp_Events_Inner = { "Events", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FSekiroTaeEvent, METADATA_PARAMS(0, nullptr) }; // 2260859055
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_USekiroSkillDataAsset_Statics::NewProp_Events = { "Events", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(USekiroSkillDataAsset, Events), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Events_MetaData), NewProp_Events_MetaData) }; // 2260859055
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_USekiroSkillDataAsset_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_USekiroSkillDataAsset_Statics::NewProp_CharacterId,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_USekiroSkillDataAsset_Statics::NewProp_AnimationName,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_USekiroSkillDataAsset_Statics::NewProp_FrameCount,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_USekiroSkillDataAsset_Statics::NewProp_FrameRate,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_USekiroSkillDataAsset_Statics::NewProp_Events_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_USekiroSkillDataAsset_Statics::NewProp_Events,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_USekiroSkillDataAsset_Statics::PropPointers) < 2048);
// ********** End Class USekiroSkillDataAsset Property Definitions *********************************
UObject* (*const Z_Construct_UClass_USekiroSkillDataAsset_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UDataAsset,
	(UObject* (*)())Z_Construct_UPackage__Script_SekiroSkillEditorPlugin,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_USekiroSkillDataAsset_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_USekiroSkillDataAsset_Statics::ClassParams = {
	&USekiroSkillDataAsset::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	FuncInfo,
	Z_Construct_UClass_USekiroSkillDataAsset_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	UE_ARRAY_COUNT(FuncInfo),
	UE_ARRAY_COUNT(Z_Construct_UClass_USekiroSkillDataAsset_Statics::PropPointers),
	0,
	0x001000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_USekiroSkillDataAsset_Statics::Class_MetaDataParams), Z_Construct_UClass_USekiroSkillDataAsset_Statics::Class_MetaDataParams)
};
void USekiroSkillDataAsset::StaticRegisterNativesUSekiroSkillDataAsset()
{
	UClass* Class = USekiroSkillDataAsset::StaticClass();
	FNativeFunctionRegistrar::RegisterFunctions(Class, MakeConstArrayView(Z_Construct_UClass_USekiroSkillDataAsset_Statics::Funcs));
}
UClass* Z_Construct_UClass_USekiroSkillDataAsset()
{
	if (!Z_Registration_Info_UClass_USekiroSkillDataAsset.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_USekiroSkillDataAsset.OuterSingleton, Z_Construct_UClass_USekiroSkillDataAsset_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_USekiroSkillDataAsset.OuterSingleton;
}
USekiroSkillDataAsset::USekiroSkillDataAsset(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, USekiroSkillDataAsset);
USekiroSkillDataAsset::~USekiroSkillDataAsset() {}
// ********** End Class USekiroSkillDataAsset ******************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Data_SekiroSkillDataAsset_h__Script_SekiroSkillEditorPlugin_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_USekiroSkillDataAsset, USekiroSkillDataAsset::StaticClass, TEXT("USekiroSkillDataAsset"), &Z_Registration_Info_UClass_USekiroSkillDataAsset, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(USekiroSkillDataAsset), 818577926U) },
	};
}; // Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Data_SekiroSkillDataAsset_h__Script_SekiroSkillEditorPlugin_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Data_SekiroSkillDataAsset_h__Script_SekiroSkillEditorPlugin_2164962759{
	TEXT("/Script/SekiroSkillEditorPlugin"),
	Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Data_SekiroSkillDataAsset_h__Script_SekiroSkillEditorPlugin_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Data_SekiroSkillDataAsset_h__Script_SekiroSkillEditorPlugin_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
