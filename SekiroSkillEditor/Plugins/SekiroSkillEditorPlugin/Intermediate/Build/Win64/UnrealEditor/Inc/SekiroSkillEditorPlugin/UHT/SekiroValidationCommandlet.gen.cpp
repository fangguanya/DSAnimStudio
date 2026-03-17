// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Import/SekiroValidationCommandlet.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeSekiroValidationCommandlet() {}

// ********** Begin Cross Module References ********************************************************
ENGINE_API UClass* Z_Construct_UClass_UCommandlet();
SEKIROSKILLEDITORPLUGIN_API UClass* Z_Construct_UClass_USekiroValidationCommandlet();
SEKIROSKILLEDITORPLUGIN_API UClass* Z_Construct_UClass_USekiroValidationCommandlet_NoRegister();
UPackage* Z_Construct_UPackage__Script_SekiroSkillEditorPlugin();
// ********** End Cross Module References **********************************************************

// ********** Begin Class USekiroValidationCommandlet **********************************************
FClassRegistrationInfo Z_Registration_Info_UClass_USekiroValidationCommandlet;
UClass* USekiroValidationCommandlet::GetPrivateStaticClass()
{
	using TClass = USekiroValidationCommandlet;
	if (!Z_Registration_Info_UClass_USekiroValidationCommandlet.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("SekiroValidationCommandlet"),
			Z_Registration_Info_UClass_USekiroValidationCommandlet.InnerSingleton,
			StaticRegisterNativesUSekiroValidationCommandlet,
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
	return Z_Registration_Info_UClass_USekiroValidationCommandlet.InnerSingleton;
}
UClass* Z_Construct_UClass_USekiroValidationCommandlet_NoRegister()
{
	return USekiroValidationCommandlet::GetPrivateStaticClass();
}
struct Z_Construct_UClass_USekiroValidationCommandlet_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * Commandlet to validate all imported Sekiro assets.\n * Checks SkeletalMesh, Skeleton, AnimSequence, Textures, Materials, and Skill Configs.\n * Usage: UnrealEditor-Cmd.exe project.uproject -run=SekiroValidation -ExportDir=\"E:/Sekiro/Export\" [-ChrFilter=c1010]\n */" },
#endif
		{ "IncludePath", "Import/SekiroValidationCommandlet.h" },
		{ "ModuleRelativePath", "Public/Import/SekiroValidationCommandlet.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Commandlet to validate all imported Sekiro assets.\nChecks SkeletalMesh, Skeleton, AnimSequence, Textures, Materials, and Skill Configs.\nUsage: UnrealEditor-Cmd.exe project.uproject -run=SekiroValidation -ExportDir=\"E:/Sekiro/Export\" [-ChrFilter=c1010]" },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Class USekiroValidationCommandlet constinit property declarations **************
// ********** End Class USekiroValidationCommandlet constinit property declarations ****************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<USekiroValidationCommandlet>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_USekiroValidationCommandlet_Statics
UObject* (*const Z_Construct_UClass_USekiroValidationCommandlet_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UCommandlet,
	(UObject* (*)())Z_Construct_UPackage__Script_SekiroSkillEditorPlugin,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_USekiroValidationCommandlet_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_USekiroValidationCommandlet_Statics::ClassParams = {
	&USekiroValidationCommandlet::StaticClass,
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
	0x000000A8u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_USekiroValidationCommandlet_Statics::Class_MetaDataParams), Z_Construct_UClass_USekiroValidationCommandlet_Statics::Class_MetaDataParams)
};
void USekiroValidationCommandlet::StaticRegisterNativesUSekiroValidationCommandlet()
{
}
UClass* Z_Construct_UClass_USekiroValidationCommandlet()
{
	if (!Z_Registration_Info_UClass_USekiroValidationCommandlet.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_USekiroValidationCommandlet.OuterSingleton, Z_Construct_UClass_USekiroValidationCommandlet_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_USekiroValidationCommandlet.OuterSingleton;
}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, USekiroValidationCommandlet);
USekiroValidationCommandlet::~USekiroValidationCommandlet() {}
// ********** End Class USekiroValidationCommandlet ************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Import_SekiroValidationCommandlet_h__Script_SekiroSkillEditorPlugin_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_USekiroValidationCommandlet, USekiroValidationCommandlet::StaticClass, TEXT("USekiroValidationCommandlet"), &Z_Registration_Info_UClass_USekiroValidationCommandlet, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(USekiroValidationCommandlet), 3331438674U) },
	};
}; // Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Import_SekiroValidationCommandlet_h__Script_SekiroSkillEditorPlugin_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Import_SekiroValidationCommandlet_h__Script_SekiroSkillEditorPlugin_4214046004{
	TEXT("/Script/SekiroSkillEditorPlugin"),
	Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Import_SekiroValidationCommandlet_h__Script_SekiroSkillEditorPlugin_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Import_SekiroValidationCommandlet_h__Script_SekiroSkillEditorPlugin_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
