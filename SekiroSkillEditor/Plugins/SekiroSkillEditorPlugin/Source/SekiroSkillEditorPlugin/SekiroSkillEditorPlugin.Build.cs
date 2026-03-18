using UnrealBuildTool;

public class SekiroSkillEditorPlugin : ModuleRules
{
	public SekiroSkillEditorPlugin(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"Slate",
			"SlateCore",
			"UnrealEd",
			"EditorStyle",
			"Json",
			"JsonUtilities",
			"AssetTools",
			"ContentBrowser",
			"ToolMenus",
			"EditorFramework",
			"Projects"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"PropertyEditor",
			"LevelEditor",
			"AdvancedPreviewScene",
			"EditorViewport",
			"WorkspaceMenuStructure",
			"EditorWidgets",
			"EditorScriptingUtilities",
			"InterchangeCore",
			"InterchangeEngine"
		});
	}
}
