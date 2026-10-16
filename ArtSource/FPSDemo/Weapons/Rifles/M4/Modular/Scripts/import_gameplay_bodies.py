"""Enable saved M4 attachment visuals without changing gameplay tuning or other weapons."""
from pathlib import Path
from datetime import datetime
import hashlib
import json
import re
import shutil
import unreal

SOURCE = Path(__file__).resolve().parents[1]
ROOT = next(p for p in SOURCE.parents if (p / 'MCP_Test.uproject').exists())
OUT = ROOT / 'Saved/RaidSpawnAttachments/Authoring' / datetime.now().strftime('%Y%m%d-%H%M%S')
OUT.mkdir(parents=True, exist_ok=False)
lib = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
report = []

def before(path):
    file = ROOT / 'Content' / (path.removeprefix('/Game/') + '.uasset')
    row = dict(path=path, existed=file.exists())
    if file.exists():
        row['before_sha256'] = hashlib.sha256(file.read_bytes()).hexdigest()
        shutil.copy2(file, OUT / file.name)
    report.append(row)
    return row, file

def save(asset, row, file):
    assert lib.save_loaded_asset(asset, False), row['path']
    row['after_sha256'] = hashlib.sha256(file.read_bytes()).hexdigest()

meshes = {}
manifest = json.loads((SOURCE / 'GameplayBodiesManifest.json').read_text(encoding='utf-8'))
for entry in manifest['assets']:
    path = entry['destination'] + '/' + entry['name']
    row, file = before(path)
    task = unreal.AssetImportTask()
    task.filename = str(SOURCE / entry['file'])
    task.destination_path = entry['destination']
    task.destination_name = entry['name']
    task.automated = True
    task.replace_existing = True
    task.save = True
    task.factory = unreal.FbxFactory()
    opt = unreal.FbxImportUI()
    opt.automated_import_should_detect_type = False
    opt.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    opt.import_as_skeletal = False
    opt.import_mesh = True
    opt.import_materials = False
    opt.import_textures = False
    info = opt.static_mesh_import_data
    info.combine_meshes = True
    info.generate_lightmap_u_vs = True
    info.auto_generate_collision = False
    info.convert_scene = False
    info.convert_scene_unit = True
    info.set_editor_property('coordinate_system_policy', unreal.CoordinateSystemPolicy.KEEP_XYZ_AXES)
    info.normal_import_method = unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
    task.options = opt
    tools.import_asset_tasks([task])
    mesh = unreal.load_asset(path)
    assert isinstance(mesh, unreal.StaticMesh), path
    for i, slot in enumerate(mesh.static_materials):
        name = re.sub(r'[._]\d{3}$', '', str(slot.material_slot_name))
        material = unreal.load_asset('/Game/FPSDemo/Art/Industrial/Materials/' + name)
        assert material, name
        mesh.set_material(i, material)
    bounds = mesh.get_bounding_box()
    size = bounds.max - bounds.min
    expected = [(hi - lo) * 100 for lo, hi in zip(entry['bounds_m']['min'], entry['bounds_m']['max'])]
    assert all(abs(a - b) < max(.15, b * .01) for a, b in zip((size.x, size.y, size.z), expected))
    save(mesh, row, file)
    meshes[entry['name']] = mesh

# Newly imported packages need their on-disk hashes in the registry before definitions record them.
registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.scan_modified_asset_files([str(ROOT / 'Content' / (entry['destination'].removeprefix('/Game/') + '/' + entry['name'] + '.uasset')) for entry in manifest['assets']])
registry.wait_for_completion()

for suffix in ('OpticFixed', 'MuzzleBrake', 'MagazineExtended'):
    path = '/Game/FPSDemo/Weapons/Attachments/DA_M4_' + suffix
    row, file = before(path)
    part = unreal.load_asset(path)
    preview = part.get_editor_property('workbench')
    assert preview.mesh, path
    # The preview was authored in the same M4 local coordinates as all modular parts.
    for field in ('first_person', 'third_person', 'pickup'):
        part.set_editor_property(field, preview)
    part.set_editor_property('use_weapon_presentation', False)
    save(part, row, file)

path = '/Game/FPSDemo/Weapons/Definitions/DA_M4'
row, file = before(path)
weapon = unreal.load_asset(path)
weapon.set_editor_property('modular_body', meshes['SM_M4_ModularGameplayBody'])
weapon.set_editor_property('modular_reload_body', meshes['SM_M4_ModularGameplayReloadBody'])
builtin = dict(weapon.get_editor_property('builtin_parts'))
preview_parts = dict(weapon.get_editor_property('workbench_parts'))
for name in ('Muzzle', 'Magazine'):
    key = unreal.Name('Mount.' + name)
    assert preview_parts[key].mesh, name
    builtin[key] = preview_parts[key]
weapon.set_editor_property('builtin_parts', builtin)
save(weapon, row, file)
assert all(str(version) for version in weapon.get_editor_property('saved_resource_versions').values()), 'Unrecorded weapon resource revision'
(OUT / 'Report.json').write_text(json.dumps(dict(status='PASS', assets=report), ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
unreal.log('FPS_GAMEPLAY_BODIES_IMPORT PASS ' + str(OUT / 'Report.json'))
