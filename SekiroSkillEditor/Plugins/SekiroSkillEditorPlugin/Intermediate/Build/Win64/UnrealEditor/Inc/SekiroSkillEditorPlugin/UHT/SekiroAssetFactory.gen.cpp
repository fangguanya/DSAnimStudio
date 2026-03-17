// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Import/SekiroAssetFactory.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeSekiroAssetFactory() {}

// ********** Begin Cross Module References ********************************************************
SEKIROSKILLEDITORPLUGIN_API UClass* Z_Construct_UClass_USekiroSkillConfigFactory();
SEKIROSKILLEDITORPLUGIN_API UClass* Z_Construct_UClass_USekiroSkillConfigFactory_NoRegister();
UNREALED_API UClass* Z_Construct_UClass_UFactory();
UPackage* Z_Construct_UPackage__Script_SekiroSkillEditorPlugin();
// ********** End Cross Module References **********************************************************

// ********** Begin Class USekiroSkillConfigFactory ************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_USekiroSkillConfigFactory;
UClass* USekiroSkillConfigFactory::GetPrivateStaticClass()
{
	using TClass = USekiroSkillConfigFactory;
	if (!Z_Registration_Info_UClass_USekiroSkillConfigFactory.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("SekiroSkillConfigFactory"),
			Z_Registration_Info_UClass_USekiroSkillConfigFactory.InnerSingleton,
			StaticRegisterNativesUSekiroSkillConfigFactory,
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
	return Z_Registration_Info_UClass_USekiroSkillConfigFactory.InnerSingleton;
}
UClass* Z_Construct_UClass_USekiroSkillConfigFactory_NoRegister()
{
	return USekiroSkillConfigFactory::GetPrivateStaticClass();
}
struct Z_Construct_UClass_USekiroSkillConfigFactory_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * UFactory implementation that allows .json skill config files to be\n * imported through the Content Browser or drag-and-drop into the editor.\n */" },
#endif
		{ "IncludePath", "Import/SekiroAssetFactory.h" },
		{ "ModuleRelativePath", "Public/Import/SekiroAssetFactory.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "UFactory implementation that allows .json skill config files to be\nimported through the Content Browser or drag-and-drop into the editor." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Class USekiroSkillConfigFactory constinit property declarations ****************
// ********** End Class USekiroSkillConfigFactory constinit property declarations ******************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<USekiroSkillConfigFactory>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_USekiroSkillConfigFactory_Statics
UObject* (*const Z_Construct_UClass_USekiroSkillConfigFactory_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UFactory,
	(UObject* (*)())Z_Construct_UPackage__Script_SekiroSkillEditorPlugin,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_USekiroSkillConfigFactory_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_USekiroSkillConfigFactory_Statics::ClassParams = {
	&USekiroSkillConfigFactory::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	nullptr,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	0,
	0,
	0x001000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_USekiroSkillConfigFactory_Statics::Class_MetaDataParams), Z_Construct_UClass_USekiroSkillConfigFactory_Statics::Class_MetaDataParams)
};
void USekiroSkillConfigFactory::StaticRegisterNativesUSekiroSkillConfigFactory()
{
}
UClass* Z_Construct_UClass_USekiroSkillConfigFactory()
{
	if (!Z_Registration_Info_UClass_USekiroSkillConfigFactory.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_USekiroSkillConfigFactory.OuterSingleton, Z_Construct_UClass_USekiroSkillConfigFactory_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_USekiroSkillConfigFactory.OuterSingleton;
}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, USekiroSkillConfigFactory);
USekiroSkillConfigFactory::~USekiroSkillConfigFactory() {}
// ********** End Class USekiroSkillConfigFactory **************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Import_SekiroAssetFactory_h__Script_SekiroSkillEditorPlugin_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_USekiroSkillConfigFactory, USekiroSkillConfigFactory::StaticClass, TEXT("USekiroSkillConfigFactory"), &Z_Registration_Info_UClass_USekiroSkillConfigFactory, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(USekiroSkillConfigFactory), 2768599536U) },
	};
}; // Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Import_SekiroAssetFactory_h__Script_SekiroSkillEditorPlugin_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Import_SekiroAssetFactory_h__Script_SekiroSkillEditorPlugin_451690298{
	TEXT("/Script/SekiroSkillEditorPlugin"),
	Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Import_SekiroAssetFactory_h__Script_SekiroSkillEditorPlugin_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Import_SekiroAssetFactory_h__Script_SekiroSkillEditorPlugin_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
