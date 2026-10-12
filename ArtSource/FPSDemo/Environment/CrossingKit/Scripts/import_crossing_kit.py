"""Import only CrossingKit meshes; reuse the current shared industrial materials."""
import json
from pathlib import Path
import unreal
OUT=Path(__file__).resolve().parents[1]
ROOT=Path(__file__).resolve().parents[5]
DEST='/Game/FPSDemo/Art/Environment/CrossingKit/Meshes'
tools=unreal.AssetToolsHelpers.get_asset_tools()
assets=unreal.EditorAssetLibrary
records=[]
for item in json.loads((OUT/'manifest.json').read_text(encoding='utf-8'))['assets']:
    task=unreal.AssetImportTask()
    task.filename=str(OUT/item['file'])
    task.destination_path=DEST
    task.destination_name=item['name']
    task.automated=True
    task.replace_existing=True
    task.replace_existing_settings=True
    task.save=True
    task.factory=unreal.FbxFactory()
    opt=unreal.FbxImportUI()
    opt.set_editor_property('automated_import_should_detect_type',False)
    opt.set_editor_property('mesh_type_to_import',unreal.FBXImportType.FBXIT_STATIC_MESH)
    opt.set_editor_property('import_as_skeletal',False)
    opt.set_editor_property('import_mesh',True)
    opt.set_editor_property('import_materials',False)
    opt.set_editor_property('import_textures',False)
    info=opt.get_editor_property('static_mesh_import_data')
    for key,value in [('combine_meshes',True),('generate_lightmap_u_vs',True),('auto_generate_collision',True),
                      ('convert_scene',False),('convert_scene_unit',True),('force_front_x_axis',False),('import_uniform_scale',1.0)]:
        info.set_editor_property(key,value)
    info.set_editor_property('coordinate_system_policy',unreal.CoordinateSystemPolicy.KEEP_XYZ_AXES)
    info.set_editor_property('normal_import_method',unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
    task.options=opt
    tools.import_asset_tasks([task])
    mesh=unreal.load_asset(DEST+'/'+item['name'])
    assert isinstance(mesh,unreal.StaticMesh)
    for i,slot in enumerate(mesh.get_editor_property('static_materials')):
        name=str(slot.get_editor_property('material_slot_name'))
        material=unreal.load_asset('/Game/FPSDemo/Art/Industrial/Materials/'+name)
        assert material, name
        mesh.set_material(i,material)
    mesh.get_editor_property('body_setup').set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    bbox=mesh.get_bounding_box()
    actual=[bbox.max.x-bbox.min.x,bbox.max.y-bbox.min.y,bbox.max.z-bbox.min.z]
    expected=[(b-a)*100 for a,b in zip(item['bounds_m']['min'],item['bounds_m']['max'])]
    assert all(abs(a-b)<max(.2,b*.01) for a,b in zip(actual,expected)),(item['name'],actual,expected)
    assert assets.save_loaded_asset(mesh,False)
    records.append({'asset':mesh.get_path_name(),'dimensions_cm':actual,'expected_cm':expected})
report=ROOT/'Saved/IndustrialCrossing'
report.mkdir(parents=True,exist_ok=True)
(report/'KitImport.json').write_text(json.dumps({'status':'passed','meshes':records},indent=2),encoding='utf-8')
unreal.log('CROSSING_KIT IMPORT PASS')
