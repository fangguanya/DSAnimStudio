// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Import/SekiroAssetImporter.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeSekiroAssetImporter() {}

// ********** Begin Cross Module References ********************************************************
COREUOBJECT_API UClass* Z_Construct_UClass_UObject();
SEKIROSKILLEDITORPLUGIN_API UClass* Z_Construct_UClass_USekiroAssetImporter();
SEKIROSKILLEDITORPLUGIN_API UClass* Z_Construct_UClass_USekiroAssetImporter_NoRegister();
SEKIROSKILLEDITORPLUGIN_API UClass* Z_Construct_UClass_USekiroSkillDataAsset_NoRegister();
UPackage* Z_Construct_UPackage__Script_SekiroSkillEditorPlugin();
// ********** End Cross Module References **********************************************************

// ********** Begin Class USekiroAssetImporter Function ImportSkillConfig **************************
struct Z_Construct_UFunction_USekiroAssetImporter_ImportSkillConfig_Statics
{
	struct SekiroAssetImporter_eventImportSkillConfig_Parms
	{
		FString JsonFilePath;
		FString OutputPackagePath;
		TArray<USekiroSkillDataAsset*> ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "Sekiro|Import" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Imports a skill configuration JSON file and creates data assets for each animation entry.\n\x09 *\n\x09 * @param JsonFilePath        Absolute path to the JSON configuration file.\n\x09 * @param OutputPackagePath   Content browser path where assets will be created (e.g. \"/Game/Sekiro/Skills\").\n\x09 * @return Array of newly created skill data assets. Empty if the import fails.\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/Import/SekiroAssetImporter.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Imports a skill configuration JSON file and creates data assets for each animation entry.\n\n@param JsonFilePath        Absolute path to the JSON configuration file.\n@param OutputPackagePath   Content browser path where assets will be created (e.g. \"/Game/Sekiro/Skills\").\n@return Array of newly created skill data assets. Empty if the import fails." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_JsonFilePath_MetaData[] = {
		{ "NativeConst", "" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_OutputPackagePath_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA

// ********** Begin Function ImportSkillConfig constinit property declarations *********************
	static const UECodeGen_Private::FStrPropertyParams NewProp_JsonFilePath;
	static const UECodeGen_Private::FStrPropertyParams NewProp_OutputPackagePath;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_ReturnValue_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function ImportSkillConfig constinit property declarations ***********************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function ImportSkillConfig Property Definitions ********************************
const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_USekiroAssetImporter_ImportSkillConfig_Statics::NewProp_JsonFilePath = { "JsonFilePath", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(SekiroAssetImporter_eventImportSkillConfig_Parms, JsonFilePath), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_JsonFilePath_MetaData), NewProp_JsonFilePath_MetaData) };
const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_USekiroAssetImporter_ImportSkillConfig_Statics::NewProp_OutputPackagePath = { "OutputPackagePath", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(SekiroAssetImporter_eventImportSkillConfig_Parms, OutputPackagePath), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_OutputPackagePath_MetaData), NewProp_OutputPackagePath_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_USekiroAssetImporter_ImportSkillConfig_Statics::NewProp_ReturnValue_Inner = { "ReturnValue", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UClass_USekiroSkillDataAsset_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UFunction_USekiroAssetImporter_ImportSkillConfig_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(SekiroAssetImporter_eventImportSkillConfig_Parms, ReturnValue), EArrayPropertyFlags::None, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_USekiroAssetImporter_ImportSkillConfig_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_USekiroAssetImporter_ImportSkillConfig_Statics::NewProp_JsonFilePath,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_USekiroAssetImporter_ImportSkillConfig_Statics::NewProp_OutputPackagePath,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_USekiroAssetImporter_ImportSkillConfig_Statics::NewProp_ReturnValue_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_USekiroAssetImporter_ImportSkillConfig_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_USekiroAssetImporter_ImportSkillConfig_Statics::PropPointers) < 2048);
// ********** End Function ImportSkillConfig Property Definitions **********************************
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_USekiroAssetImporter_ImportSkillConfig_Statics::FuncParams = { { (UObject*(*)())Z_Construct_UClass_USekiroAssetImporter, nullptr, "ImportSkillConfig", 	Z_Construct_UFunction_USekiroAssetImporter_ImportSkillConfig_Statics::PropPointers, 
	UE_ARRAY_COUNT(Z_Construct_UFunction_USekiroAssetImporter_ImportSkillConfig_Statics::PropPointers), 
sizeof(Z_Construct_UFunction_USekiroAssetImporter_ImportSkillConfig_Statics::SekiroAssetImporter_eventImportSkillConfig_Parms),
RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_USekiroAssetImporter_ImportSkillConfig_Statics::Function_MetaDataParams), Z_Construct_UFunction_USekiroAssetImporter_ImportSkillConfig_Statics::Function_MetaDataParams)},  };
static_assert(sizeof(Z_Construct_UFunction_USekiroAssetImporter_ImportSkillConfig_Statics::SekiroAssetImporter_eventImportSkillConfig_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_USekiroAssetImporter_ImportSkillConfig()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_USekiroAssetImporter_ImportSkillConfig_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(USekiroAssetImporter::execImportSkillConfig)
{
	P_GET_PROPERTY(FStrProperty,Z_Param_JsonFilePath);
	P_GET_PROPERTY(FStrProperty,Z_Param_OutputPackagePath);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(TArray<USekiroSkillDataAsset*>*)Z_Param__Result=USekiroAssetImporter::ImportSkillConfig(Z_Param_JsonFilePath,Z_Param_OutputPackagePath);
	P_NATIVE_END;
}
// ********** End Class USekiroAssetImporter Function ImportSkillConfig ****************************

// ********** Begin Class USekiroAssetImporter *****************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_USekiroAssetImporter;
UClass* USekiroAssetImporter::GetPrivateStaticClass()
{
	using TClass = USekiroAssetImporter;
	if (!Z_Registration_Info_UClass_USekiroAssetImporter.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("SekiroAssetImporter"),
			Z_Registration_Info_UClass_USekiroAssetImporter.InnerSingleton,
			StaticRegisterNativesUSekiroAssetImporter,
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
	return Z_Registration_Info_UClass_USekiroAssetImporter.InnerSingleton;
}
UClass* Z_Construct_UClass_USekiroAssetImporter_NoRegister()
{
	return USekiroAssetImporter::GetPrivateStaticClass();
}
struct Z_Construct_UClass_USekiroAssetImporter_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * Handles importing Sekiro skill configuration data from JSON files\n * exported by DSAnimStudio. Parses character animation data and TAE events\n * into USekiroSkillDataAsset instances.\n */" },
#endif
		{ "IncludePath", "Import/SekiroAssetImporter.h" },
		{ "ModuleRelativePath", "Public/Import/SekiroAssetImporter.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Handles importing Sekiro skill configuration data from JSON files\nexported by DSAnimStudio. Parses character animation data and TAE events\ninto USekiroSkillDataAsset instances." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Class USekiroAssetImporter constinit property declarations *********************
// ********** End Class USekiroAssetImporter constinit property declarations ***********************
	static constexpr UE::CodeGen::FClassNativeFunction Funcs[] = {
		{ .NameUTF8 = UTF8TEXT("ImportSkillConfig"), .Pointer = &USekiroAssetImporter::execImportSkillConfig },
	};
	static UObject* (*const DependentSingletons[])();
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_USekiroAssetImporter_ImportSkillConfig, "ImportSkillConfig" }, // 1464023183
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<USekiroAssetImporter>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_USekiroAssetImporter_Statics
UObject* (*const Z_Construct_UClass_USekiroAssetImporter_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UObject,
	(UObject* (*)())Z_Construct_UPackage__Script_SekiroSkillEditorPlugin,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_USekiroAssetImporter_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_USekiroAssetImporter_Statics::ClassParams = {
	&USekiroAssetImporter::StaticClass,
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
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_USekiroAssetImporter_Statics::Class_MetaDataParams), Z_Construct_UClass_USekiroAssetImporter_Statics::Class_MetaDataParams)
};
void USekiroAssetImporter::StaticRegisterNativesUSekiroAssetImporter()
{
	UClass* Class = USekiroAssetImporter::StaticClass();
	FNativeFunctionRegistrar::RegisterFunctions(Class, MakeConstArrayView(Z_Construct_UClass_USekiroAssetImporter_Statics::Funcs));
}
UClass* Z_Construct_UClass_USekiroAssetImporter()
{
	if (!Z_Registration_Info_UClass_USekiroAssetImporter.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_USekiroAssetImporter.OuterSingleton, Z_Construct_UClass_USekiroAssetImporter_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_USekiroAssetImporter.OuterSingleton;
}
USekiroAssetImporter::USekiroAssetImporter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, USekiroAssetImporter);
USekiroAssetImporter::~USekiroAssetImporter() {}
// ********** End Class USekiroAssetImporter *******************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Import_SekiroAssetImporter_h__Script_SekiroSkillEditorPlugin_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_USekiroAssetImporter, USekiroAssetImporter::StaticClass, TEXT("USekiroAssetImporter"), &Z_Registration_Info_UClass_USekiroAssetImporter, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(USekiroAssetImporter), 504034206U) },
	};
}; // Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Import_SekiroAssetImporter_h__Script_SekiroSkillEditorPlugin_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Import_SekiroAssetImporter_h__Script_SekiroSkillEditorPlugin_3624625032{
	TEXT("/Script/SekiroSkillEditorPlugin"),
	Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Import_SekiroAssetImporter_h__Script_SekiroSkillEditorPlugin_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Import_SekiroAssetImporter_h__Script_SekiroSkillEditorPlugin_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
