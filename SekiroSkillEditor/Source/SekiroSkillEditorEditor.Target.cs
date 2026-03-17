using UnrealBuildTool;
using System.Collections.Generic;

public class SekiroSkillEditorEditorTarget : TargetRules
{
	public SekiroSkillEditorEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		ExtraModuleNames.Add("SekiroSkillEditor");
	}
}
