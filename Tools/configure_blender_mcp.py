"""Add this project's installed Blender MCP server without changing other servers."""
from pathlib import Path
import tomllib

root = Path(__file__).resolve().parents[1]
config = root / '.codex/config.toml'
content = config.read_text(encoding='utf-8')
parsed = tomllib.loads(content)
python = root / '.tools/blender-mcp/venv/Scripts/python.exe'
assert python.is_file(), 'Install the project-local MCP environment first'
if 'blender' in parsed.get('mcp_servers', {}):
    previous = (root / 'Saved/BlenderMCP/venv/Scripts/python.exe').as_posix()
    if parsed['mcp_servers']['blender']['command'] == previous:
        config.write_text(content.replace(previous, python.as_posix()), encoding='utf-8')
        print('Updated the installed Blender MCP runtime location')
        raise SystemExit(0)
    raise SystemExit('Blender config already exists; inspect it instead of overwriting')
addition = f'''

[mcp_servers.blender]
command = '{python.as_posix()}'
args = ["-m", "blender_mcp.server"]
startup_timeout_sec = 30
tool_timeout_sec = 180

[mcp_servers.blender.env]
BLENDER_HOST = "127.0.0.1"
BLENDER_PORT = "9876"
DISABLE_TELEMETRY = "true"
'''
tomllib.loads(content + addition)
backup = root / 'Saved/BlenderMCP/config.before-blender.toml'
if not backup.exists():
    backup.write_text(content, encoding='utf-8')
config.write_text(content + addition, encoding='utf-8')
print('Configured project Blender MCP; preserved existing configuration')
