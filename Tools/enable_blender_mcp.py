"""Enable the installed Blender MCP addon and persist only user preferences."""
import json
from pathlib import Path
import bpy
import addon_utils

addon_utils.enable('blender_mcp', default_set=True, persistent=True)
addon = bpy.context.preferences.addons.get('blender_mcp')
assert addon is not None, 'Blender MCP did not enable'
addon.preferences.telemetry_consent = False
bpy.ops.wm.save_userpref()
result = {
    'blender_version': bpy.app.version_string,
    'addon_enabled': True,
    'telemetry_consent': addon.preferences.telemetry_consent,
    'auto_start_default': bpy.context.scene.blendermcp_auto_start_server,
    'port': bpy.context.scene.blendermcp_port,
}
out = Path(__file__).resolve().parents[1] / 'Saved/BlenderMCP/AddonInstall.json'
out.write_text(json.dumps(result, indent=2), encoding='utf-8')
print('BLENDER_MCP_ADDON_ENABLED', json.dumps(result), flush=True)
