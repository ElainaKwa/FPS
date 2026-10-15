"""Import the validated modular M7 visual assets into categorized UE folders.

Run inside Unreal's Python commandlet with legacy FBX importing enabled.
This importer creates art resources only. Source geometry, source PNGs,
gameplay weapon definitions and existing gameplay levels are not modified.
"""
import datetime
import hashlib
import json
import re
import sys
import traceback
from pathlib import Path

import unreal

ROOT = Path(__file__).resolve().parents[1]
PROJECT = Path(unreal.Paths.project_dir()).resolve()
sys.path.insert(0, str(ROOT / 'Scripts'))
from ue_m7_materials import build_materials, OWNER, OWNER_KEY

CORE_ROOT = '/Game/FPSDemo/Art/Weapons/Rifles/M7'
ATTACHMENT_ROOT = '/Game/FPSDemo/Art/Weapons/Attachments'
SLOT_FOLDERS = {'Stock':'Stocks', 'Magazine':'Magazines', 'RearGrip':'RearGrips',
                'Optic':'Optics', 'Foregrip':'Foregrips', 'Muzzle':'Muzzles',
                'Sights':'Sights', 'Barrel':'Barrels'}
LIB = unreal.EditorAssetLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()


def digest(path):
    value=hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda:stream.read(1024*1024),b''):value.update(block)
    return value.hexdigest()


def folder_for(row):
    if row['category']=='Core':return CORE_ROOT
    return ATTACHMENT_ROOT+'/'+SLOT_FOLDERS[row['category']]+'/M7'


def mesh_path(row):
    folder=CORE_ROOT+'/Presets' if 'parts' in row else folder_for(row)+'/Meshes'
    return folder+'/'+row['name']


def ensure_owned(path):
    if not LIB.does_asset_exist(path):return
    asset=unreal.load_asset(path)
    if str(LIB.get_metadata_tag(asset,OWNER_KEY))!=OWNER:
        raise RuntimeError('Refusing to overwrite an unrelated UE asset: '+path)
    if not isinstance(asset,unreal.StaticMesh):raise TypeError(path+' is not a StaticMesh')


def settings():
    opt=unreal.FbxImportUI()
    values={'automated_import_should_detect_type':False,
            'mesh_type_to_import':unreal.FBXImportType.FBXIT_STATIC_MESH,
            'import_as_skeletal':False,'import_mesh':True,
            'import_materials':False,'import_textures':False,'import_animations':False}
    for key,value in values.items():opt.set_editor_property(key,value)
    info=opt.get_editor_property('static_mesh_import_data')
    values={'combine_meshes':True,'auto_generate_collision':False,
            'generate_lightmap_u_vs':False,'remove_degenerates':False,
            'convert_scene':True,'convert_scene_unit':True,'force_front_x_axis':True,
            'coordinate_system_policy':unreal.CoordinateSystemPolicy.MATCH_UP_FORWARD_AXES,
            'import_uniform_scale':1.0,
            'import_rotation':unreal.Rotator(pitch=0,yaw=90,roll=0),
            'normal_import_method':unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS}
    for key,value in values.items():info.set_editor_property(key,value)
    return opt


def verify_mesh(mesh,row):
    bb=mesh.get_bounding_box()
    observed={'min':[bb.min.x,bb.min.y,bb.min.z], 'max':[bb.max.x,bb.max.y,bb.max.z]}
    bounds=row.get('assembly_bounds_m',row.get('local_bounds_m'))
    expected={key:[v*100 for v in bounds[key]] for key in ('min','max')}
    # FBX is right-handed; UE reflects Y while retaining the corrected X/Z.
    expected['min'][1],expected['max'][1]=-expected['max'][1],-expected['min'][1]
    errors=[abs(a-b) for key in ('min','max') for a,b in zip(observed[key],expected[key])]
    assert max(errors)<.01,(row['name'],'centimetre bounds mismatch',observed,expected)
    triangles=mesh.get_num_triangles(0)
    geometry=json.loads((ROOT/'UEGeometryPreflight.json').read_text(encoding='utf-8'))
    assert geometry['success']
    evidence=next(item for item in geometry['assets'] if item['name']==row['name'])
    assert evidence['source_sha256']==digest(ROOT/row['exports']['fbx'])
    assert evidence['source_triangles']==row['triangles']
    # FBX stores n-gons. UE/FBX SDK and Blender may triangulate collinear
    # corners differently; float32 centimetre positions can also coincide.
    # Bound differences by measured exact source/float32 degenerates only.
    # Near-point threshold candidates do not increase this budget.
    budget=evidence['strong_degenerate_candidate_count']
    difference=row['triangles']-triangles
    assert triangles>0,(row['name'],'empty UE render geometry')
    needs_review=not 0<=difference<=budget
    assert mesh.get_num_tex_coords(0)>=1,(row['name'],'missing UV0')
    return {'bounds_cm':observed,'expected_bounds_cm':expected,'maximum_bound_error_cm':max(errors),
            'triangles':triangles,'source_triangles':row['triangles'],
            'triangulation_triangle_delta':difference,'degenerate_candidate_budget':budget,
            'needs_surface_review':needs_review,
            'triangle_validation':'Counts recorded; n-gon tessellation is not topology invariant. Differences beyond measured degeneracy candidates require render-surface area review.',
            'geometry_preflight':'UEGeometryPreflight.json',
            'uv_channels':mesh.get_num_tex_coords(0),'lod_count':mesh.get_num_lods()}


