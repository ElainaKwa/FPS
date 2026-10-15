"""Editor-only import and idempotent data migration; existing tuning is preserved."""
from pathlib import Path
from datetime import datetime
import hashlib, json, re
import unreal

SOURCE=Path(__file__).resolve().parents[1]
ROOT=next(p for p in SOURCE.parents if (p/'MCP_Test.uproject').exists())
manifest=json.loads((SOURCE/'Manifest.json').read_text(encoding='utf-8'))
lib=unreal.EditorAssetLibrary
tools=unreal.AssetToolsHelpers.get_asset_tools()
report=[]
meshes={}
for entry in manifest['assets']:
    path=entry['destination']+'/'+entry['name']
    if not lib.does_asset_exist(path):
        task=unreal.AssetImportTask();task.filename=str(SOURCE/entry['file']);task.destination_path=entry['destination'];task.destination_name=entry['name']
        task.automated=True;task.replace_existing=False;task.save=True;task.factory=unreal.FbxFactory()
        opt=unreal.FbxImportUI();opt.automated_import_should_detect_type=False;opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
        opt.import_as_skeletal=False;opt.import_mesh=True;opt.import_materials=False;opt.import_textures=False
        info=opt.static_mesh_import_data;info.combine_meshes=True;info.generate_lightmap_u_vs=True;info.auto_generate_collision=False
        info.convert_scene=False;info.convert_scene_unit=True;info.set_editor_property('coordinate_system_policy',unreal.CoordinateSystemPolicy.KEEP_XYZ_AXES)
        info.normal_import_method=unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS;task.options=opt;tools.import_asset_tasks([task])
        mesh=unreal.load_asset(path);assert isinstance(mesh,unreal.StaticMesh),path
        for i,slot in enumerate(mesh.static_materials):
            name=re.sub(r'[._]\d{3}$','',str(slot.material_slot_name))
            material=unreal.load_asset('/Game/FPSDemo/Art/Industrial/Materials/'+name);assert material,name
            mesh.set_material(i,material)
        assert lib.save_loaded_asset(mesh,False),path
    mesh=unreal.load_asset(path);assert isinstance(mesh,unreal.StaticMesh),path
    bounds=mesh.get_bounding_box();size=bounds.max-bounds.min
    expected=[(hi-lo)*100 for lo,hi in zip(entry['bounds_m']['min'],entry['bounds_m']['max'])]
    assert all(abs(a-b)<max(.15,b*.01) for a,b in zip((size.x,size.y,size.z),expected)),(path,str(size),expected)
    meshes[entry['name'].removeprefix('SM_M4_Modular')]=mesh
    report.append(dict(path=path,size_cm=[size.x,size.y,size.z]))

def view(mesh):
    value=unreal.FPSWeaponPresentation();value.set_editor_property('mesh',mesh);value.set_editor_property('transform',unreal.Transform());return value

# Initial authoring values only; reruns never retune an existing saved definition.
specs=[
 ('RearGripErgo','灵巧后握把',unreal.FPSAttachmentCategory.REAR_GRIP,'RearGrip',.88,1.04,1.04,1),
 ('RearGripControl','控制后握把',unreal.FPSAttachmentCategory.REAR_GRIP,'RearGrip',1.06,.93,.90,1),
 ('StockLight','轻型枪托',unreal.FPSAttachmentCategory.STOCK,'Stock',.90,1.08,1.08,1),
 ('StockStable','稳定枪托',unreal.FPSAttachmentCategory.STOCK,'Stock',1.12,.85,.90,1),
 ('BarrelLight','轻型枪管',unreal.FPSAttachmentCategory.BARREL,'Barrel',.93,1.04,1.03,.90),
 ('BarrelHeavy','重型枪管',unreal.FPSAttachmentCategory.BARREL,'Barrel',1.10,.92,.96,1.25)]
