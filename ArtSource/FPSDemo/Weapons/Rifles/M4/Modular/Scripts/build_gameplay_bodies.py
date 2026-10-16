"""Derive gameplay assembly bases; preserve existing part meshes and source libraries."""
from pathlib import Path
import importlib.util
import hashlib
import json
import bpy

OUT = Path(__file__).resolve().parents[1]
ROOT = next(p for p in OUT.parents if (p / 'MCP_Test.uproject').exists())
source = ROOT / 'Tools/build_industrial_art.py'
spec = importlib.util.spec_from_file_location('industrial', source)
g = importlib.util.module_from_spec(spec)
spec.loader.exec_module(g)
export = g.export
g.export = lambda *args: None
g.OUT = OUT
g.reset()
original = g.weapon('M4')
g.export = export
bpy.context.preferences.filepaths.save_version = 0

# All replaceable pieces are separate. The reload rig also owns the charging handle.
replaceable = ('Pistol grip', 'Grip traction', 'Adjustable stock', 'Stock inset',
               'Butt pad', 'Barrel', 'Magazine', 'Muzzle device', 'Bore', 'Brake port')
assets = []
for suffix, excluded in (('GameplayBody', replaceable),
                         ('GameplayReloadBody', replaceable + ('Charging handle',))):
    col = g.group('SM_M4_Modular' + suffix)
    for obj in original.objects:
        if not obj.name.startswith(excluded):
            col.objects.link(obj)
    assert col.objects
    row = export(col, 'Modular')
    row['destination'] = '/Game/FPSDemo/Art/Weapons/Rifles/M4/Modular/Meshes'
    row['sha256'] = hashlib.sha256((OUT / row['file']).read_bytes()).hexdigest()
    assets.append(row)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT / 'Source/M4_GameplayBodies.blend'))
(OUT / 'GameplayBodiesManifest.json').write_text(json.dumps(dict(
    generator=Path(__file__).relative_to(ROOT).as_posix(), source=source.relative_to(ROOT).as_posix(),
    source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
    scope='M4 gameplay bases with detachable muzzle and magazine', assets=assets),
    ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
print('FPS_GAMEPLAY_BODIES PASS', len(assets), flush=True)