def socket_positions(row,manifest):
    if row.get('category')=='Core':
        return {key:value for key,value in manifest['mounts'].items()
                if key not in ('Core','Foregrip','Muzzle','FrontIron')}
    if row.get('category')=='Barrel':return row['local_mounts_m']
    return {}


def configure_sockets(mesh,row,manifest):
    positions=socket_positions(row,manifest)
    result={}
    for name,point in positions.items():
        socket=mesh.find_socket(name)
        if socket is None:
            socket=unreal.new_object(unreal.StaticMeshSocket,outer=mesh)
            socket.set_editor_property('socket_name',name)
            mesh.add_socket(socket)
        location=[point[0]*100,-point[1]*100,point[2]*100]
        socket.set_editor_property('relative_location',unreal.Vector(*location))
        socket.set_editor_property('relative_rotation',unreal.Rotator(pitch=0,yaw=0,roll=0))
        socket.set_editor_property('relative_scale',unreal.Vector(1,1,1))
        observed=socket.get_editor_property('relative_location')
        actual=[observed.x,observed.y,observed.z]
        assert max(abs(a-b) for a,b in zip(actual,location))<1e-4
        result[name]=actual
    return result


def slot_key(name,materials):
    if name in materials:return name
    sanitized=[key for key in materials if key.replace('.','_')==name]
    if len(sanitized)==1:return sanitized[0]
    canonical=re.sub(r'(?:\.\d{3})+$','',name)
    if canonical in materials:return canonical
    raise KeyError('Unrecognized FBX material slot: '+name)


def export_render_geometry_for_review(mesh,row):
    destination=PROJECT/'Saved/M7UnrealImport/GeometryReview'/(row['name']+'.fbx')
    destination.parent.mkdir(parents=True,exist_ok=True)
    options=unreal.FbxExportOption()
    for key,value in {'ascii':False,'force_front_x_axis':True,'export_source_mesh':False,
                      'collision':False,'level_of_detail':False,
                      'bake_material_inputs':unreal.FbxMaterialBakeMode.DISABLED}.items():
        options.set_editor_property(key,value)
    task=unreal.AssetExportTask()
    for key,value in {'object':mesh,'filename':str(destination),'automated':True,
                      'prompt':False,'replace_identical':True,
                      'exporter':unreal.StaticMeshExporterFBX(),'options':options}.items():
        task.set_editor_property(key,value)
    assert unreal.Exporter.run_asset_export_task(task),'Cannot export UE render geometry '+row['name']
    assert destination.is_file() and destination.stat().st_size>0
    return str(destination.relative_to(PROJECT)).replace('\\','/')


def import_mesh(row,manifest,materials):
    path=mesh_path(row);ensure_owned(path)
    folder,name=path.rsplit('/',1)
    mesh=unreal.load_asset(path) if LIB.does_asset_exist(path) else None
    source_sha=digest(ROOT/row['exports']['fbx'])
    reusable=mesh is not None and str(LIB.get_metadata_tag(mesh,'m7_source_sha256'))==source_sha and str(LIB.get_metadata_tag(mesh,'m7_forward_axis'))=='+X' and str(LIB.get_metadata_tag(mesh,'m7_units'))=='centimetres'
    if not reusable:
        task=unreal.AssetImportTask()
        for key,value in {'filename':str(ROOT/row['exports']['fbx']),
            'destination_path':folder,'destination_name':name,
            'automated':True,'save':False,'replace_existing':True,
            'replace_existing_settings':True,'factory':unreal.FbxFactory(),'options':settings()}.items():
            task.set_editor_property(key,value)
        TOOLS.import_asset_tasks([task])
        mesh=unreal.load_asset(path)
        assert isinstance(mesh,unreal.StaticMesh),(path,task.imported_object_paths)
    # Tag immediately so an interrupted import can resume only its own assets.
    LIB.set_metadata_tag(mesh,OWNER_KEY,OWNER)
    assigned=[]
    for i,slot in enumerate(mesh.get_editor_property('static_materials')):
        name=str(slot.get_editor_property('material_slot_name'))
        key=slot_key(name,materials)
        mesh.set_material(i,materials[key])
        assigned.append({'slot':name,'source_slot':key,'material':materials[key].get_path_name()})
    assert {x['source_slot'] for x in assigned}==set(row['used_materials']),(path,'material coverage mismatch')
    proof=verify_mesh(mesh,row)
    sockets=configure_sockets(mesh,row,manifest)
    for key,value in {'m7_source_fbx':row['exports']['fbx'],'m7_source_sha256':source_sha,
                      'm7_category':row.get('category','AssembledPreset'),'m7_model':'M7',
                      'm7_rigged':'false','m7_forward_axis':'+X','m7_units':'centimetres'}.items():
        LIB.set_metadata_tag(mesh,key,value)
    if 'compatible_barrels' in row:
        LIB.set_metadata_tag(mesh,'m7_compatible_barrels',json.dumps(row['compatible_barrels']))
    assert LIB.save_loaded_asset(mesh,False),'Failed to save '+path
    assert all(mesh.get_material(i)==materials[entry['source_slot']] for i,entry in enumerate(assigned))
    result={'name':row['name'],'path':path,'category':row.get('category','Preset'),
            'class':mesh.get_class().get_name(),'materials':assigned,'sockets_cm':sockets,
            'source_fbx':row['exports']['fbx'],'saved':True,'reused_unchanged_import':reusable,**proof}
    if proof['needs_surface_review']:
        result['geometry_review_export']=export_render_geometry_for_review(mesh,row)
    if 'parts' in row:result['parts']=row['parts'];result['part_mounts_cm']={n:[v*100 for v in p] for n,p in row['part_mounts_m'].items()}
    unreal.log('M7_UE_IMPORTED '+path)
    return result


