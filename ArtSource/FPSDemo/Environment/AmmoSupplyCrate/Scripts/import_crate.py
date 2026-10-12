"""Import and validate the supply crate in its own Unreal asset folder."""
import unreal,json,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
PROJECT=ROOT.parents[3]
DEST='/Game/FPSDemo/Art/Environment/AmmoSupplyCrate'
MAP=DEST+'/Maps/LV_AmmoSupplyCratePreview'
manifest=json.loads((ROOT/'AssetManifest.json').read_text(encoding='utf-8'))
lib=unreal.EditorAssetLibrary
tools=unreal.AssetToolsHelpers.get_asset_tools()
mel=unreal.MaterialEditingLibrary
unreal.load_module('StaticMeshEditor')
sm=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.StaticMeshEditorSubsystem()
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report=dict(generated_by='Scripts/import_crate.py',version=manifest['version'],success=False,errors=[])
try:
    # Reuse the existing project-wide master; do not copy or modify it.
    master_path='/Game/FPSDemo/Art/Industrial/Materials/M_IND_Surface'
    master=unreal.load_asset(master_path);assert isinstance(master,unreal.Material),master_path
    mats={}
    for key,values in manifest['materials'].items():
        name='MI_AMC_'+key;path=DEST+'/Materials/'+name
        mi=unreal.load_asset(path) if lib.does_asset_exist(path) else tools.create_asset(name,DEST+'/Materials',unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(mi,master)
        mel.set_material_instance_vector_parameter_value(mi,'BaseColor',unreal.LinearColor(*values['color'],1))
        mel.set_material_instance_scalar_parameter_value(mi,'Metallic',values['metallic'])
        mel.set_material_instance_scalar_parameter_value(mi,'Roughness',values['roughness'])
        mel.set_material_instance_vector_parameter_value(mi,'Emissive',unreal.LinearColor(0,0,0,1))
        mel.update_material_instance(mi);assert lib.save_loaded_asset(mi);mats[name]=mi
    task=unreal.AssetImportTask();task.filename=str(ROOT/manifest['fbx'])
    task.destination_path=DEST+'/Meshes';task.destination_name='SM_AmmoSupplyCrate'
    task.automated=True;task.replace_existing=True;task.replace_existing_settings=True;task.save=True
    task.factory=unreal.FbxFactory()
    opt=unreal.FbxImportUI();opt.automated_import_should_detect_type=False
    opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
    opt.import_as_skeletal=False;opt.import_mesh=True;opt.import_materials=False;opt.import_textures=False;opt.import_animations=False
    info=opt.static_mesh_import_data
    info.combine_meshes=True;info.generate_lightmap_u_vs=False;info.auto_generate_collision=False
    info.convert_scene=False;info.convert_scene_unit=True;info.force_front_x_axis=False
    info.set_editor_property('coordinate_system_policy',unreal.CoordinateSystemPolicy.KEEP_XYZ_AXES)
    info.import_uniform_scale=1.0;info.normal_import_method=unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
    task.options=opt;tools.import_asset_tasks([task])
    mesh_path=DEST+'/Meshes/SM_AmmoSupplyCrate';mesh=unreal.load_asset(mesh_path)
    assert isinstance(mesh,unreal.StaticMesh),task.imported_object_paths
    assigned=[]
    for index,slot in enumerate(mesh.get_editor_property('static_materials')):
        key=str(slot.material_slot_name);assert key in mats,key
        mesh.set_material(index,mats[key]);assigned.append(key)
    assert len(assigned)==6,assigned
    sm.remove_collisions(mesh)
    assert sm.add_simple_collisions(mesh,unreal.ScriptCollisionShapeType.BOX)>=0
    body=mesh.get_editor_property('body_setup');body.set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AND_COMPLEX)
    options=unreal.StaticMeshReductionOptions()
    options.auto_compute_lod_screen_size=False
    options.reduction_settings=[unreal.StaticMeshReductionSettings(percent_triangles=p,screen_size=s) for p,s in [(1,1),(.5,.35),(.20,.12)]]
    assert sm.set_lods(mesh,options)==3
    mesh.set_editor_property('light_map_resolution',128)
    mesh.set_editor_property('light_map_coordinate_index',1)
    assert lib.save_loaded_asset(mesh)
    bounds=mesh.get_bounding_box();observed=[bounds.max.x-bounds.min.x,bounds.max.y-bounds.min.y,bounds.max.z-bounds.min.z]
    expected=[(manifest['bounds_m'][1][i]-manifest['bounds_m'][0][i])*100 for i in range(3)]
    assert max(abs(a-b) for a,b in zip(observed,expected))<.1,(observed,expected)
    assert abs(bounds.min.z)<.01,bounds
    assert sm.get_simple_collision_count(mesh)==1
    assert sm.get_num_uv_channels(mesh,0)>=2
    lod_verts=[sm.get_number_verts(mesh,i) for i in range(sm.get_lod_count(mesh))]
    assert lod_verts[0]>lod_verts[1]>lod_verts[2]>0,lod_verts
    report.update(mesh=mesh_path,materials=assigned,shared_master=master_path,
                  bounds_cm=observed,bottom_z_cm=bounds.min.z,simple_collisions=1,lods=3,lod_vertices=lod_verts,
                  uv_channels=sm.get_num_uv_channels(mesh,0),preview_map=MAP)

    # Dedicated placement level; never alter the user's existing gameplay levels.
    if lib.does_asset_exist(MAP):
        assert levels.load_level(MAP)
        for actor in actors.get_all_level_actors():
            if actor.get_actor_label().startswith('AMC_'):actors.destroy_actor(actor)
    else:assert levels.new_level(MAP)
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property('default_game_mode',unreal.GameModeBase.static_class())
    def place(label,asset,loc,scale=(1,1,1),yaw=0):
        a=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*loc),unreal.Rotator(pitch=0,yaw=yaw,roll=0))
        a.set_actor_label(label);a.static_mesh_component.set_static_mesh(asset)
        a.static_mesh_component.set_mobility(unreal.ComponentMobility.STATIC)
        a.static_mesh_component.set_collision_profile_name('BlockAll')
        a.set_actor_scale3d(unreal.Vector(*scale));return a
    crate=place('AMC_SupplyCrate',mesh,(0,0,0))
    test=place('AMC_TransformTest',mesh,(300,0,0),(1.25,1.25,1.25),90)
    assert test.get_actor_scale3d().x==1.25
    assert abs(test.get_actor_rotation().yaw-90)<.01
    actors.destroy_actor(test)
    hit=unreal.SystemLibrary.line_trace_single(world,unreal.Vector(0,-160,20),unreal.Vector(0,0,20),unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[],unreal.DrawDebugTrace.NONE,False)
    assert hit is not None,'No simple collision hit on placed crate'
    hit_values=hit.to_tuple();assert hit_values[0],str(hit_values)
    report['placement_checks']={'spawned_static_mesh_actor':True,'rotation_scale':True,'simple_collision_trace':str(hit_values)}
    floor=place('AMC_StudioFloor',unreal.load_asset('/Engine/BasicShapes/Cube'),(0,0,-2),(20,20,.04))
    floor.static_mesh_component.set_material(0,mats['MI_AMC_Panel'])
    light=actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,400),unreal.Rotator(pitch=-48,yaw=-125,roll=0));light.set_actor_label('AMC_Key')
    light.light_component.set_mobility(unreal.ComponentMobility.MOVABLE);light.light_component.set_editor_property('intensity',5.0)
    sky=actors.spawn_actor_from_class(unreal.SkyLight,unreal.Vector(0,0,300));sky.set_actor_label('AMC_Fill')
    sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky.light_component.set_editor_property('source_type',unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
    sky.light_component.set_editor_property('cubemap',unreal.load_asset('/Engine/MapTemplates/Sky/DaylightAmbientCubemap'))
    sky.light_component.set_editor_property('intensity',1.6)
    fill=actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,300),unreal.Rotator(pitch=-30,yaw=-35,roll=0));fill.set_actor_label('AMC_StudioFill')
    fill.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    fill.light_component.set_editor_property('intensity',1.2)
    fill.light_component.set_editor_property('cast_shadows',False)
    # FBX handedness conversion mirrors Y: Blender -Y becomes Unreal +Y.
    camera=actors.spawn_actor_from_class(unreal.CameraActor,unreal.Vector(110,155,103));camera.set_actor_label('AMC_PreviewCamera')
    camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(camera.get_actor_location(),unreal.Vector(0,0,21)),False)
    camera.camera_component.set_editor_property('field_of_view',42.0)
    camera.camera_component.set_editor_property('aspect_ratio',4.0/3.0)
    unreal.EditorLevelLibrary.set_level_viewport_camera_info(camera.get_actor_location(),camera.get_actor_rotation())
    assert levels.save_current_level()
    assert levels.load_level(MAP)
    reloaded=next(a for a in actors.get_all_level_actors() if a.get_actor_label()=='AMC_SupplyCrate')
    assert reloaded.static_mesh_component.static_mesh.get_path_name()==mesh.get_path_name()
    report['placement_checks']['saved_level_reopened']=True
    lib.save_directory(DEST,only_if_is_dirty=True,recursive=True)
    report['success']=True;unreal.log('AMMO_CRATE_IMPORT_PASS '+mesh_path)
except Exception as exc:
    report['errors'].append(str(exc));raise
finally:
    (ROOT/'UnrealValidation.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
