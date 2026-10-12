"""Verify actual MCP stdio handshake and a reversible edit in the running Blender."""
import asyncio
import hashlib
import json
import os
from pathlib import Path
from mcp import ClientSession, StdioServerParameters
from mcp.client.stdio import stdio_client

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'Saved/BlenderMCP'
SOURCE = ROOT / 'ArtSource/FPSDemo/Industrial/WeaponLibrary.blend'


async def main():
    before = hashlib.sha256(SOURCE.read_bytes()).hexdigest()
    params = StdioServerParameters(
        command=str(ROOT / '.tools/blender-mcp/venv/Scripts/python.exe'),
        args=['-m', 'blender_mcp.server'],
        env={**os.environ, 'DISABLE_TELEMETRY': 'true',
             'BLENDER_HOST': '127.0.0.1', 'BLENDER_PORT': '9876'},
    )
    report = {'success': False, 'checks': {}}
    with (OUT / 'ServerTest.log').open('w', encoding='utf-8') as errors:
        async with stdio_client(params, errlog=errors) as (read, write):
            async with ClientSession(read, write) as session:
                init = await session.initialize()
                report['server'] = init.serverInfo.model_dump()
                listed = await session.list_tools()
                report['tools'] = [t.name for t in listed.tools]
                for name, arguments in [
                    ('get_addon_status', {}),
                    ('get_scene_info', {'limit': 3}),
                    ('execute_blender_code', {'code': '''import bpy, json
before_count = len(bpy.data.objects)
mesh = bpy.data.meshes.new("MCP_ConnectionProbeMesh")
mesh.from_pydata([(0,0,0),(1,0,0),(0,1,0)], [], [(0,1,2)])
obj = bpy.data.objects.new("MCP_ConnectionProbe", mesh)
bpy.context.scene.collection.objects.link(obj)
obj.location.x = 1.25
assert obj.location.x == 1.25 and len(obj.data.polygons) == 1
bpy.data.objects.remove(obj, do_unlink=True)
bpy.data.meshes.remove(mesh)
assert len(bpy.data.objects) == before_count
print(json.dumps({"probe": "PASS", "objects": before_count, "file": bpy.data.filepath}))
'''}),
                ]:
                    result = await session.call_tool(name, arguments)
                    text = '\n'.join(c.text for c in result.content if c.type == 'text')
                    assert not result.isError, (name, text)
                    report['checks'][name] = text
                    if name == 'execute_blender_code':
                        assert 'PASS' in text, text
                assert hashlib.sha256(SOURCE.read_bytes()).hexdigest() == before
                report['source_file_unchanged'] = True
                report['success'] = True
    (OUT / 'ConnectionReport.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps(report, ensure_ascii=False, indent=2))


asyncio.run(main())
