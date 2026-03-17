// Copyright (c) 2026 SekiroSkillEditor. All Rights Reserved.

#include "Data/SekiroCharacterData.h"

const FSekiroDummyPoly* USekiroCharacterData::FindDummyPoly(int32 InReferenceId) const
{
	for (const FSekiroDummyPoly& DummyPoly : DummyPolys)
	{
		if (DummyPoly.ReferenceId == InReferenceId)
		{
			return &DummyPoly;
		}
	}

	return nullptr;
}