def main():
    manifest=json.loads((ROOT/'AssetManifest.json').read_text(encoding='utf-8'))
    validation=json.loads((ROOT/'Validation.json').read_text(encoding='utf-8'))
    assert validation['success'] and len(manifest['assets'])==19 and len(manifest['presets'])==3
    rows=manifest['assets']+manifest['presets']
    report={'generator':'Scripts/ue_import_m7.py','generated_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
            'engine':unreal.SystemLibrary.get_engine_version(),'success':False,
            'weapon_category':'Rifles','model':'M7','core_root':CORE_ROOT,'attachment_root':ATTACHMENT_ROOT,
            'slot_folders':SLOT_FOLDERS,'assets':[],'textures':[],'materials':[],'errors':[],
            'import_settings':{'coordinate_system_policy':'MATCH_UP_FORWARD_AXES','import_yaw_degrees':90,
                'convert_scene_unit':True,'uniform_scale':1,'target_units':'centimetres','target_forward_axis':'+X',
                'normal_method':'ImportNormals/MikkTSpaceTangents','generate_lightmap_uv':False,
                'auto_generate_collision':False,'rigged':False,'gameplay_integration':False},
            'source_blend_sha256':digest(ROOT/manifest['source_file']),
            'blender_validation_source_sha256':validation['source_sha256_after'],'source_inputs':{}}
    destination=ROOT/'UnrealImportReport.json'
    try:
        required={'AssetManifest.json','MaterialReport.json','AttachmentRegistry.json'}
        for row in rows:required.update(row['exports'].values())
        for row in manifest['assets']:
            for channels in row['texture_sets'].values():required.update(channels.values())
        for relative in sorted(required):
            sha=digest(ROOT/relative)
            assert validation['validated_files'].get(relative)==sha,('Input differs from validated export',relative)
            report['source_inputs'][relative]=sha
        proof_path=ROOT/'SourceContentComparison.json'
        if report['source_blend_sha256']!=report['blender_validation_source_sha256']:
            proof=json.loads(proof_path.read_text(encoding='utf-8'))
            assert proof['files_unchanged'] and all(not value for category in proof['summary'].values() for value in category.values())
            assert report['source_blend_sha256'] in proof['files_after'].values()
            report['source_content_comparison']={'report':'SourceContentComparison.json',
                'sha256':digest(proof_path),'asset_geometry_uv_modifiers_materials_images_scenes_equal':True,
                'note':'Byte-level source differs; checked asset contents match the archived validated source.'}
        for row in rows:ensure_owned(mesh_path(row))
        geometry_path=ROOT/'UEGeometryPreflight.json'
        geometry=json.loads(geometry_path.read_text(encoding='utf-8'))
        assert geometry['success'] and len(geometry['assets'])==22
        assert {row['name'] for row in geometry['assets']}=={row['name'] for row in rows}
        for proof in geometry['assets']:
            assert proof['source_sha256']==digest(ROOT/proof['source_file'])
        report['geometry_preflight']={'file':'UEGeometryPreflight.json','sha256':digest(geometry_path)}
        materials,_=build_materials(manifest,ROOT,CORE_ROOT,folder_for,report)
        for row in rows:
            report['assets'].append(import_mesh(row,manifest,materials))
            destination.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
            # These dense source meshes otherwise retain a large amount of
            # import-transient data until the entire MCP call finishes.
            unreal.SystemLibrary.collect_garbage()
        assert len(report['assets'])==22 and len(report['textures'])==57
        report['success']=True
        report['summary']={'meshes':22,'independent_components':19,'assembled_presets':3,
                           'textures':len(report['textures']),'materials':len(report['materials']),
                           'socket_meshes':sum(bool(a['sockets_cm']) for a in report['assets']),
                           'surface_reviews_required':sum(a['needs_surface_review'] for a in report['assets'])}
        unreal.log('M7_UE_IMPORT_COMPLETE '+json.dumps(report['summary']))
    except Exception:
        report['errors'].append(traceback.format_exc())
        unreal.log_error('M7_UE_IMPORT_FAILED '+report['errors'][-1])
        raise
    finally:
        destination.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')


if __name__=='__main__':main()
