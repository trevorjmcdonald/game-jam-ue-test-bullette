using UnrealBuildTool;

public class JamKit : ModuleRules
{
	public JamKit(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Module-relative includes ("UI/JamKitMenuWidget.h", "JamKit.h") from any subfolder, here and in games.
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "DeveloperSettings" });
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "GameplayTags", "Niagara", "RHI", "RenderCore", "ImageCore", "Json" });
	}
}
