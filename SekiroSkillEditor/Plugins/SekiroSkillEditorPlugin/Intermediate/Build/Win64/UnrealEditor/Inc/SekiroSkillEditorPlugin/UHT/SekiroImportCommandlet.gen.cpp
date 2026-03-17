// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Import/SekiroImportCommandlet.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeSekiroImportCommandlet() {}

// ********** Begin Cross Module References ********************************************************
ENGINE_API UClass* Z_Construct_UClass_UCommandlet();
SEKIROSKILLEDITORPLUGIN_API UClass* Z_Construct_UClass_USekiroImportCommandlet();
SEKIROSKILLEDITORPLUGIN_API UClass* Z_Construct_UClass_USekiroImportCommandlet_NoRegister();
UPackage* Z_Construct_UPackage__Script_SekiroSkillEditorPlugin();
// ********** End Cross Module References **********************************************************

// ********** Begin Class USekiroImportCommandlet **************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_USekiroImportCommandlet;
UClass* USekiroImportCommandlet::GetPrivateStaticClass()
{
	using TClass = USekiroImportCommandlet;
	if (!Z_Registration_Info_UClass_USekiroImportCommandlet.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("SekiroImportCommandlet"),
			Z_Registration_Info_UClass_USekiroImportCommandlet.InnerSingleton,
			StaticRegisterNativesUSekiroImportCommandlet,
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
	return Z_Registration_Info_UClass_USekiroImportCommandlet.InnerSingleton;
}
UClass* Z_Construct_UClass_USekiroImportCommandlet_NoRegister()
{
	return USekiroImportCommandlet::GetPrivateStaticClass();
}
struct Z_Construct_UClass_USekiroImportCommandlet_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * Commandlet to batch import Sekiro exported assets (textures, models, animations).\n * Supports FBX, glTF (via Interchange), and Collada formats.\n * Usage: UnrealEditor-Cmd.exe project.uproject -run=SekiroImport -ExportDir=\"E:/Sekiro/Export\" [-ChrFilter=c1020,c1000]\n */" },
#endif
		{ "IncludePath", "Import/SekiroImportCommandlet.h" },
		{ "ModuleRelativePath", "Public/Import/SekiroImportCommandlet.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Commandlet to batch import Sekiro exported assets (textures, models, animations).\nSupports FBX, glTF (via Interchange), and Collada formats.\nUsage: UnrealEditor-Cmd.exe project.uproject -run=SekiroImport -ExportDir=\"E:/Sekiro/Export\" [-ChrFilter=c1020,c1000]" },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Class USekiroImportCommandlet constinit property declarations ******************
// ********** End Class USekiroImportCommandlet constinit property declarations ********************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<USekiroImportCommandlet>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_USekiroImportCommandlet_Statics
UObject* (*const Z_Construct_UClass_USekiroImportCommandlet_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UCommandlet,
	(UObject* (*)())Z_Construct_UPackage__Script_SekiroSkillEditorPlugin,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_USekiroImportCommandlet_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_USekiroImportCommandlet_Statics::ClassParams = {
	&USekiroImportCommandlet::StaticClass,
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
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_USekiroImportCommandlet_Statics::Class_MetaDataParams), Z_Construct_UClass_USekiroImportCommandlet_Statics::Class_MetaDataParams)
};
void USekiroImportCommandlet::StaticRegisterNativesUSekiroImportCommandlet()
{
}
UClass* Z_Construct_UClass_USekiroImportCommandlet()
{
	if (!Z_Registration_Info_UClass_USekiroImportCommandlet.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_USekiroImportCommandlet.OuterSingleton, Z_Construct_UClass_USekiroImportCommandlet_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_USekiroImportCommandlet.OuterSingleton;
}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, USekiroImportCommandlet);
USekiroImportCommandlet::~USekiroImportCommandlet() {}
// ********** End Class USekiroImportCommandlet ****************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Import_SekiroImportCommandlet_h__Script_SekiroSkillEditorPlugin_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_USekiroImportCommandlet, USekiroImportCommandlet::StaticClass, TEXT("USekiroImportCommandlet"), &Z_Registration_Info_UClass_USekiroImportCommandlet, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(USekiroImportCommandlet), 453768740U) },
	};
}; // Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Import_SekiroImportCommandlet_h__Script_SekiroSkillEditorPlugin_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Import_SekiroImportCommandlet_h__Script_SekiroSkillEditorPlugin_3921085495{
	TEXT("/Script/SekiroSkillEditorPlugin"),
	Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Import_SekiroImportCommandlet_h__Script_SekiroSkillEditorPlugin_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Import_SekiroImportCommandlet_h__Script_SekiroSkillEditorPlugin_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
