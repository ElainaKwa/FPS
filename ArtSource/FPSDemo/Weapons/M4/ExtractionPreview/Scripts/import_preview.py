"""Import experimental M4 workbench meshes through Unreal; never retune weapon definitions."""
from pathlib import Path
from datetime import datetime
import hashlib,json,re,unreal
ROOT=Path(__file__).resolve().parents[6]; SOURCE=Path(__file__).resolve().parents[1]
DEST='/Game/FPSDemo/Art/Weapons/M4/ExtractionPreview'
OUT=ROOT/'Saved/FPSExtraction/WeaponPreview'/datetime.now().strftime('%Y%m%d-%H%M%S')
OUT.mkdir(parents=True,exist_ok=False)
manifest=json.loads((SOURCE/'Manifest.json').read_text(encoding='utf-8'))
owned=ROOT/'Content/FPSDemo/Art/Weapons/M4/ExtractionPreview'
others={p:hashlib.sha256(p.read_bytes()).hexdigest() for p in (ROOT/'Content').rglob('*') if p.suffix in ('.uasset','.umap') and owned not in p.parents}
report=[];lib=unreal.EditorAssetLibrary;tools=unreal.AssetToolsHelpers.get_asset_tools()
for entry in manifest['assets']:
    path=DEST+'/'+entry['name']
    if not lib.does_asset_exist(path):
        task=unreal.AssetImportTask(); task.filename=str(SOURCE/entry['file']);task.destination_path=DEST;task.destination_name=entry['name']
        task.automated=True;task.replace_existing=False;task.save=True;task.factory=unreal.FbxFactory()
        opt=unreal.FbxImportUI();opt.automated_import_should_detect_type=False;opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
        opt.import_as_skeletal=False;opt.import_mesh=True;opt.import_materials=False;opt.import_textures=False
        info=opt.static_mesh_import_data;info.combine_meshes=True;info.generate_lightmap_u_vs=True;info.auto_generate_collision=False
        info.convert_scene=False;info.convert_scene_unit=True;info.force_front_x_axis=False;info.set_editor_property('coordinate_system_policy',unreal.CoordinateSystemPolicy.KEEP_XYZ_AXES)
        info.normal_import_method=unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS;task.options=opt;tools.import_asset_tasks([task])
    mesh=unreal.load_asset(path);assert isinstance(mesh,unreal.StaticMesh),path
    for i,slot in enumerate(mesh.get_editor_property('static_materials')):
        name=re.sub(r'[._]\d{3}$','',str(slot.material_slot_name))
        material=unreal.load_asset('/Game/FPSDemo/Art/Industrial/Materials/'+name);assert material,name
        mesh.set_material(i,material)
    bounds=mesh.get_bounding_box(); size=bounds.max-bounds.min
    actual=[size.x,size.y,size.z];expected=[(b-a)*100 for a,b in zip(entry['bounds_m']['min'],entry['bounds_m']['max'])]
    assert all(abs(a-b)<max(.2,b*.01) for a,b in zip(actual,expected)),(path,actual,expected)
    assert lib.save_loaded_asset(mesh,False),path
    report.append(dict(path=path,size_cm=actual))
assert all(hashlib.sha256(p.read_bytes()).hexdigest()==h for p,h in others.items()),'Unrelated content changed'
(OUT/'Report.json').write_text(json.dumps(dict(status='PASS',assets=report,unrelated_content_unchanged=len(others)),indent=2)+'\n')
unreal.log('EXTRACTION_WEAPON_PREVIEW_IMPORT PASS '+str(OUT/'Report.json'))
