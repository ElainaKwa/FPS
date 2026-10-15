"""Run with a rebuilt Unreal Editor Python commandlet; only owns reload assembly assets."""
import json
import hashlib
from pathlib import Path
import unreal

OUT = Path(__file__).resolve().parents[1]
ROOT = next(p for p in OUT.parents if (p / 'MCP_Test.uproject').exists())
lib = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
report = []

def load(path):
    asset = unreal.load_asset(path)
    assert asset, path
    return asset

def xyz(value):
    return (value.x, value.y, value.z)

def bounds_values(mesh):
    box = mesh.get_bounding_box()
    return xyz(box.min), xyz(box.max)

def definition_values(definition):
    fields = ['damage','fire_interval','magazine_size','initial_magazine','initial_reserve','reload_duration','range','weapon_id']
    result = {key: str(definition.get_editor_property(key)) for key in fields}
    for key in ('first_person','third_person','pickup'):
        p = definition.get_editor_property(key)
        t = p.transform
        result[key] = (str(p.mesh), xyz(t.translation), xyz(t.scale3d), xyz(t.rotation), t.rotation.w)
    for key in ('aim_location','hip_location'):
        result[key] = xyz(definition.get_editor_property(key))
    return result

for weapon in json.loads((OUT / 'manifest.json').read_text(encoding='utf-8'))['weapons']:
    parts = {}
    for name, entry in weapon['parts'].items():
        assert hashlib.sha256((OUT / entry['file']).read_bytes()).hexdigest() == entry['sha256'], entry['file']
        task = unreal.AssetImportTask()
        task.filename = str(OUT / entry['file'])
        task.destination_path = entry['destination']
        task.destination_name = entry['name']
        task.automated = True
        task.replace_existing = True
        task.replace_existing_settings = True
        task.save = True
        task.factory = unreal.FbxFactory()
        opt = unreal.FbxImportUI()
        opt.import_mesh = True
        opt.import_materials = False
        opt.import_textures = False
        opt.automated_import_should_detect_type = False
        opt.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
        info = opt.static_mesh_import_data
        info.combine_meshes = True
        info.convert_scene = False
        info.convert_scene_unit = True
        info.set_editor_property('coordinate_system_policy', unreal.CoordinateSystemPolicy.KEEP_XYZ_AXES)
        info.normal_import_method = unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
        task.options = opt
        tools.import_asset_tasks([task])
        mesh = load(entry['destination'] + '/' + entry['name'])
        for i, slot in enumerate(mesh.static_materials):
            material = str(slot.material_slot_name).split('.')[0]
            mesh.set_material(i, load('/Game/FPSDemo/Art/Industrial/Materials/' + material))
        bounds = mesh.get_bounding_box()
        for key, vector in [('min',bounds.min), ('max',bounds.max)]:
            assert all(abs(a - b) < .05 for a,b in zip((vector.x,vector.y,vector.z),entry['unreal_bounds_cm'][key])), (entry['name'],key,str(vector),entry['unreal_bounds_cm'][key])
        assert lib.save_loaded_asset(mesh, False)
        parts[name] = mesh
    original = load(weapon['original'])
    before_bounds = bounds_values(original)
    assert unreal.FPSWeaponReloadRigLibrary.install_reload_rig(original, parts['Body'], parts['Magazine'], parts['Cycle'],
        unreal.Vector(*weapon['magazine_grip']), unreal.Vector(*weapon['cycling_grip']), weapon['cycling_travel'])
    assert lib.save_loaded_asset(original, False)
    assert bounds_values(original) == before_bounds
    definition = load('/Game/FPSDemo/Weapons/Definitions/DA_' + weapon['weapon'])
    before = definition_values(definition)
    # PreSave refreshes resource revisions after the new hard dependencies were saved.
    assert lib.save_loaded_asset(definition, False)
    assert before == definition_values(definition)
    report.append(dict(weapon=weapon['weapon'], original_geometry_unchanged=True, definition_values_unchanged=True,
        parts={k:v.get_path_name() for k,v in parts.items()}))
path = ROOT / 'Saved/FPSAnimationTests/ReloadImport.json'
path.parent.mkdir(parents=True, exist_ok=True)
path.write_text(json.dumps(dict(result='PASS', weapons=report),indent=2)+'\n',encoding='utf-8')
unreal.log('FPS_RELOAD_IMPORT PASS weapons=7 meshes=21')
