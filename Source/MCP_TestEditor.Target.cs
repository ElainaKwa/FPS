// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class MCP_TestEditorTarget : TargetRules
{
	public MCP_TestEditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("MCP_Test");

		// Content is part of a successful project editor build, not just C++ compilation.
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PostBuildSteps.Add("\"$(EngineDir)/Binaries/Win64/UnrealEditor-Cmd.exe\" \"$(ProjectFile)\" -run=FPSWeaponData -unattended -nop4 -nullrhi -nosound -ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False -abslog=\"$(ProjectDir)/Saved/Logs/FPSWeaponBuildValidation.log\"");
		}
	}
}
