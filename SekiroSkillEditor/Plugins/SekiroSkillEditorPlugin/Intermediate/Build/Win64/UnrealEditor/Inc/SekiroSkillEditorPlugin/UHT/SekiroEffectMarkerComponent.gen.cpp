// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Visualization/SekiroEffectMarkerComponent.h"
#include "Data/SekiroTaeEvent.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeSekiroEffectMarkerComponent() {}

// ********** Begin Cross Module References ********************************************************
ENGINE_API UClass* Z_Construct_UClass_UActorComponent();
SEKIROSKILLEDITORPLUGIN_API UClass* Z_Construct_UClass_USekiroEffectMarkerComponent();
SEKIROSKILLEDITORPLUGIN_API UClass* Z_Construct_UClass_USekiroEffectMarkerComponent_NoRegister();
SEKIROSKILLEDITORPLUGIN_API UScriptStruct* Z_Construct_UScriptStruct_FSekiroTaeEvent();
UPackage* Z_Construct_UPackage__Script_SekiroSkillEditorPlugin();
// ********** End Cross Module References **********************************************************

// ********** Begin Class USekiroEffectMarkerComponent Function UpdateActiveEvents *****************
struct Z_Construct_UFunction_USekiroEffectMarkerComponent_UpdateActiveEvents_Statics
{
	struct SekiroEffectMarkerComponent_eventUpdateActiveEvents_Parms
	{
		float CurrentFrame;
		TArray<FSekiroTaeEvent> AllEvents;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "Effect Visualization" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Filters AllEvents to those active at CurrentFrame whose Category\n\x09 * is \"Effect\" or \"Sound\", storing the result in ActiveEvents.\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/Visualization/SekiroEffectMarkerComponent.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Filters AllEvents to those active at CurrentFrame whose Category\nis \"Effect\" or \"Sound\", storing the result in ActiveEvents." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_AllEvents_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA

// ********** Begin Function UpdateActiveEvents constinit property declarations ********************
	static const UECodeGen_Private::FFloatPropertyParams NewProp_CurrentFrame;
	static const UECodeGen_Private::FStructPropertyParams NewProp_AllEvents_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_AllEvents;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function UpdateActiveEvents constinit property declarations **********************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function UpdateActiveEvents Property Definitions *******************************
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UFunction_USekiroEffectMarkerComponent_UpdateActiveEvents_Statics::NewProp_CurrentFrame = { "CurrentFrame", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(SekiroEffectMarkerComponent_eventUpdateActiveEvents_Parms, CurrentFrame), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UFunction_USekiroEffectMarkerComponent_UpdateActiveEvents_Statics::NewProp_AllEvents_Inner = { "AllEvents", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FSekiroTaeEvent, METADATA_PARAMS(0, nullptr) }; // 2260859055
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UFunction_USekiroEffectMarkerComponent_UpdateActiveEvents_Statics::NewProp_AllEvents = { "AllEvents", nullptr, (EPropertyFlags)0x0010000008000182, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(SekiroEffectMarkerComponent_eventUpdateActiveEvents_Parms, AllEvents), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_AllEvents_MetaData), NewProp_AllEvents_MetaData) }; // 2260859055
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_USekiroEffectMarkerComponent_UpdateActiveEvents_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_USekiroEffectMarkerComponent_UpdateActiveEvents_Statics::NewProp_CurrentFrame,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_USekiroEffectMarkerComponent_UpdateActiveEvents_Statics::NewProp_AllEvents_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_USekiroEffectMarkerComponent_UpdateActiveEvents_Statics::NewProp_AllEvents,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_USekiroEffectMarkerComponent_UpdateActiveEvents_Statics::PropPointers) < 2048);
// ********** End Function UpdateActiveEvents Property Definitions *********************************
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_USekiroEffectMarkerComponent_UpdateActiveEvents_Statics::FuncParams = { { (UObject*(*)())Z_Construct_UClass_USekiroEffectMarkerComponent, nullptr, "UpdateActiveEvents", 	Z_Construct_UFunction_USekiroEffectMarkerComponent_UpdateActiveEvents_Statics::PropPointers, 
	UE_ARRAY_COUNT(Z_Construct_UFunction_USekiroEffectMarkerComponent_UpdateActiveEvents_Statics::PropPointers), 
sizeof(Z_Construct_UFunction_USekiroEffectMarkerComponent_UpdateActiveEvents_Statics::SekiroEffectMarkerComponent_eventUpdateActiveEvents_Parms),
RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04420401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_USekiroEffectMarkerComponent_UpdateActiveEvents_Statics::Function_MetaDataParams), Z_Construct_UFunction_USekiroEffectMarkerComponent_UpdateActiveEvents_Statics::Function_MetaDataParams)},  };
static_assert(sizeof(Z_Construct_UFunction_USekiroEffectMarkerComponent_UpdateActiveEvents_Statics::SekiroEffectMarkerComponent_eventUpdateActiveEvents_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_USekiroEffectMarkerComponent_UpdateActiveEvents()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_USekiroEffectMarkerComponent_UpdateActiveEvents_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(USekiroEffectMarkerComponent::execUpdateActiveEvents)
{
	P_GET_PROPERTY(FFloatProperty,Z_Param_CurrentFrame);
	P_GET_TARRAY_REF(FSekiroTaeEvent,Z_Param_Out_AllEvents);
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->UpdateActiveEvents(Z_Param_CurrentFrame,Z_Param_Out_AllEvents);
	P_NATIVE_END;
}
// ********** End Class USekiroEffectMarkerComponent Function UpdateActiveEvents *******************

// ********** Begin Class USekiroEffectMarkerComponent *********************************************
FClassRegistrationInfo Z_Registration_Info_UClass_USekiroEffectMarkerComponent;
UClass* USekiroEffectMarkerComponent::GetPrivateStaticClass()
{
	using TClass = USekiroEffectMarkerComponent;
	if (!Z_Registration_Info_UClass_USekiroEffectMarkerComponent.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("SekiroEffectMarkerComponent"),
			Z_Registration_Info_UClass_USekiroEffectMarkerComponent.InnerSingleton,
			StaticRegisterNativesUSekiroEffectMarkerComponent,
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
	return Z_Registration_Info_UClass_USekiroEffectMarkerComponent.InnerSingleton;
}
UClass* Z_Construct_UClass_USekiroEffectMarkerComponent_NoRegister()
{
	return USekiroEffectMarkerComponent::GetPrivateStaticClass();
}
struct Z_Construct_UClass_USekiroEffectMarkerComponent_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "BlueprintSpawnableComponent", "" },
		{ "ClassGroupNames", "Sekiro" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * Draws debug markers for Effect and Sound TAE events.\n * Blue spheres indicate SFX/VFX spawn points; green diamonds indicate\n * sound trigger locations. Attach to ASekiroSkillPreviewActor.\n */" },
#endif
		{ "IncludePath", "Visualization/SekiroEffectMarkerComponent.h" },
		{ "ModuleRelativePath", "Public/Visualization/SekiroEffectMarkerComponent.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Draws debug markers for Effect and Sound TAE events.\nBlue spheres indicate SFX/VFX spawn points; green diamonds indicate\nsound trigger locations. Attach to ASekiroSkillPreviewActor." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ActiveEvents_MetaData[] = {
		{ "Category", "Effect Visualization" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** TAE events of Effect/Sound categories active at the current frame. */" },
#endif
		{ "ModuleRelativePath", "Public/Visualization/SekiroEffectMarkerComponent.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "TAE events of Effect/Sound categories active at the current frame." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_bShowMarkers_MetaData[] = {
		{ "Category", "Effect Visualization" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Whether effect/sound marker drawing is enabled. */" },
#endif
		{ "ModuleRelativePath", "Public/Visualization/SekiroEffectMarkerComponent.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Whether effect/sound marker drawing is enabled." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_MarkerSize_MetaData[] = {
		{ "Category", "Effect Visualization" },
		{ "ClampMin", "1.0" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Base size of the debug markers in Unreal units. */" },
#endif
		{ "ModuleRelativePath", "Public/Visualization/SekiroEffectMarkerComponent.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Base size of the debug markers in Unreal units." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Class USekiroEffectMarkerComponent constinit property declarations *************
	static const UECodeGen_Private::FStructPropertyParams NewProp_ActiveEvents_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_ActiveEvents;
	static void NewProp_bShowMarkers_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_bShowMarkers;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_MarkerSize;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class USekiroEffectMarkerComponent constinit property declarations ***************
	static constexpr UE::CodeGen::FClassNativeFunction Funcs[] = {
		{ .NameUTF8 = UTF8TEXT("UpdateActiveEvents"), .Pointer = &USekiroEffectMarkerComponent::execUpdateActiveEvents },
	};
	static UObject* (*const DependentSingletons[])();
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_USekiroEffectMarkerComponent_UpdateActiveEvents, "UpdateActiveEvents" }, // 1554094532
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<USekiroEffectMarkerComponent>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_USekiroEffectMarkerComponent_Statics

// ********** Begin Class USekiroEffectMarkerComponent Property Definitions ************************
const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_USekiroEffectMarkerComponent_Statics::NewProp_ActiveEvents_Inner = { "ActiveEvents", nullptr, (EPropertyFlags)0x0000000000020000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FSekiroTaeEvent, METADATA_PARAMS(0, nullptr) }; // 2260859055
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_USekiroEffectMarkerComponent_Statics::NewProp_ActiveEvents = { "ActiveEvents", nullptr, (EPropertyFlags)0x0010000000022015, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(USekiroEffectMarkerComponent, ActiveEvents), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ActiveEvents_MetaData), NewProp_ActiveEvents_MetaData) }; // 2260859055
void Z_Construct_UClass_USekiroEffectMarkerComponent_Statics::NewProp_bShowMarkers_SetBit(void* Obj)
{
	((USekiroEffectMarkerComponent*)Obj)->bShowMarkers = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_USekiroEffectMarkerComponent_Statics::NewProp_bShowMarkers = { "bShowMarkers", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(USekiroEffectMarkerComponent), &Z_Construct_UClass_USekiroEffectMarkerComponent_Statics::NewProp_bShowMarkers_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_bShowMarkers_MetaData), NewProp_bShowMarkers_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_USekiroEffectMarkerComponent_Statics::NewProp_MarkerSize = { "MarkerSize", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(USekiroEffectMarkerComponent, MarkerSize), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_MarkerSize_MetaData), NewProp_MarkerSize_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_USekiroEffectMarkerComponent_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_USekiroEffectMarkerComponent_Statics::NewProp_ActiveEvents_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_USekiroEffectMarkerComponent_Statics::NewProp_ActiveEvents,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_USekiroEffectMarkerComponent_Statics::NewProp_bShowMarkers,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_USekiroEffectMarkerComponent_Statics::NewProp_MarkerSize,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_USekiroEffectMarkerComponent_Statics::PropPointers) < 2048);
// ********** End Class USekiroEffectMarkerComponent Property Definitions **************************
UObject* (*const Z_Construct_UClass_USekiroEffectMarkerComponent_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UActorComponent,
	(UObject* (*)())Z_Construct_UPackage__Script_SekiroSkillEditorPlugin,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_USekiroEffectMarkerComponent_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_USekiroEffectMarkerComponent_Statics::ClassParams = {
	&USekiroEffectMarkerComponent::StaticClass,
	"Engine",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	FuncInfo,
	Z_Construct_UClass_USekiroEffectMarkerComponent_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	UE_ARRAY_COUNT(FuncInfo),
	UE_ARRAY_COUNT(Z_Construct_UClass_USekiroEffectMarkerComponent_Statics::PropPointers),
	0,
	0x00B000A4u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_USekiroEffectMarkerComponent_Statics::Class_MetaDataParams), Z_Construct_UClass_USekiroEffectMarkerComponent_Statics::Class_MetaDataParams)
};
void USekiroEffectMarkerComponent::StaticRegisterNativesUSekiroEffectMarkerComponent()
{
	UClass* Class = USekiroEffectMarkerComponent::StaticClass();
	FNativeFunctionRegistrar::RegisterFunctions(Class, MakeConstArrayView(Z_Construct_UClass_USekiroEffectMarkerComponent_Statics::Funcs));
}
UClass* Z_Construct_UClass_USekiroEffectMarkerComponent()
{
	if (!Z_Registration_Info_UClass_USekiroEffectMarkerComponent.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_USekiroEffectMarkerComponent.OuterSingleton, Z_Construct_UClass_USekiroEffectMarkerComponent_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_USekiroEffectMarkerComponent.OuterSingleton;
}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, USekiroEffectMarkerComponent);
USekiroEffectMarkerComponent::~USekiroEffectMarkerComponent() {}
// ********** End Class USekiroEffectMarkerComponent ***********************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Visualization_SekiroEffectMarkerComponent_h__Script_SekiroSkillEditorPlugin_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_USekiroEffectMarkerComponent, USekiroEffectMarkerComponent::StaticClass, TEXT("USekiroEffectMarkerComponent"), &Z_Registration_Info_UClass_USekiroEffectMarkerComponent, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(USekiroEffectMarkerComponent), 2009715132U) },
	};
}; // Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Visualization_SekiroEffectMarkerComponent_h__Script_SekiroSkillEditorPlugin_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Visualization_SekiroEffectMarkerComponent_h__Script_SekiroSkillEditorPlugin_332116794{
	TEXT("/Script/SekiroSkillEditorPlugin"),
	Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Visualization_SekiroEffectMarkerComponent_h__Script_SekiroSkillEditorPlugin_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Visualization_SekiroEffectMarkerComponent_h__Script_SekiroSkillEditorPlugin_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
