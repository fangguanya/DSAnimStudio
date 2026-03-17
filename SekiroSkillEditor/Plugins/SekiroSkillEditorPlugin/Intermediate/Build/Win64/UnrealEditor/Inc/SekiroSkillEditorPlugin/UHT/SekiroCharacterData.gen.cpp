// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Data/SekiroCharacterData.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeSekiroCharacterData() {}

// ********** Begin Cross Module References ********************************************************
ENGINE_API UClass* Z_Construct_UClass_UDataAsset();
ENGINE_API UClass* Z_Construct_UClass_UMaterialInterface_NoRegister();
ENGINE_API UClass* Z_Construct_UClass_USkeletalMesh_NoRegister();
ENGINE_API UClass* Z_Construct_UClass_USkeleton_NoRegister();
SEKIROSKILLEDITORPLUGIN_API UClass* Z_Construct_UClass_USekiroCharacterData();
SEKIROSKILLEDITORPLUGIN_API UClass* Z_Construct_UClass_USekiroCharacterData_NoRegister();
SEKIROSKILLEDITORPLUGIN_API UClass* Z_Construct_UClass_USekiroSkillDataAsset_NoRegister();
UPackage* Z_Construct_UPackage__Script_SekiroSkillEditorPlugin();
// ********** End Cross Module References **********************************************************

