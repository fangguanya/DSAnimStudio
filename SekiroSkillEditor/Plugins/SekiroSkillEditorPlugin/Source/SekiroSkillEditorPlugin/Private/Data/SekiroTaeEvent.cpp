// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

#include "Data/SekiroTaeEvent.h"

FLinearColor FSekiroTaeEvent::GetCategoryColor(const FString& InCategory)
{
	if (InCategory == TEXT("Attack"))
	{
		return FLinearColor::Red;
	}
	if (InCategory == TEXT("Effect"))
	{
		return FLinearColor(0.2f, 0.4f, 1.0f, 1.0f);
	}
	if (InCategory == TEXT("Sound"))
	{
		return FLinearColor(0.2f, 0.8f, 0.2f, 1.0f);
	}
	if (InCategory == TEXT("Movement"))
	{
		return FLinearColor(1.0f, 0.9f, 0.2f, 1.0f);
	}
	if (InCategory == TEXT("State"))
	{
		return FLinearColor(0.7f, 0.3f, 0.9f, 1.0f);
	}
	if (InCategory == TEXT("WeaponArt"))
	{
		return FLinearColor(1.0f, 0.5f, 0.1f, 1.0f);
	}
	if (InCategory == TEXT("Camera"))
	{
		return FLinearColor(0.2f, 0.8f, 0.8f, 1.0f);
	}
	if (InCategory == TEXT("SekiroSpecial"))
	{
		return FLinearColor(1.0f, 0.4f, 0.6f, 1.0f);
	}

	// Default: Gray
	return FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);
}

const FSekiroEventParam* FSekiroTaeEvent::FindParam(const FString& InName) const
{
	for (const FSekiroEventParam& Param : Params)
	{
		if (Param.Name.Equals(InName, ESearchCase::IgnoreCase))
		{
			return &Param;
		}
	}

	return nullptr;
}
