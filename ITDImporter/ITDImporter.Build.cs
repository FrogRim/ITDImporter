// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ITDImporter : ModuleRules
{
	public ITDImporter(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
				"ITDImporter/Public"
            }
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
                "ITDImporter/Private",
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
                "Core",
				"CoreUObject",
				"Engine",
				"UnrealEd", // ������ ����� ��� �ʿ�
				"AssetTools", // ���� ���� �۾��� ���� ���
				"Slate",
				"SlateCore",
				"EditorScriptingUtilities", // �ʿ信 ���� �߰�
                "EditorStyle", // �ʿ信 ���� �߰�
				"AssetRegistry",
				"StaticMeshDescription",
                "MeshDescription", // �� ����� �߰��ϼ���
                
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Slate",
				"SlateCore",
                "Projects",
				"UnrealEd",
				"EditorFramework",
                "AssetTools", // ���� ���� �۾��� ���� �ʿ��ϴٸ� �߰�
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
