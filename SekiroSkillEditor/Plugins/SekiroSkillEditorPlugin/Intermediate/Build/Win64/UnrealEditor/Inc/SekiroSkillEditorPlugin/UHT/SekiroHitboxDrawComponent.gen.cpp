// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Visualization/SekiroHitboxDrawComponent.h"
#include "Data/SekiroTaeEvent.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeSekiroHitboxDrawComponent() {}

// ********** Begin Cross Module References ********************************************************
ENGINE_API UClass* Z_Construct_UClass_UActorComponent();
SEKIROSKILLEDITORPLUGIN_API UClass* Z_Construct_UClass_USekiroHitboxDrawComponent();
SEKIROSKILLEDITORPLUGIN_API UClass* Z_Construct_UClass_USekiroHitboxDrawComponent_NoRegister();
SEKIROSKILLEDITORPLUGIN_API UScriptStruct* Z_Construct_UScriptStruct_FSekiroTaeEvent();
UPackage* Z_Construct_UPackage__Script_SekiroSkillEditorPlugin();
// ********** End Cross Module References **********************************************************

// ********** Begin Class USekiroHitboxDrawComponent Function UpdateActiveEvents *******************
struct Z_Construct_UFunction_USekiroHitboxDrawComponent_UpdateActiveEvents_Statics
{
	struct SekiroHitboxDrawComponent_eventUpdateActiveEvents_Parms
	{
		float CurrentFrame;
		TArray<FSekiroTaeEvent> AllEvents;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "Hitbox Visualization" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Filters AllEvents to those whose StartFrame <= CurrentFrame <= EndFrame\n\x09 * and whose Category equals \"Attack\", storing the result in ActiveEvents.\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/Visualization/SekiroHitboxDrawComponent.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Filters AllEvents to those whose StartFrame <= CurrentFrame <= EndFrame\nand whose Category equals \"Attack\", storing the result in ActiveEvents." },
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
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UFunction_USekiroHitboxDrawComponent_UpdateActiveEvents_Statics::NewProp_CurrentFrame = { "CurrentFrame", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(SekiroHitboxDrawComponent_eventUpdateActiveEvents_Parms, CurrentFrame), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UFunction_USekiroHitboxDrawComponent_UpdateActiveEvents_Statics::NewProp_AllEvents_Inner = { "AllEvents", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FSekiroTaeEvent, METADATA_PARAMS(0, nullptr) }; // 2260859055
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UFunction_USekiroHitboxDrawComponent_UpdateActiveEvents_Statics::NewProp_AllEvents = { "AllEvents", nullptr, (EPropertyFlags)0x0010000008000182, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(SekiroHitboxDrawComponent_eventUpdateActiveEvents_Parms, AllEvents), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_AllEvents_MetaData), NewProp_AllEvents_MetaData) }; // 2260859055
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_USekiroHitboxDrawComponent_UpdateActiveEvents_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_USekiroHitboxDrawComponent_UpdateActiveEvents_Statics::NewProp_CurrentFrame,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_USekiroHitboxDrawComponent_UpdateActiveEvents_Statics::NewProp_AllEvents_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_USekiroHitboxDrawComponent_UpdateActiveEvents_Statics::NewProp_AllEvents,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_USekiroHitboxDrawComponent_UpdateActiveEvents_Statics::PropPointers) < 2048);
// ********** End Function UpdateActiveEvents Property Definitions *********************************
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_USekiroHitboxDrawComponent_UpdateActiveEvents_Statics::FuncParams = { { (UObject*(*)())Z_Construct_UClass_USekiroHitboxDrawComponent, nullptr, "UpdateActiveEvents", 	Z_Construct_UFunction_USekiroHitboxDrawComponent_UpdateActiveEvents_Statics::PropPointers, 
	UE_ARRAY_COUNT(Z_Construct_UFunction_USekiroHitboxDrawComponent_UpdateActiveEvents_Statics::PropPointers), 
sizeof(Z_Construct_UFunction_USekiroHitboxDrawComponent_UpdateActiveEvents_Statics::SekiroHitboxDrawComponent_eventUpdateActiveEvents_Parms),
RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04420401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_USekiroHitboxDrawComponent_UpdateActiveEvents_Statics::Function_MetaDataParams), Z_Construct_UFunction_USekiroHitboxDrawComponent_UpdateActiveEvents_Statics::Function_MetaDataParams)},  };
static_assert(sizeof(Z_Construct_UFunction_USekiroHitboxDrawComponent_UpdateActiveEvents_Statics::SekiroHitboxDrawComponent_eventUpdateActiveEvents_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_USekiroHitboxDrawComponent_UpdateActiveEvents()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_USekiroHitboxDrawComponent_UpdateActiveEvents_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(USekiroHitboxDrawComponent::execUpdateActiveEvents)
{
	P_GET_PROPERTY(FFloatProperty,Z_Param_CurrentFrame);
	P_GET_TARRAY_REF(FSekiroTaeEvent,Z_Param_Out_AllEvents);
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->UpdateActiveEvents(Z_Param_CurrentFrame,Z_Param_Out_AllEvents);
	P_NATIVE_END;
}
// ********** End Class USekiroHitboxDrawComponent Function UpdateActiveEvents *********************

// ********** Begin Class USekiroHitboxDrawComponent ***********************************************
FClassRegistrationInfo Z_Registration_Info_UClass_USekiroHitboxDrawComponent;
UClass* USekiroHitboxDrawComponent::GetPrivateStaticClass()
{
	using TClass = USekiroHitboxDrawComponent;
	if (!Z_Registration_Info_UClass_USekiroHitboxDrawComponent.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("SekiroHitboxDrawComponent"),
			Z_Registration_Info_UClass_USekiroHitboxDrawComponent.InnerSingleton,
			StaticRegisterNativesUSekiroHitboxDrawComponent,
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
	return Z_Registration_Info_UClass_USekiroHitboxDrawComponent.InnerSingleton;
}
UClass* Z_Construct_UClass_USekiroHitboxDrawComponent_NoRegister()
{
	return USekiroHitboxDrawComponent::GetPrivateStaticClass();
}
struct Z_Construct_UClass_USekiroHitboxDrawComponent_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "BlueprintSpawnableComponent", "" },
		{ "ClassGroupNames", "Sekiro" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * Draws debug visualizations for attack hitbox TAE events.\n * Attach to ASekiroSkillPreviewActor to overlay red spheres/capsules\n * for every active attack event at the current frame.\n */" },
#endif
		{ "IncludePath", "Visualization/SekiroHitboxDrawComponent.h" },
		{ "ModuleRelativePath", "Public/Visualization/SekiroHitboxDrawComponent.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Draws debug visualizations for attack hitbox TAE events.\nAttach to ASekiroSkillPreviewActor to overlay red spheres/capsules\nfor every active attack event at the current frame." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ActiveEvents_MetaData[] = {
		{ "Category", "Hitbox Visualization" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** The set of TAE events currently active at the playback position. */" },
#endif
		{ "ModuleRelativePath", "Public/Visualization/SekiroHitboxDrawComponent.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "The set of TAE events currently active at the playback position." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_bShowHitboxes_MetaData[] = {
		{ "Category", "Hitbox Visualization" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Whether hitbox debug drawing is enabled. */" },
#endif
		{ "ModuleRelativePath", "Public/Visualization/SekiroHitboxDrawComponent.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Whether hitbox debug drawing is enabled." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_HitboxRadius_MetaData[] = {
		{ "Category", "Hitbox Visualization" },
		{ "ClampMin", "1.0" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Radius used for debug sphere/capsule drawing (in Unreal units). */" },
#endif
		{ "ModuleRelativePath", "Public/Visualization/SekiroHitboxDrawComponent.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Radius used for debug sphere/capsule drawing (in Unreal units)." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Class USekiroHitboxDrawComponent constinit property declarations ***************
	static const UECodeGen_Private::FStructPropertyParams NewProp_ActiveEvents_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_ActiveEvents;
	static void NewProp_bShowHitboxes_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_bShowHitboxes;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_HitboxRadius;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class USekiroHitboxDrawComponent constinit property declarations *****************
	static constexpr UE::CodeGen::FClassNativeFunction Funcs[] = {
		{ .NameUTF8 = UTF8TEXT("UpdateActiveEvents"), .Pointer = &USekiroHitboxDrawComponent::execUpdateActiveEvents },
	};
	static UObject* (*const DependentSingletons[])();
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_USekiroHitboxDrawComponent_UpdateActiveEvents, "UpdateActiveEvents" }, // 1236942976
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<USekiroHitboxDrawComponent>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_USekiroHitboxDrawComponent_Statics

// ********** Begin Class USekiroHitboxDrawComponent Property Definitions **************************
const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_USekiroHitboxDrawComponent_Statics::NewProp_ActiveEvents_Inner = { "ActiveEvents", nullptr, (EPropertyFlags)0x0000000000020000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FSekiroTaeEvent, METADATA_PARAMS(0, nullptr) }; // 2260859055
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_USekiroHitboxDrawComponent_Statics::NewProp_ActiveEvents = { "ActiveEvents", nullptr, (EPropertyFlags)0x0010000000022015, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(USekiroHitboxDrawComponent, ActiveEvents), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ActiveEvents_MetaData), NewProp_ActiveEvents_MetaData) }; // 2260859055
void Z_Construct_UClass_USekiroHitboxDrawComponent_Statics::NewProp_bShowHitboxes_SetBit(void* Obj)
{
	((USekiroHitboxDrawComponent*)Obj)->bShowHitboxes = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_USekiroHitboxDrawComponent_Statics::NewProp_bShowHitboxes = { "bShowHitboxes", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(USekiroHitboxDrawComponent), &Z_Construct_UClass_USekiroHitboxDrawComponent_Statics::NewProp_bShowHitboxes_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_bShowHitboxes_MetaData), NewProp_bShowHitboxes_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_USekiroHitboxDrawComponent_Statics::NewProp_HitboxRadius = { "HitboxRadius", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(USekiroHitboxDrawComponent, HitboxRadius), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_HitboxRadius_MetaData), NewProp_HitboxRadius_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_USekiroHitboxDrawComponent_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_USekiroHitboxDrawComponent_Statics::NewProp_ActiveEvents_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_USekiroHitboxDrawComponent_Statics::NewProp_ActiveEvents,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_USekiroHitboxDrawComponent_Statics::NewProp_bShowHitboxes,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_USekiroHitboxDrawComponent_Statics::NewProp_HitboxRadius,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_USekiroHitboxDrawComponent_Statics::PropPointers) < 2048);
// ********** End Class USekiroHitboxDrawComponent Property Definitions ****************************
UObject* (*const Z_Construct_UClass_USekiroHitboxDrawComponent_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UActorComponent,
	(UObject* (*)())Z_Construct_UPackage__Script_SekiroSkillEditorPlugin,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_USekiroHitboxDrawComponent_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_USekiroHitboxDrawComponent_Statics::ClassParams = {
	&USekiroHitboxDrawComponent::StaticClass,
	"Engine",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	FuncInfo,
	Z_Construct_UClass_USekiroHitboxDrawComponent_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	UE_ARRAY_COUNT(FuncInfo),
	UE_ARRAY_COUNT(Z_Construct_UClass_USekiroHitboxDrawComponent_Statics::PropPointers),
	0,
	0x00B000A4u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_USekiroHitboxDrawComponent_Statics::Class_MetaDataParams), Z_Construct_UClass_USekiroHitboxDrawComponent_Statics::Class_MetaDataParams)
};
void USekiroHitboxDrawComponent::StaticRegisterNativesUSekiroHitboxDrawComponent()
{
	UClass* Class = USekiroHitboxDrawComponent::StaticClass();
	FNativeFunctionRegistrar::RegisterFunctions(Class, MakeConstArrayView(Z_Construct_UClass_USekiroHitboxDrawComponent_Statics::Funcs));
}
UClass* Z_Construct_UClass_USekiroHitboxDrawComponent()
{
	if (!Z_Registration_Info_UClass_USekiroHitboxDrawComponent.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_USekiroHitboxDrawComponent.OuterSingleton, Z_Construct_UClass_USekiroHitboxDrawComponent_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_USekiroHitboxDrawComponent.OuterSingleton;
}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, USekiroHitboxDrawComponent);
USekiroHitboxDrawComponent::~USekiroHitboxDrawComponent() {}
// ********** End Class USekiroHitboxDrawComponent *************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Visualization_SekiroHitboxDrawComponent_h__Script_SekiroSkillEditorPlugin_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_USekiroHitboxDrawComponent, USekiroHitboxDrawComponent::StaticClass, TEXT("USekiroHitboxDrawComponent"), &Z_Registration_Info_UClass_USekiroHitboxDrawComponent, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(USekiroHitboxDrawComponent), 3465509022U) },
	};
}; // Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Visualization_SekiroHitboxDrawComponent_h__Script_SekiroSkillEditorPlugin_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Visualization_SekiroHitboxDrawComponent_h__Script_SekiroSkillEditorPlugin_1818852677{
	TEXT("/Script/SekiroSkillEditorPlugin"),
	Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Visualization_SekiroHitboxDrawComponent_h__Script_SekiroSkillEditorPlugin_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Visualization_SekiroHitboxDrawComponent_h__Script_SekiroSkillEditorPlugin_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
