"""Derived assembly: original Industrial weapon generator remains authoritative for shapes."""
import importlib.util
import json
import hashlib
from pathlib import Path
import bpy

OUT = Path(__file__).resolve().parents[1]
ROOT = next(p for p in OUT.parents if (p / 'MCP_Test.uproject').exists())
spec = importlib.util.spec_from_file_location('industrial', ROOT / 'Tools/build_industrial_art.py')
gen = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gen)
export = gen.export
gen.export = lambda *args, **kwargs: None  # Never overwrite the source kit's complete meshes.
gen.OUT = OUT
(OUT / 'Exports').mkdir(parents=True, exist_ok=True)
(OUT / 'Source').mkdir(exist_ok=True)
weapons = []
for kind in ('M4', 'AK', 'BR3', 'MP5', 'UMP45', 'M9', 'HeavyPistol'):
    gen.reset()
    col = gen.weapon(kind)
    pistol = kind in ('M9', 'HeavyPistol')
    if pistol:
        # Hidden inside the existing grip when seated, exposed when the magazine is removed.
        gen.profile('Magazine inner body', [(-.058,-.141),(-.014,-.141),(.011,-.047),(-.027,-.040)], .025, 'Steel', .001)
    groups = {part: bpy.data.collections.new(f'SM_{kind}_Reload{part}') for part in ('Body','Magazine','Cycle')}
    for group in groups.values():
        bpy.context.scene.collection.children.link(group)
    counts = {part: 0 for part in groups}
    for obj in list(col.objects):
        name = obj.name
        moving_slide = pistol and (name == 'Slide' or name.startswith(('Slide serration','Rear sight ear','Front sight base','Front post','Ejection recess')))
        part = 'Magazine' if name.startswith('Magazine') else 'Cycle' if moving_slide or name.startswith('Charging handle') else 'Body'
        col.objects.unlink(obj)
        groups[part].objects.link(obj)
        counts[part] += 1
    assert all(counts.values()), (kind, counts)
    category = 'Pistols' if pistol else 'SMGs' if kind in ('MP5','UMP45') else 'Rifles'
    entries = {}
    for part, group in groups.items():
        entry = export(group, part)
        folder = f'Attachments/Magazines/{kind}' if part == 'Magazine' else f'{category}/{kind}'
        entry['destination'] = f'/Game/FPSDemo/Art/Weapons/{folder}/Meshes'
        entry['objects'] = counts[part]
        lo, hi = entry['bounds_m']['min'], entry['bounds_m']['max']
        # Legacy FBX import changes handedness, as it does for the original complete weapon.
        entry['unreal_bounds_cm'] = {'min':[lo[0]*100,-hi[1]*100,lo[2]*100], 'max':[hi[0]*100,-lo[1]*100,hi[2]*100]}
        entry['sha256'] = hashlib.sha256((OUT / entry['file']).read_bytes()).hexdigest()
        entries[part] = entry
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT / 'Source' / f'{kind}_ReloadParts.blend'))
    weapons.append(dict(weapon=kind, original=f'/Game/FPSDemo/Art/Industrial/Weapons/SM_IND_{kind}', parts=entries,
        magazine_grip=[-3.5,-2.6,-13] if pistol else [14,-2.8,-13],
        cycling_grip=[-1,-3.5,3] if pistol else [7.7,4.7 if kind == 'UMP45' else 4.45,2.4],
        cycling_travel=3.5 if pistol else 4.5))
(OUT / 'manifest.json').write_text(json.dumps(dict(version=1, blender=bpy.app.version_string,
    source='Tools/build_industrial_art.py', source_sha256=hashlib.sha256((ROOT/'Tools/build_industrial_art.py').read_bytes()).hexdigest(),
    generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    generator=str(Path(__file__).relative_to(ROOT)).replace('\\','/'), weapons=weapons), indent=2)+'\n', encoding='utf-8')
print('FPS_RELOAD_EXPORT PASS weapons=7 meshes=21', flush=True)
