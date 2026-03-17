// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Visualization/SekiroSkillPreviewActor.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeSekiroSkillPreviewActor() {}

// ********** Begin Cross Module References ********************************************************
ENGINE_API UClass* Z_Construct_UClass_AActor();
ENGINE_API UClass* Z_Construct_UClass_UAnimSequence_NoRegister();
ENGINE_API UClass* Z_Construct_UClass_USkeletalMeshComponent_NoRegister();
SEKIROSKILLEDITORPLUGIN_API UClass* Z_Construct_UClass_ASekiroSkillPreviewActor();
SEKIROSKILLEDITORPLUGIN_API UClass* Z_Construct_UClass_ASekiroSkillPreviewActor_NoRegister();
SEKIROSKILLEDITORPLUGIN_API UClass* Z_Construct_UClass_USekiroCharacterData_NoRegister();
UPackage* Z_Construct_UPackage__Script_SekiroSkillEditorPlugin();
// ********** End Cross Module References **********************************************************

// ********** Begin Class ASekiroSkillPreviewActor Function GetCurrentFrame ************************
struct Z_Construct_UFunction_ASekiroSkillPreviewActor_GetCurrentFrame_Statics
{
	struct SekiroSkillPreviewActor_eventGetCurrentFrame_Parms
	{
		float ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "Preview" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Returns the current frame position of animation playback. */" },
#endif
		{ "ModuleRelativePath", "Public/Visualization/SekiroSkillPreviewActor.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Returns the current frame position of animation playback." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Function GetCurrentFrame constinit property declarations ***********************
	static const UECodeGen_Private::FFloatPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function GetCurrentFrame constinit property declarations *************************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function GetCurrentFrame Property Definitions **********************************
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UFunction_ASekiroSkillPreviewActor_GetCurrentFrame_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(SekiroSkillPreviewActor_eventGetCurrentFrame_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_ASekiroSkillPreviewActor_GetCurrentFrame_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_ASekiroSkillPreviewActor_GetCurrentFrame_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_ASekiroSkillPreviewActor_GetCurrentFrame_Statics::PropPointers) < 2048);
// ********** End Function GetCurrentFrame Property Definitions ************************************
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_ASekiroSkillPreviewActor_GetCurrentFrame_Statics::FuncParams = { { (UObject*(*)())Z_Construct_UClass_ASekiroSkillPreviewActor, nullptr, "GetCurrentFrame", 	Z_Construct_UFunction_ASekiroSkillPreviewActor_GetCurrentFrame_Statics::PropPointers, 
	UE_ARRAY_COUNT(Z_Construct_UFunction_ASekiroSkillPreviewActor_GetCurrentFrame_Statics::PropPointers), 
sizeof(Z_Construct_UFunction_ASekiroSkillPreviewActor_GetCurrentFrame_Statics::SekiroSkillPreviewActor_eventGetCurrentFrame_Parms),
RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_ASekiroSkillPreviewActor_GetCurrentFrame_Statics::Function_MetaDataParams), Z_Construct_UFunction_ASekiroSkillPreviewActor_GetCurrentFrame_Statics::Function_MetaDataParams)},  };
static_assert(sizeof(Z_Construct_UFunction_ASekiroSkillPreviewActor_GetCurrentFrame_Statics::SekiroSkillPreviewActor_eventGetCurrentFrame_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_ASekiroSkillPreviewActor_GetCurrentFrame()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_ASekiroSkillPreviewActor_GetCurrentFrame_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(ASekiroSkillPreviewActor::execGetCurrentFrame)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(float*)Z_Param__Result=P_THIS->GetCurrentFrame();
	P_NATIVE_END;
}
// ********** End Class ASekiroSkillPreviewActor Function GetCurrentFrame **************************

// ********** Begin Class ASekiroSkillPreviewActor Function PlayAnimation **************************
struct Z_Construct_UFunction_ASekiroSkillPreviewActor_PlayAnimation_Statics
{
	struct SekiroSkillPreviewActor_eventPlayAnimation_Parms
	{
		UAnimSequence* InAnim;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "Preview" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Assigns a new animation sequence to the skeletal mesh component and\n\x09 * resets the playback position to frame 0.\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/Visualization/SekiroSkillPreviewActor.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Assigns a new animation sequence to the skeletal mesh component and\nresets the playback position to frame 0." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Function PlayAnimation constinit property declarations *************************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_InAnim;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function PlayAnimation constinit property declarations ***************************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function PlayAnimation Property Definitions ************************************
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_ASekiroSkillPreviewActor_PlayAnimation_Statics::NewProp_InAnim = { "InAnim", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(SekiroSkillPreviewActor_eventPlayAnimation_Parms, InAnim), Z_Construct_UClass_UAnimSequence_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_ASekiroSkillPreviewActor_PlayAnimation_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_ASekiroSkillPreviewActor_PlayAnimation_Statics::NewProp_InAnim,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_ASekiroSkillPreviewActor_PlayAnimation_Statics::PropPointers) < 2048);
// ********** End Function PlayAnimation Property Definitions **************************************
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_ASekiroSkillPreviewActor_PlayAnimation_Statics::FuncParams = { { (UObject*(*)())Z_Construct_UClass_ASekiroSkillPreviewActor, nullptr, "PlayAnimation", 	Z_Construct_UFunction_ASekiroSkillPreviewActor_PlayAnimation_Statics::PropPointers, 
	UE_ARRAY_COUNT(Z_Construct_UFunction_ASekiroSkillPreviewActor_PlayAnimation_Statics::PropPointers), 
sizeof(Z_Construct_UFunction_ASekiroSkillPreviewActor_PlayAnimation_Statics::SekiroSkillPreviewActor_eventPlayAnimation_Parms),
RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_ASekiroSkillPreviewActor_PlayAnimation_Statics::Function_MetaDataParams), Z_Construct_UFunction_ASekiroSkillPreviewActor_PlayAnimation_Statics::Function_MetaDataParams)},  };
static_assert(sizeof(Z_Construct_UFunction_ASekiroSkillPreviewActor_PlayAnimation_Statics::SekiroSkillPreviewActor_eventPlayAnimation_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_ASekiroSkillPreviewActor_PlayAnimation()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_ASekiroSkillPreviewActor_PlayAnimation_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(ASekiroSkillPreviewActor::execPlayAnimation)
{
	P_GET_OBJECT(UAnimSequence,Z_Param_InAnim);
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->PlayAnimation(Z_Param_InAnim);
	P_NATIVE_END;
}
// ********** End Class ASekiroSkillPreviewActor Function PlayAnimation ****************************

// ********** Begin Class ASekiroSkillPreviewActor Function SetCharacterData ***********************
struct Z_Construct_UFunction_ASekiroSkillPreviewActor_SetCharacterData_Statics
{
	struct SekiroSkillPreviewActor_eventSetCharacterData_Parms
	{
		USekiroCharacterData* InCharacterData;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "Preview" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Configures the preview mesh and materials from a character data asset.\n\x09 * Loads soft references synchronously so the mesh is available immediately.\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/Visualization/SekiroSkillPreviewActor.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Configures the preview mesh and materials from a character data asset.\nLoads soft references synchronously so the mesh is available immediately." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Function SetCharacterData constinit property declarations **********************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_InCharacterData;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function SetCharacterData constinit property declarations ************************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function SetCharacterData Property Definitions *********************************
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_ASekiroSkillPreviewActor_SetCharacterData_Statics::NewProp_InCharacterData = { "InCharacterData", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(SekiroSkillPreviewActor_eventSetCharacterData_Parms, InCharacterData), Z_Construct_UClass_USekiroCharacterData_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_ASekiroSkillPreviewActor_SetCharacterData_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_ASekiroSkillPreviewActor_SetCharacterData_Statics::NewProp_InCharacterData,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_ASekiroSkillPreviewActor_SetCharacterData_Statics::PropPointers) < 2048);
// ********** End Function SetCharacterData Property Definitions ***********************************
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_ASekiroSkillPreviewActor_SetCharacterData_Statics::FuncParams = { { (UObject*(*)())Z_Construct_UClass_ASekiroSkillPreviewActor, nullptr, "SetCharacterData", 	Z_Construct_UFunction_ASekiroSkillPreviewActor_SetCharacterData_Statics::PropPointers, 
	UE_ARRAY_COUNT(Z_Construct_UFunction_ASekiroSkillPreviewActor_SetCharacterData_Statics::PropPointers), 
sizeof(Z_Construct_UFunction_ASekiroSkillPreviewActor_SetCharacterData_Statics::SekiroSkillPreviewActor_eventSetCharacterData_Parms),
RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_ASekiroSkillPreviewActor_SetCharacterData_Statics::Function_MetaDataParams), Z_Construct_UFunction_ASekiroSkillPreviewActor_SetCharacterData_Statics::Function_MetaDataParams)},  };
static_assert(sizeof(Z_Construct_UFunction_ASekiroSkillPreviewActor_SetCharacterData_Statics::SekiroSkillPreviewActor_eventSetCharacterData_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_ASekiroSkillPreviewActor_SetCharacterData()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_ASekiroSkillPreviewActor_SetCharacterData_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(ASekiroSkillPreviewActor::execSetCharacterData)
{
	P_GET_OBJECT(USekiroCharacterData,Z_Param_InCharacterData);
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->SetCharacterData(Z_Param_InCharacterData);
	P_NATIVE_END;
}
// ********** End Class ASekiroSkillPreviewActor Function SetCharacterData *************************

// ********** Begin Class ASekiroSkillPreviewActor Function SetPlaybackPosition ********************
struct Z_Construct_UFunction_ASekiroSkillPreviewActor_SetPlaybackPosition_Statics
{
	struct SekiroSkillPreviewActor_eventSetPlaybackPosition_Parms
	{
		float Frame;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "Preview" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Manually sets the animation to a specific frame.\n\x09 * Pauses playback automatically.\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/Visualization/SekiroSkillPreviewActor.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Manually sets the animation to a specific frame.\nPauses playback automatically." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Function SetPlaybackPosition constinit property declarations *******************
	static const UECodeGen_Private::FFloatPropertyParams NewProp_Frame;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Function SetPlaybackPosition constinit property declarations *********************
	static const UECodeGen_Private::FFunctionParams FuncParams;
};

// ********** Begin Function SetPlaybackPosition Property Definitions ******************************
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UFunction_ASekiroSkillPreviewActor_SetPlaybackPosition_Statics::NewProp_Frame = { "Frame", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(SekiroSkillPreviewActor_eventSetPlaybackPosition_Parms, Frame), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_ASekiroSkillPreviewActor_SetPlaybackPosition_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_ASekiroSkillPreviewActor_SetPlaybackPosition_Statics::NewProp_Frame,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_ASekiroSkillPreviewActor_SetPlaybackPosition_Statics::PropPointers) < 2048);
// ********** End Function SetPlaybackPosition Property Definitions ********************************
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_ASekiroSkillPreviewActor_SetPlaybackPosition_Statics::FuncParams = { { (UObject*(*)())Z_Construct_UClass_ASekiroSkillPreviewActor, nullptr, "SetPlaybackPosition", 	Z_Construct_UFunction_ASekiroSkillPreviewActor_SetPlaybackPosition_Statics::PropPointers, 
	UE_ARRAY_COUNT(Z_Construct_UFunction_ASekiroSkillPreviewActor_SetPlaybackPosition_Statics::PropPointers), 
sizeof(Z_Construct_UFunction_ASekiroSkillPreviewActor_SetPlaybackPosition_Statics::SekiroSkillPreviewActor_eventSetPlaybackPosition_Parms),
RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_ASekiroSkillPreviewActor_SetPlaybackPosition_Statics::Function_MetaDataParams), Z_Construct_UFunction_ASekiroSkillPreviewActor_SetPlaybackPosition_Statics::Function_MetaDataParams)},  };
static_assert(sizeof(Z_Construct_UFunction_ASekiroSkillPreviewActor_SetPlaybackPosition_Statics::SekiroSkillPreviewActor_eventSetPlaybackPosition_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_ASekiroSkillPreviewActor_SetPlaybackPosition()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_ASekiroSkillPreviewActor_SetPlaybackPosition_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(ASekiroSkillPreviewActor::execSetPlaybackPosition)
{
	P_GET_PROPERTY(FFloatProperty,Z_Param_Frame);
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->SetPlaybackPosition(Z_Param_Frame);
	P_NATIVE_END;
}
// ********** End Class ASekiroSkillPreviewActor Function SetPlaybackPosition **********************

// ********** Begin Class ASekiroSkillPreviewActor *************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_ASekiroSkillPreviewActor;
UClass* ASekiroSkillPreviewActor::GetPrivateStaticClass()
{
	using TClass = ASekiroSkillPreviewActor;
	if (!Z_Registration_Info_UClass_ASekiroSkillPreviewActor.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("SekiroSkillPreviewActor"),
			Z_Registration_Info_UClass_ASekiroSkillPreviewActor.InnerSingleton,
			StaticRegisterNativesASekiroSkillPreviewActor,
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
	return Z_Registration_Info_UClass_ASekiroSkillPreviewActor.InnerSingleton;
}
UClass* Z_Construct_UClass_ASekiroSkillPreviewActor_NoRegister()
{
	return ASekiroSkillPreviewActor::GetPrivateStaticClass();
}
struct Z_Construct_UClass_ASekiroSkillPreviewActor_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * Preview actor that displays a Sekiro character's skeletal mesh\n * and drives animation playback for the skill editor viewport.\n */" },
#endif
		{ "IncludePath", "Visualization/SekiroSkillPreviewActor.h" },
		{ "ModuleRelativePath", "Public/Visualization/SekiroSkillPreviewActor.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Preview actor that displays a Sekiro character's skeletal mesh\nand drives animation playback for the skill editor viewport." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_MeshComp_MetaData[] = {
		{ "Category", "Preview" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Primary skeletal mesh component showing the character model. */" },
#endif
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "Public/Visualization/SekiroSkillPreviewActor.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Primary skeletal mesh component showing the character model." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_CurrentAnim_MetaData[] = {
		{ "Category", "Preview|Animation" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** The animation sequence currently assigned for preview playback. */" },
#endif
		{ "ModuleRelativePath", "Public/Visualization/SekiroSkillPreviewActor.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "The animation sequence currently assigned for preview playback." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_PlaybackRate_MetaData[] = {
		{ "Category", "Preview|Animation" },
		{ "ClampMax", "10.0" },
		{ "ClampMin", "0.0" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Playback speed multiplier. 1.0 = normal speed. */" },
#endif
		{ "ModuleRelativePath", "Public/Visualization/SekiroSkillPreviewActor.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Playback speed multiplier. 1.0 = normal speed." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_bIsPlaying_MetaData[] = {
		{ "Category", "Preview|Animation" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Whether the animation is currently advancing each tick. */" },
#endif
		{ "ModuleRelativePath", "Public/Visualization/SekiroSkillPreviewActor.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Whether the animation is currently advancing each tick." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_CurrentFrame_MetaData[] = {
		{ "Category", "Preview|Animation" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Current playback position expressed in frames. */" },
#endif
		{ "ModuleRelativePath", "Public/Visualization/SekiroSkillPreviewActor.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Current playback position expressed in frames." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Class ASekiroSkillPreviewActor constinit property declarations *****************
	static const UECodeGen_Private::FObjectPropertyParams NewProp_MeshComp;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_CurrentAnim;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_PlaybackRate;
	static void NewProp_bIsPlaying_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_bIsPlaying;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_CurrentFrame;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class ASekiroSkillPreviewActor constinit property declarations *******************
	static constexpr UE::CodeGen::FClassNativeFunction Funcs[] = {
		{ .NameUTF8 = UTF8TEXT("GetCurrentFrame"), .Pointer = &ASekiroSkillPreviewActor::execGetCurrentFrame },
		{ .NameUTF8 = UTF8TEXT("PlayAnimation"), .Pointer = &ASekiroSkillPreviewActor::execPlayAnimation },
		{ .NameUTF8 = UTF8TEXT("SetCharacterData"), .Pointer = &ASekiroSkillPreviewActor::execSetCharacterData },
		{ .NameUTF8 = UTF8TEXT("SetPlaybackPosition"), .Pointer = &ASekiroSkillPreviewActor::execSetPlaybackPosition },
	};
	static UObject* (*const DependentSingletons[])();
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_ASekiroSkillPreviewActor_GetCurrentFrame, "GetCurrentFrame" }, // 3230202767
		{ &Z_Construct_UFunction_ASekiroSkillPreviewActor_PlayAnimation, "PlayAnimation" }, // 2585071121
		{ &Z_Construct_UFunction_ASekiroSkillPreviewActor_SetCharacterData, "SetCharacterData" }, // 2366437762
		{ &Z_Construct_UFunction_ASekiroSkillPreviewActor_SetPlaybackPosition, "SetPlaybackPosition" }, // 3007592754
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<ASekiroSkillPreviewActor>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_ASekiroSkillPreviewActor_Statics

// ********** Begin Class ASekiroSkillPreviewActor Property Definitions ****************************
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_ASekiroSkillPreviewActor_Statics::NewProp_MeshComp = { "MeshComp", nullptr, (EPropertyFlags)0x01140000000a001d, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(ASekiroSkillPreviewActor, MeshComp), Z_Construct_UClass_USkeletalMeshComponent_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_MeshComp_MetaData), NewProp_MeshComp_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_ASekiroSkillPreviewActor_Statics::NewProp_CurrentAnim = { "CurrentAnim", nullptr, (EPropertyFlags)0x0114000000002014, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(ASekiroSkillPreviewActor, CurrentAnim), Z_Construct_UClass_UAnimSequence_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_CurrentAnim_MetaData), NewProp_CurrentAnim_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_ASekiroSkillPreviewActor_Statics::NewProp_PlaybackRate = { "PlaybackRate", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(ASekiroSkillPreviewActor, PlaybackRate), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_PlaybackRate_MetaData), NewProp_PlaybackRate_MetaData) };
void Z_Construct_UClass_ASekiroSkillPreviewActor_Statics::NewProp_bIsPlaying_SetBit(void* Obj)
{
	((ASekiroSkillPreviewActor*)Obj)->bIsPlaying = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_ASekiroSkillPreviewActor_Statics::NewProp_bIsPlaying = { "bIsPlaying", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(ASekiroSkillPreviewActor), &Z_Construct_UClass_ASekiroSkillPreviewActor_Statics::NewProp_bIsPlaying_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_bIsPlaying_MetaData), NewProp_bIsPlaying_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_ASekiroSkillPreviewActor_Statics::NewProp_CurrentFrame = { "CurrentFrame", nullptr, (EPropertyFlags)0x0010000000020015, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(ASekiroSkillPreviewActor, CurrentFrame), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_CurrentFrame_MetaData), NewProp_CurrentFrame_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_ASekiroSkillPreviewActor_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ASekiroSkillPreviewActor_Statics::NewProp_MeshComp,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ASekiroSkillPreviewActor_Statics::NewProp_CurrentAnim,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ASekiroSkillPreviewActor_Statics::NewProp_PlaybackRate,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ASekiroSkillPreviewActor_Statics::NewProp_bIsPlaying,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ASekiroSkillPreviewActor_Statics::NewProp_CurrentFrame,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_ASekiroSkillPreviewActor_Statics::PropPointers) < 2048);
// ********** End Class ASekiroSkillPreviewActor Property Definitions ******************************
UObject* (*const Z_Construct_UClass_ASekiroSkillPreviewActor_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_AActor,
	(UObject* (*)())Z_Construct_UPackage__Script_SekiroSkillEditorPlugin,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_ASekiroSkillPreviewActor_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_ASekiroSkillPreviewActor_Statics::ClassParams = {
	&ASekiroSkillPreviewActor::StaticClass,
	"Engine",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	FuncInfo,
	Z_Construct_UClass_ASekiroSkillPreviewActor_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	UE_ARRAY_COUNT(FuncInfo),
	UE_ARRAY_COUNT(Z_Construct_UClass_ASekiroSkillPreviewActor_Statics::PropPointers),
	0,
	0x009002A4u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_ASekiroSkillPreviewActor_Statics::Class_MetaDataParams), Z_Construct_UClass_ASekiroSkillPreviewActor_Statics::Class_MetaDataParams)
};
void ASekiroSkillPreviewActor::StaticRegisterNativesASekiroSkillPreviewActor()
{
	UClass* Class = ASekiroSkillPreviewActor::StaticClass();
	FNativeFunctionRegistrar::RegisterFunctions(Class, MakeConstArrayView(Z_Construct_UClass_ASekiroSkillPreviewActor_Statics::Funcs));
}
UClass* Z_Construct_UClass_ASekiroSkillPreviewActor()
{
	if (!Z_Registration_Info_UClass_ASekiroSkillPreviewActor.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_ASekiroSkillPreviewActor.OuterSingleton, Z_Construct_UClass_ASekiroSkillPreviewActor_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_ASekiroSkillPreviewActor.OuterSingleton;
}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, ASekiroSkillPreviewActor);
ASekiroSkillPreviewActor::~ASekiroSkillPreviewActor() {}
// ********** End Class ASekiroSkillPreviewActor ***************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Visualization_SekiroSkillPreviewActor_h__Script_SekiroSkillEditorPlugin_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_ASekiroSkillPreviewActor, ASekiroSkillPreviewActor::StaticClass, TEXT("ASekiroSkillPreviewActor"), &Z_Registration_Info_UClass_ASekiroSkillPreviewActor, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(ASekiroSkillPreviewActor), 716014054U) },
	};
}; // Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Visualization_SekiroSkillPreviewActor_h__Script_SekiroSkillEditorPlugin_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Visualization_SekiroSkillPreviewActor_h__Script_SekiroSkillEditorPlugin_1754395288{
	TEXT("/Script/SekiroSkillEditorPlugin"),
	Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Visualization_SekiroSkillPreviewActor_h__Script_SekiroSkillEditorPlugin_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_SekiroSkillEditor_Plugins_SekiroSkillEditorPlugin_Source_SekiroSkillEditorPlugin_Public_Visualization_SekiroSkillPreviewActor_h__Script_SekiroSkillEditorPlugin_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