// ********** Begin Class USekiroCharacterData *****************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_USekiroCharacterData;
UClass* USekiroCharacterData::GetPrivateStaticClass()
{
	using TClass = USekiroCharacterData;
	if (!Z_Registration_Info_UClass_USekiroCharacterData.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("SekiroCharacterData"),
			Z_Registration_Info_UClass_USekiroCharacterData.InnerSingleton,
			StaticRegisterNativesUSekiroCharacterData,
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
	return Z_Registration_Info_UClass_USekiroCharacterData.InnerSingleton;
}
UClass* Z_Construct_UClass_USekiroCharacterData_NoRegister()
{
	return USekiroCharacterData::GetPrivateStaticClass();
}
struct Z_Construct_UClass_USekiroCharacterData_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * Data asset holding character-level data: mesh, skeleton, materials,\n * and references to all associated skill data assets.\n */" },
#endif
		{ "IncludePath", "Data/SekiroCharacterData.h" },
		{ "ModuleRelativePath", "Public/Data/SekiroCharacterData.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Data asset holding character-level data: mesh, skeleton, materials,\nand references to all associated skill data assets." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_CharacterId_MetaData[] = {
		{ "Category", "Character Data" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Unique identifier for this character (e.g. \"c0000\" for the player). */" },
#endif
		{ "ModuleRelativePath", "Public/Data/SekiroCharacterData.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Unique identifier for this character (e.g. \"c0000\" for the player)." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Mesh_MetaData[] = {
		{ "Category", "Character Data" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Soft reference to the character's skeletal mesh. */" },
#endif
		{ "ModuleRelativePath", "Public/Data/SekiroCharacterData.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Soft reference to the character's skeletal mesh." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Skeleton_MetaData[] = {
		{ "Category", "Character Data" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Soft reference to the character's skeleton asset. */" },
#endif
		{ "ModuleRelativePath", "Public/Data/SekiroCharacterData.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Soft reference to the character's skeleton asset." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_MaterialMap_MetaData[] = {
		{ "Category", "Character Data" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Named material slots mapped to soft material references. */" },
#endif
		{ "ModuleRelativePath", "Public/Data/SekiroCharacterData.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Named material slots mapped to soft material references." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Skills_MetaData[] = {
		{ "Category", "Character Data" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Soft references to all skill data assets for this character. */" },
#endif
		{ "ModuleRelativePath", "Public/Data/SekiroCharacterData.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Soft references to all skill data assets for this character." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Class USekiroCharacterData constinit property declarations *********************
	static const UECodeGen_Private::FStrPropertyParams NewProp_CharacterId;
	static const UECodeGen_Private::FSoftObjectPropertyParams NewProp_Mesh;
	static const UECodeGen_Private::FSoftObjectPropertyParams NewProp_Skeleton;
	static const UECodeGen_Private::FSoftObjectPropertyParams NewProp_MaterialMap_ValueProp;
	static const UECodeGen_Private::FStrPropertyParams NewProp_MaterialMap_Key_KeyProp;
	static const UECodeGen_Private::FMapPropertyParams NewProp_MaterialMap;
	static const UECodeGen_Private::FSoftObjectPropertyParams NewProp_Skills_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_Skills;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class USekiroCharacterData constinit property declarations ***********************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<USekiroCharacterData>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_USekiroCharacterData_Statics

// ********** Begin Class USekiroCharacterData Property Definitions ********************************
const UECodeGen_Private::FStrPropertyParams Z_Construct_UClass_USekiroCharacterData_Statics::NewProp_CharacterId = { "CharacterId", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(USekiroCharacterData, CharacterId), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_CharacterId_MetaData), NewProp_CharacterId_MetaData) };
const UECodeGen_Private::FSoftObjectPropertyParams Z_Construct_UClass_USekiroCharacterData_Statics::NewProp_Mesh = { "Mesh", nullptr, (EPropertyFlags)0x0014000000000005, UECodeGen_Private::EPropertyGenFlags::SoftObject, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(USekiroCharacterData, Mesh), Z_Construct_UClass_USkeletalMesh_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Mesh_MetaData), NewProp_Mesh_MetaData) };
const UECodeGen_Private::FSoftObjectPropertyParams Z_Construct_UClass_USekiroCharacterData_Statics::NewProp_Skeleton = { "Skeleton", nullptr, (EPropertyFlags)0x0014000000000005, UECodeGen_Private::EPropertyGenFlags::SoftObject, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(USekiroCharacterData, Skeleton), Z_Construct_UClass_USkeleton_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Skeleton_MetaData), NewProp_Skeleton_MetaData) };
const UECodeGen_Private::FSoftObjectPropertyParams Z_Construct_UClass_USekiroCharacterData_Statics::NewProp_MaterialMap_ValueProp = { "MaterialMap", nullptr, (EPropertyFlags)0x0004000000000001, UECodeGen_Private::EPropertyGenFlags::SoftObject, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 1, Z_Construct_UClass_UMaterialInterface_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams Z_Construct_UClass_USekiroCharacterData_Statics::NewProp_MaterialMap_Key_KeyProp = { "MaterialMap_Key", nullptr, (EPropertyFlags)0x0000000000000001, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FMapPropertyParams Z_Construct_UClass_USekiroCharacterData_Statics::NewProp_MaterialMap = { "MaterialMap", nullptr, (EPropertyFlags)0x0014000000000005, UECodeGen_Private::EPropertyGenFlags::Map, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(USekiroCharacterData, MaterialMap), EMapPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_MaterialMap_MetaData), NewProp_MaterialMap_MetaData) };
const UECodeGen_Private::FSoftObjectPropertyParams Z_Construct_UClass_USekiroCharacterData_Statics::NewProp_Skills_Inner = { "Skills", nullptr, (EPropertyFlags)0x0004000000000000, UECodeGen_Private::EPropertyGenFlags::SoftObject, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UClass_USekiroSkillDataAsset_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_USekiroCharacterData_Statics::NewProp_Skills = { "Skills", nullptr, (EPropertyFlags)0x0014000000000005, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(USekiroCharacterData, Skills), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Skills_MetaData), NewProp_Skills_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_USekiroCharacterData_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_USekiroCharacterData_Statics::NewProp_CharacterId,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_USekiroCharacterData_Statics::NewProp_Mesh,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_USekiroCharacterData_Statics::NewProp_Skeleton,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_USekiroCharacterData_Statics::NewProp_MaterialMap_ValueProp,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_USekiroCharacterData_Statics::NewProp_MaterialMap_Key_KeyProp,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_USekiroCharacterData_Statics::NewProp_MaterialMap,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_USekiroCharacterData_Statics::NewProp_Skills_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_USekiroCharacterData_Statics::NewProp_Skills,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_USekiroCharacterData_Statics::PropPointers) < 2048);
// ********** End Class USekiroCharacterData Property Definitions **********************************
UObject* (*const Z_Construct_UClass_USekiroCharacterData_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UDataAsset,
	(UObject* (*)())Z_Construct_UPackage__Script_SekiroSkillEditorPlugin,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_USekiroCharacterData_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_USekiroCharacterData_Statics::ClassParams = {
	&USekiroCharacterData::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_USekiroCharacterData_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_USekiroCharacterData_Statics::PropPointers),
	0,
	0x001000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_USekiroCharacterData_Statics::Class_MetaDataParams), Z_Construct_UClass_USekiroCharacterData_Statics::Class_MetaDataParams)
};
void USekiroCharacterData::StaticRegisterNativesUSekiroCharacterData()
{
}
UClass* Z_Construct_UClass_USekiroCharacterData()
{
	if (!Z_Registration_Info_UClass_USekiroCharacterData.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_USekiroCharacterData.OuterSingleton, Z_Construct_UClass_USekiroCharacterData_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_USekiroCharacterData.OuterSingleton;
}
USekiroCharacterData::USekiroCharacterData(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, USekiroCharacterData);
USekiroCharacterData::~USekiroCharacterData() {}
// ********** End Class USekiroCharacterData *******************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Data_SekiroCharacterData_h__Script_SekiroSkillEditorPlugin_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_USekiroCharacterData, USekiroCharacterData::StaticClass, TEXT("USekiroCharacterData"), &Z_Registration_Info_UClass_USekiroCharacterData, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(USekiroCharacterData), 64763493U) },
	};
}; // Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Data_SekiroCharacterData_h__Script_SekiroSkillEditorPlugin_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Data_SekiroCharacterData_h__Script_SekiroSkillEditorPlugin_1187011782{
	TEXT("/Script/SekiroSkillEditorPlugin"),
	Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Data_SekiroCharacterData_h__Script_SekiroSkillEditorPlugin_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Data_SekiroCharacterData_h__Script_SekiroSkillEditorPlugin_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
