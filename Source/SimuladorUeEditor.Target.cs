using UnrealBuildTool;
using System.Collections.Generic;

public class SimuladorUeEditorTarget : TargetRules
{
	public SimuladorUeEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("SimuladorUe");
	}
}
