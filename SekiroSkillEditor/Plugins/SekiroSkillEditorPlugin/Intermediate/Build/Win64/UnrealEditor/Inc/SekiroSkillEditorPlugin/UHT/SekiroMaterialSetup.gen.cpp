// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Import/SekiroMaterialSetup.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeSekiroMaterialSetup() {}

// ********** Begin Cross Module References ********************************************************
COREUOBJECT_API UClass* Z_Construct_UClass_UObject();
ENGINE_API UClass* Z_Construct_UClass_UMaterialInstanceConstant_NoRegister();
SEKIROSKILLEDITORPLUGIN_API UClass* Z_Construct_UClass_USekiroMaterialSetup();
SEKIROSKILLEDITORPLUGIN_API UClass* Z_Construct_UClass_USekiroMaterialSetup_NoRegister();
UPackage* Z_Construct_UPackage__Script_SekiroSkillEditorPlugin();
// ********** End Cross Module References **********************************************************

// ********** Begin Class USekiroMaterialSetup Function SetupMaterialsFromManifest *****************
struct Z_Construct_UFunction_USekiroMaterialSetup_SetupMaterialsFromManifest_Statics
{
	struct SekiroMaterialSetup_eventSetupMaterialsFromManifest_Parms
	{
		FString ManifestJsonPath;
		FString TextureDirectory;
		FString OutputPackagePath;
		TArray<UMaterialInstanceConstant*> ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "Sekiro|Import" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Reads a material manifest JSON and creates material instances for each entry.\n\x09 *\n\x09 * @param ManifestJsonPath   Absolute path to material_manifest.json.\n\x09 * @param TextureDirectory   Absolute path to the directory containing referenced textures.\n\x09 * @param OutputPackagePath  Content browser path where material instances will be saved.\n\x09 * @return Array of newly created material instance constants.\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/Import/SekiroMaterialSetup.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Reads a material manifest JSON and creates material instances for each entry.\n\n@param ManifestJsonPath   Absolute path to material_manifest.json.\n@param TextureDirectory   Absolute path to the directory containing referenced textures.\n@param OutputPackagePath  Content browser path where material instances will be saved.\n@return Array of newly created material instance constants." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ManifestJsonPath_MetaData[] = {
		{ "NativeConst", "" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_TextureDirectory_MetaData[] = {
		{ "NativeConst", "" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_OutputPackagePath_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA

// ********** Begin Function SetupMaterialsFromManifest constinit property declarations ************
	static const UECodeGen_Private::FStrPropertyParams NewProp_ManifestJsonPath;
	static const UECodeGen_Private::FStrPropertyParams NewProp_TextureDirectory;
	static const UECodeGen_Private::FStrPropertyParams NewProp_OutputPackagePath;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_ReturnValue_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function SetupMaterialsFromManifest constinit property declarations **************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function SetupMaterialsFromManifest Property Definitions ***********************
const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_USekiroMaterialSetup_SetupMaterialsFromManifest_Statics::NewProp_ManifestJsonPath = { "ManifestJsonPath", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(SekiroMaterialSetup_eventSetupMaterialsFromManifest_Parms, ManifestJsonPath), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ManifestJsonPath_MetaData), NewProp_ManifestJsonPath_MetaData) };
const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_USekiroMaterialSetup_SetupMaterialsFromManifest_Statics::NewProp_TextureDirectory = { "TextureDirectory", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(SekiroMaterialSetup_eventSetupMaterialsFromManifest_Parms, TextureDirectory), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_TextureDirectory_MetaData), NewProp_TextureDirectory_MetaData) };
const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_USekiroMaterialSetup_SetupMaterialsFromManifest_Statics::NewProp_OutputPackagePath = { "OutputPackagePath", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(SekiroMaterialSetup_eventSetupMaterialsFromManifest_Parms, OutputPackagePath), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_OutputPackagePath_MetaData), NewProp_OutputPackagePath_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_USekiroMaterialSetup_SetupMaterialsFromManifest_Statics::NewProp_ReturnValue_Inner = { "ReturnValue", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UClass_UMaterialInstanceConstant_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UFunction_USekiroMaterialSetup_SetupMaterialsFromManifest_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(SekiroMaterialSetup_eventSetupMaterialsFromManifest_Parms, ReturnValue), EArrayPropertyFlags::None, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_USekiroMaterialSetup_SetupMaterialsFromManifest_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_USekiroMaterialSetup_SetupMaterialsFromManifest_Statics::NewProp_ManifestJsonPath,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_USekiroMaterialSetup_SetupMaterialsFromManifest_Statics::NewProp_TextureDirectory,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_USekiroMaterialSetup_SetupMaterialsFromManifest_Statics::NewProp_OutputPackagePath,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_USekiroMaterialSetup_SetupMaterialsFromManifest_Statics::NewProp_ReturnValue_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_USekiroMaterialSetup_SetupMaterialsFromManifest_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_USekiroMaterialSetup_SetupMaterialsFromManifest_Statics::PropPointers) < 2048);
// ********** End Function SetupMaterialsFromManifest Property Definitions *************************
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_USekiroMaterialSetup_SetupMaterialsFromManifest_Statics::FuncParams = { { (UObject*(*)())Z_Construct_UClass_USekiroMaterialSetup, nullptr, "SetupMaterialsFromManifest", 	Z_Construct_UFunction_USekiroMaterialSetup_SetupMaterialsFromManifest_Statics::PropPointers, 
	UE_ARRAY_COUNT(Z_Construct_UFunction_USekiroMaterialSetup_SetupMaterialsFromManifest_Statics::PropPointers), 
sizeof(Z_Construct_UFunction_USekiroMaterialSetup_SetupMaterialsFromManifest_Statics::SekiroMaterialSetup_eventSetupMaterialsFromManifest_Parms),
RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_USekiroMaterialSetup_SetupMaterialsFromManifest_Statics::Function_MetaDataParams), Z_Construct_UFunction_USekiroMaterialSetup_SetupMaterialsFromManifest_Statics::Function_MetaDataParams)},  };
static_assert(sizeof(Z_Construct_UFunction_USekiroMaterialSetup_SetupMaterialsFromManifest_Statics::SekiroMaterialSetup_eventSetupMaterialsFromManifest_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_USekiroMaterialSetup_SetupMaterialsFromManifest()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_USekiroMaterialSetup_SetupMaterialsFromManifest_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(USekiroMaterialSetup::execSetupMaterialsFromManifest)
{
	P_GET_PROPERTY(FStrProperty,Z_Param_ManifestJsonPath);
	P_GET_PROPERTY(FStrProperty,Z_Param_TextureDirectory);
	P_GET_PROPERTY(FStrProperty,Z_Param_OutputPackagePath);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(TArray<UMaterialInstanceConstant*>*)Z_Param__Result=USekiroMaterialSetup::SetupMaterialsFromManifest(Z_Param_ManifestJsonPath,Z_Param_TextureDirectory,Z_Param_OutputPackagePath);
	P_NATIVE_END;
}
// ********** End Class USekiroMaterialSetup Function SetupMaterialsFromManifest *******************

// ********** Begin Class USekiroMaterialSetup *****************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_USekiroMaterialSetup;
UClass* USekiroMaterialSetup::GetPrivateStaticClass()
{
	using TClass = USekiroMaterialSetup;
	if (!Z_Registration_Info_UClass_USekiroMaterialSetup.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("SekiroMaterialSetup"),
			Z_Registration_Info_UClass_USekiroMaterialSetup.InnerSingleton,
			StaticRegisterNativesUSekiroMaterialSetup,
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
	return Z_Registration_Info_UClass_USekiroMaterialSetup.InnerSingleton;
}
UClass* Z_Construct_UClass_USekiroMaterialSetup_NoRegister()
{
	return USekiroMaterialSetup::GetPrivateStaticClass();
}
struct Z_Construct_UClass_USekiroMaterialSetup_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * Utility class for creating UMaterialInstanceConstant assets from a\n * material manifest JSON exported by the Sekiro asset pipeline.\n * Maps texture slots (BaseColor, Normal, Specular, Emissive) to the\n * corresponding material parameter names.\n */" },
#endif
		{ "IncludePath", "Import/SekiroMaterialSetup.h" },
		{ "ModuleRelativePath", "Public/Import/SekiroMaterialSetup.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Utility class for creating UMaterialInstanceConstant assets from a\nmaterial manifest JSON exported by the Sekiro asset pipeline.\nMaps texture slots (BaseColor, Normal, Specular, Emissive) to the\ncorresponding material parameter names." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Class USekiroMaterialSetup constinit property declarations *********************
// ********** End Class USekiroMaterialSetup constinit property declarations ***********************
	static constexpr UE::CodeGen::FClassNativeFunction Funcs[] = {
		{ .NameUTF8 = UTF8TEXT("SetupMaterialsFromManifest"), .Pointer = &USekiroMaterialSetup::execSetupMaterialsFromManifest },
	};
	static UObject* (*const DependentSingletons[])();
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_USekiroMaterialSetup_SetupMaterialsFromManifest, "SetupMaterialsFromManifest" }, // 519528276
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<USekiroMaterialSetup>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_USekiroMaterialSetup_Statics
UObject* (*const Z_Construct_UClass_USekiroMaterialSetup_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UObject,
	(UObject* (*)())Z_Construct_UPackage__Script_SekiroSkillEditorPlugin,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_USekiroMaterialSetup_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_USekiroMaterialSetup_Statics::ClassParams = {
	&USekiroMaterialSetup::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	FuncInfo,
	nullptr,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	UE_ARRAY_COUNT(FuncInfo),
	0,
	0,
	0x001000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_USekiroMaterialSetup_Statics::Class_MetaDataParams), Z_Construct_UClass_USekiroMaterialSetup_Statics::Class_MetaDataParams)
};
void USekiroMaterialSetup::StaticRegisterNativesUSekiroMaterialSetup()
{
	UClass* Class = USekiroMaterialSetup::StaticClass();
	FNativeFunctionRegistrar::RegisterFunctions(Class, MakeConstArrayView(Z_Construct_UClass_USekiroMaterialSetup_Statics::Funcs));
}
UClass* Z_Construct_UClass_USekiroMaterialSetup()
{
	if (!Z_Registration_Info_UClass_USekiroMaterialSetup.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_USekiroMaterialSetup.OuterSingleton, Z_Construct_UClass_USekiroMaterialSetup_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_USekiroMaterialSetup.OuterSingleton;
}
USekiroMaterialSetup::USekiroMaterialSetup(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, USekiroMaterialSetup);
USekiroMaterialSetup::~USekiroMaterialSetup() {}
// ********** End Class USekiroMaterialSetup *******************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Import_SekiroMaterialSetup_h__Script_SekiroSkillEditorPlugin_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_USekiroMaterialSetup, USekiroMaterialSetup::StaticClass, TEXT("USekiroMaterialSetup"), &Z_Registration_Info_UClass_USekiroMaterialSetup, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(USekiroMaterialSetup), 3774053428U) },
	};
}; // Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Import_SekiroMaterialSetup_h__Script_SekiroSkillEditorPlugin_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Import_SekiroMaterialSetup_h__Script_SekiroSkillEditorPlugin_2110582415{
	TEXT("/Script/SekiroSkillEditorPlugin"),
	Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Import_SekiroMaterialSetup_h__Script_SekiroSkillEditorPlugin_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Import_SekiroMaterialSetup_h__Script_SekiroSkillEditorPlugin_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
