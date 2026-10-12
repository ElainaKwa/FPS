"""Unreal Editor Python import, validation and independent art-showcase authoring.

Only writes /Game/FPSDemo/Art/Industrial and a task-local report under Saved.
Does not replace gameplay maps, weapon definitions or the runtime avatar.
"""
import json
import unreal
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'ArtSource/FPSDemo/Industrial'
DEST='/Game/FPSDemo/Art/Industrial'
REPORT=ROOT/'Saved/IndustrialArt'
REPORT.mkdir(parents=True,exist_ok=True)
data=json.loads((SOURCE/'manifest.json').read_text(encoding='utf-8'))
lib=unreal.EditorAssetLibrary
tools=unreal.AssetToolsHelpers.get_asset_tools()
mel=unreal.MaterialEditingLibrary
report={'source':str(SOURCE.relative_to(ROOT)),'assets':[],'errors':[]}


def log(message):unreal.log('INDUSTRIAL_ART '+message)


def create_materials():
    path=DEST+'/Materials/M_IND_Surface'
    master=unreal.load_asset(path) if lib.does_asset_exist(path) else tools.create_asset('M_IND_Surface',DEST+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
    # Rebuild this task-owned master deterministically.
    mel.delete_all_material_expressions(master)
    for name,typ,prop,default in [
        ('BaseColor',unreal.MaterialExpressionVectorParameter,unreal.MaterialProperty.MP_BASE_COLOR,None),
        ('Metallic',unreal.MaterialExpressionScalarParameter,unreal.MaterialProperty.MP_METALLIC,.0),
        ('Roughness',unreal.MaterialExpressionScalarParameter,unreal.MaterialProperty.MP_ROUGHNESS,.5),
        ('Emissive',unreal.MaterialExpressionVectorParameter,unreal.MaterialProperty.MP_EMISSIVE_COLOR,None)]:
        node=mel.create_material_expression(master,typ)
        node.set_editor_property('parameter_name',name)
        if default is not None:node.set_editor_property('default_value',default)
        elif name=='Emissive':node.set_editor_property('default_value',unreal.LinearColor(0,0,0,1))
        mel.connect_material_property(node,'',prop)
    mel.recompile_material(master);lib.save_loaded_asset(master)
    mats={}
    for name,settings in data['materials'].items():
        assetname='MI_IND_'+name;path=DEST+'/Materials/'+assetname
        mi=unreal.load_asset(path) if lib.does_asset_exist(path) else tools.create_asset(assetname,DEST+'/Materials',unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(mi,master)
        mel.set_material_instance_vector_parameter_value(mi,'BaseColor',unreal.LinearColor(*settings['color'],1))
        mel.set_material_instance_scalar_parameter_value(mi,'Metallic',settings['metallic'])
        mel.set_material_instance_scalar_parameter_value(mi,'Roughness',settings['roughness'])
        mel.set_material_instance_vector_parameter_value(mi,'Emissive',unreal.LinearColor(*(tuple(v*2 for v in settings['color']) if name=='Light' else (0,0,0)),1))
        mel.update_material_instance(mi);lib.save_loaded_asset(mi)
        mats[assetname]=mi
    return mats


def import_mesh(entry,mats):
    folder=DEST+'/'+entry['category']
    # Initial failed Interchange probe retained its old axis settings on atomic reimport.
    # It has no users yet; rebuild just that task-owned probe before the first showcase.
    if entry['name']=='SM_IND_M4' and not lib.does_asset_exist(DEST+'/Maps/LV_IndustrialShowcase'):
        existing=unreal.load_asset(folder+'/'+entry['name']) if lib.does_asset_exist(folder+'/'+entry['name']) else None
        if existing:
            bb=existing.get_bounding_box(); dim=bb.max-bb.min
            if dim.y>dim.x:
                assert lib.delete_asset(folder+'/'+entry['name'])
    task=unreal.AssetImportTask()
    task.filename=str(SOURCE/entry['file']);task.destination_path=folder;task.destination_name=entry['name']
    task.automated=True;task.replace_existing=True;task.replace_existing_settings=True;task.save=True
    task.factory=unreal.FbxFactory()
    opt=unreal.FbxImportUI()
    sk=entry.get('skeletal',False)
    opt.set_editor_property('automated_import_should_detect_type',False)
    opt.set_editor_property('mesh_type_to_import',unreal.FBXImportType.FBXIT_SKELETAL_MESH if sk else unreal.FBXImportType.FBXIT_STATIC_MESH)
    opt.set_editor_property('import_as_skeletal',sk)
    opt.set_editor_property('import_mesh',True)
    opt.set_editor_property('import_materials',False)
    opt.set_editor_property('import_textures',False)
    opt.set_editor_property('import_animations',False)
    if sk:
        opt.set_editor_property('create_physics_asset',True)
        info=opt.get_editor_property('skeletal_mesh_import_data')
    else:
        info=opt.get_editor_property('static_mesh_import_data')
        info.set_editor_property('combine_meshes',True)
        info.set_editor_property('generate_lightmap_u_vs',True)
        info.set_editor_property('auto_generate_collision',True)
    # Exported vertex coordinates already use X/Y/Z; preserve axes, only convert units.
    info.set_editor_property('convert_scene',False)
    info.set_editor_property('convert_scene_unit',True)
    info.set_editor_property('force_front_x_axis',False)
    info.set_editor_property('coordinate_system_policy',unreal.CoordinateSystemPolicy.KEEP_XYZ_AXES)
    info.set_editor_property('import_uniform_scale',1.0)
    info.set_editor_property('normal_import_method',unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
    task.options=opt
    tools.import_asset_tasks([task])
    path=folder+'/'+entry['name']
    mesh=unreal.load_asset(path)
    expected=unreal.SkeletalMesh if sk else unreal.StaticMesh
    assert isinstance(mesh,expected),f'Wrong import type: {path}, imported={task.imported_object_paths}'
    slots=mesh.get_editor_property('materials' if sk else 'static_materials')
    assigned=[]
    for i,slot in enumerate(slots):
        slotname=str(slot.get_editor_property('material_slot_name'))
        key=next((k for k in mats if slotname.startswith(k)),None)
        assert key,f'Unknown material slot {path} {slotname}'
        if sk:slot.set_editor_property('material_interface',mats[key])
        else:mesh.set_material(i,mats[key])
        assigned.append(key)
    if sk:mesh.set_editor_property('materials',slots)
    item={'path':path,'class':mesh.get_class().get_name(),'materials':assigned}
    if not sk:
        bounds=mesh.get_bounding_box()
        dimensions=bounds.max-bounds.min
        observed=[dimensions.x,dimensions.y,dimensions.z]
        expected=[(b-a)*100 for a,b in zip(entry['bounds_m']['min'],entry['bounds_m']['max'])]
        item.update(dimensions_cm=observed,expected_dimensions_cm=expected)
        assert all(abs(a-b)<max(.2,b*.01) for a,b in zip(observed,expected)),f'Axis/scale mismatch {path}: {observed} vs {expected}'
        body=mesh.get_editor_property('body_setup')
        if entry['category']=='Environment':
            # Static architectural mesh: doorway apertures and railing gaps stay open.
            body.set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        else:
            body.set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AND_COMPLEX)
    else:
        assert mesh.get_editor_property('skeleton'),path+' skeleton missing'
        assert mesh.get_editor_property('physics_asset'),path+' physics asset missing'
    assert lib.save_loaded_asset(mesh,False),path+' save failed'
    item['saved']=True;report['assets'].append(item)
    log('SAVED '+path)
    return mesh


def import_icons():
    icons=sorted((SOURCE/'Icons').glob('*.png'))
    assert len(icons)==7,'Missing generated weapon icons'
    for file in icons:
        task=unreal.AssetImportTask();task.filename=str(file);task.destination_path=DEST+'/UI'
        task.destination_name=file.stem;task.automated=True;task.replace_existing=True;task.save=True
        tools.import_asset_tasks([task])
        tex=unreal.load_asset(DEST+'/UI/'+file.stem)
        assert isinstance(tex,unreal.Texture2D),file.name
        tex.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        tex.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)
        tex.set_editor_property('srgb',True)
        lib.save_loaded_asset(tex)
        report['assets'].append({'path':DEST+'/UI/'+file.stem,'class':'Texture2D','saved':True})


def showcase(meshes):
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    map_path=DEST+'/Maps/LV_IndustrialShowcase'
    # Only regenerate this script's own presentation level; original gameplay maps untouched.
    if lib.does_asset_exist(map_path):
        levels.load_level(map_path)
        for actor in actors.get_all_level_actors():
            if str(actor.get_actor_label()).startswith('IND_'):actors.destroy_actor(actor)
    else:assert levels.new_level(map_path)
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property('default_game_mode',unreal.GameModeBase.static_class())
    def place(name,loc,yaw=0,scale=(1,1,1)):
        actor=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*loc),unreal.Rotator(pitch=0,yaw=yaw,roll=0))
        actor.set_actor_label('IND_'+name)
        comp=actor.static_mesh_component;comp.set_static_mesh(meshes[name]);comp.set_mobility(unreal.ComponentMobility.STATIC)
        actor.set_actor_scale3d(unreal.Vector(*scale))
        comp.set_collision_profile_name('BlockAll' if name in [e['name'] for e in data['assets'] if e['category']=='Environment'] else 'NoCollision')
        return actor
    for x in (-600,-200,200,600):
        for y in (-400,0,400):place('SM_IND_Floor400',(x,y,0))
    for x in (-600,-200,200,600):place('SM_IND_Wall400',(x,600,0))
    for y in (-400,0,400):place('SM_IND_Wall400',(-800,y,0),90)
    place('SM_IND_Gateway400',(600,395,0),180)
    for x in (-600,-200,200):place('SM_IND_Pillar300',(x,580,0),180)
    for loc in [(-510,235,0),(-505,235,138),(-260,335,0)]:place('SM_IND_Cargo240',loc)
    for loc in [(-400,-60,0),(90,270,0)]:place('SM_IND_Barrier200',loc)
    for x in (-600,-200,200):place('SM_IND_TargetStand',(x,470,0),180)
    place('SM_IND_Railing200',(-420,-230,0))
    place('SM_IND_Deck400',(-530,-340,20))
    for loc in [(-650,300,0),(500,490,0)]:place('SM_IND_WorkLight',loc)
    place('SM_IND_BaseBeacon',(-40,570,80),180)
    place('SM_IND_Operator',(75,-10,0),180)
    place('SM_IND_Operator',(390,130,0),155)
    for i,kind in enumerate(('M4','AK','BR3','MP5','UMP45','M9','HeavyPistol')):
        place('SM_IND_'+kind,(-540+i*155,-305,125),0)
        place('SM_IND_WeaponRack',(-540+i*155,-295,0),180)
    light=actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,800),unreal.Rotator(pitch=-45,yaw=-35,roll=0));light.set_actor_label('IND_Sun')
    light.light_component.set_editor_property('intensity',3.5)
    light.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    light.light_component.set_editor_property('atmosphere_sun_light',True)
    sky=actors.spawn_actor_from_class(unreal.SkyLight,unreal.Vector(0,0,800));sky.set_actor_label('IND_Sky')
    sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky.light_component.set_editor_property('intensity',1.6)
    sky.light_component.set_editor_property('real_time_capture',False)
    sky.light_component.set_editor_property('source_type',unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
    sky.light_component.set_editor_property('cubemap',unreal.load_asset('/Engine/MapTemplates/Sky/DaylightAmbientCubemap'))
    atmosphere=actors.spawn_actor_from_class(unreal.SkyAtmosphere,unreal.Vector(0,0,0));atmosphere.set_actor_label('IND_Atmosphere')
    camera=actors.spawn_actor_from_class(unreal.CameraActor,unreal.Vector(1600,-1900,1450),unreal.Rotator(pitch=-28.5,yaw=130.1,roll=0));camera.set_actor_label('IND_OverviewCamera')
    camera.camera_component.set_editor_property('field_of_view',58.0)
    start=actors.spawn_actor_from_class(unreal.PlayerStart,unreal.Vector(930,-1150,180),unreal.Rotator(pitch=-10,yaw=137,roll=0));start.set_actor_label('IND_PlayerStart')
    unreal.EditorLevelLibrary.set_level_viewport_camera_info(camera.get_actor_location(),camera.get_actor_rotation())
    assert levels.save_current_level(),'showcase save failed'
    report['showcase']=map_path
    log('SHOWCASE_SAVED '+map_path)


try:
    mats=create_materials()
    meshes={}
    for entry in data['assets']:meshes[entry['name']]=import_mesh(entry,mats)
    import_icons()
    showcase(meshes)
    lib.save_directory(DEST,only_if_is_dirty=True,recursive=True)
    report['success']=True
    log('COMPLETE assets='+str(len(report['assets'])))
except Exception as exc:
    report['errors'].append(str(exc));report['success']=False
    unreal.log_error('INDUSTRIAL_ART FAILED '+str(exc))
    raise
finally:
    (REPORT/'ImportReport.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
