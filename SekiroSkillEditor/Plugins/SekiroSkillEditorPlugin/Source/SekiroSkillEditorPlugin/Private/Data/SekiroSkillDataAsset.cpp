// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

#include "Data/SekiroSkillDataAsset.h"

TArray<FSekiroTaeEvent> USekiroSkillDataAsset::GetEventsByCategory(const FString& InCategory) const
{
	TArray<FSekiroTaeEvent> Result;
	for (const FSekiroTaeEvent& Event : Events)
	{
		if (Event.Category == InCategory)
		{
			Result.Add(Event);
		}
	}
	return Result;
}

TArray<FSekiroTaeEvent> USekiroSkillDataAsset::GetEventAtFrame(float Frame) const
{
	TArray<FSekiroTaeEvent> Result;
	for (const FSekiroTaeEvent& Event : Events)
	{
		if (Event.StartFrame <= Frame && Frame <= Event.EndFrame)
		{
			Result.Add(Event);
		}
	}
	return Result;
}

bool USekiroSkillDataAsset::GetRootMotionSampleAtFrame(float Frame, FSekiroRootMotionSample& OutSample) const
{
	const int32 RoundedFrame = FMath::RoundToInt(Frame);
	for (const FSekiroRootMotionSample& Sample : RootMotion.Samples)
	{
		if (Sample.FrameIndex == RoundedFrame)
		{
			OutSample = Sample;
			return true;
		}
	}

	return false;
}