weapon=unreal.load_asset('/Game/FPSDemo/Weapons/Definitions/DA_M4');assert weapon
platform=weapon.get_editor_property('platform_id')
offers=list(weapon.get_editor_property('available_attachments'))
for suffix,label,category,mount,aim,vertical,horizontal,distance in specs:
    path='/Game/FPSDemo/Weapons/Attachments/DA_M4_'+suffix
    part=unreal.load_asset(path) if lib.does_asset_exist(path) else None
    if part is None:
        factory=unreal.DataAssetFactory();factory.set_editor_property('data_asset_class',unreal.FPSAttachmentDefinition)
        part=tools.create_asset('DA_M4_'+suffix,'/Game/FPSDemo/Weapons/Attachments',unreal.FPSAttachmentDefinition,factory)
        for key,value in dict(attachment_id='Attachment.M4_'+suffix,display_name=label,category=category,mount_id='Mount.'+mount,
            platforms=[platform],aim_time_multiplier=aim,vertical_recoil_multiplier=vertical,horizontal_recoil_multiplier=horizontal,
            range_multiplier=distance,use_weapon_presentation=False,first_person=view(meshes[suffix]),third_person=view(meshes[suffix]),
            pickup=view(meshes[suffix]),workbench=view(meshes[suffix])).items():part.set_editor_property(key,value)
        assert lib.save_loaded_asset(part,False),path
    if part not in offers:offers.append(part)

legacy='/Game/FPSDemo/Art/Weapons/M4/ExtractionPreview/SM_EXP_M4_'
for suffix,mesh_name in [('OpticFixed','Optic'),('MuzzleBrake','Brake'),('MagazineExtended','MagazineExtended')]:
    part=unreal.load_asset('/Game/FPSDemo/Weapons/Attachments/DA_M4_'+suffix);assert part
    if part.get_editor_property('workbench').mesh is None:
        mesh=unreal.load_asset(legacy+mesh_name);assert mesh
        part.set_editor_property('workbench',view(mesh));assert lib.save_loaded_asset(part,False)

weapon.set_editor_property('available_attachments',offers)
mounts=list(weapon.get_editor_property('mount_ids'))
for name in ('RearGrip','Stock','Barrel'):
    if unreal.Name('Mount.'+name) not in mounts:mounts.append(unreal.Name('Mount.'+name))
weapon.set_editor_property('mount_ids',mounts)
for field,mesh_name in [('modular_body','Body'),('modular_reload_body','ReloadBody'),('workbench_body','WorkbenchBody')]:
    if weapon.get_editor_property(field) is None:weapon.set_editor_property(field,meshes[mesh_name])
builtin=dict(weapon.get_editor_property('builtin_parts'))
for name in ('RearGrip','Stock','Barrel'):builtin.setdefault(unreal.Name('Mount.'+name),view(meshes[name+'Standard']))
weapon.set_editor_property('builtin_parts',builtin)
workbench=dict(weapon.get_editor_property('workbench_parts'))
for name,mesh_name in [('Muzzle','MuzzleStandard'),('Magazine','MagazineStandard')]:
    mesh=unreal.load_asset(legacy+mesh_name);assert mesh
    workbench.setdefault(unreal.Name('Mount.'+name),view(mesh))
weapon.set_editor_property('workbench_parts',workbench)
locations=dict(weapon.get_editor_property('attachment_mount_locations'))
for name,xyz in dict(Optic=(10,0,10),Muzzle=(56,0,0),Magazine=(15,0,-17),RearGrip=(-2,0,-8),Stock=(-28,0,-2),Barrel=(48,0,0)).items():
    locations.setdefault(unreal.Name('Mount.'+name),unreal.Vector(*xyz))
weapon.set_editor_property('attachment_mount_locations',locations)
assert lib.save_loaded_asset(weapon,False)
output=ROOT/'Saved/ModularPartsTests'/('Import-'+datetime.now().strftime('%Y%m%d-%H%M%S'))
output.mkdir(parents=True,exist_ok=False)
(output/'Report.json').write_text(json.dumps(dict(status='PASS',assets=report,definitions=len(specs),offers=len(offers)),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
unreal.log('FPS_MODULAR_IMPORT PASS '+str(output/'Report.json'))
