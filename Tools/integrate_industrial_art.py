"""Unreal editor: bind cosmetic resources and dress existing maps, preserving gameplay geometry."""
import json
import math
from pathlib import Path
import unreal

ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'ArtSource/FPSDemo/Industrial'
DEST='/Game/FPSDemo/Art/Industrial'
lib=unreal.EditorAssetLibrary
tools=unreal.AssetToolsHelpers.get_asset_tools()
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'generator':'Tools/integrate_industrial_art.py','runtime_parts':[],'weapons':[],'maps':[],'success':False}


def load(path):
    asset=unreal.load_asset(path)
    assert asset,path
    return asset


def runtime_parts():
    for entry in json.loads((SOURCE/'RuntimeParts.json').read_text())['assets']:
        task=unreal.AssetImportTask()
        task.filename=str(SOURCE/entry['file']);task.destination_path=DEST+'/Character/Runtime'
        task.destination_name=entry['name'];task.automated=True;task.replace_existing=True;task.save=True
        opt=unreal.FbxImportUI();opt.import_mesh=True;opt.import_materials=False;opt.import_textures=False
        opt.automated_import_should_detect_type=False;opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
        info=opt.static_mesh_import_data;info.combine_meshes=True
        info.convert_scene=False;info.convert_scene_unit=True
        info.set_editor_property('coordinate_system_policy',unreal.CoordinateSystemPolicy.KEEP_XYZ_AXES)
        info.normal_import_method=unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
        task.options=opt;tools.import_asset_tasks([task])
        mesh=load(task.destination_path+'/'+entry['name'])
        for i,slot in enumerate(mesh.static_materials):
            name=str(slot.material_slot_name).split('.')[0]
            mesh.set_material(i,load(DEST+'/Materials/'+name))
        assert lib.save_loaded_asset(mesh,False)
        report['runtime_parts'].append(mesh.get_path_name())


def weapons():
    stats=['damage','fire_interval','magazine_size','initial_magazine','initial_reserve','reload_duration','range','weapon_id']
    for name in ('AK','M4','BR3','MP5','UMP45','M9','HeavyPistol'):
        definition=load('/Game/FPSDemo/Weapons/Definitions/DA_'+name)
        before={k:str(definition.get_editor_property(k)) for k in stats}
        mesh=load(DEST+'/Weapons/SM_IND_'+name)
        for prop in ('first_person','third_person','pickup'):
            presentation=definition.get_editor_property(prop)
            presentation.set_editor_property('mesh',mesh)
            definition.set_editor_property(prop,presentation)
        bounds=mesh.get_bounding_box()
        # Surface sights align with the camera; offsets remain cosmetic data only.
        old_aim=definition.get_editor_property('aim_location')
        definition.set_editor_property('aim_location',unreal.Vector(old_aim.x,0,-bounds.max.z+.5))
        definition.set_editor_property('muzzle_location',unreal.Vector(bounds.max.x-1,0,0))
        after={k:str(definition.get_editor_property(k)) for k in stats}
        assert before==after,'Gameplay weapon values changed'
        assert lib.save_loaded_asset(definition,False)
        report['weapons'].append({'path':definition.get_path_name(),'mesh':mesh.get_path_name(),'gameplay_values_unchanged':True})


def dress_map(name):
    assert levels.load_level('/Game/FPSDemo/Maps/'+name)
    for actor in actors.get_all_level_actors():
        if actor.get_actor_label().startswith('IND_ART_'):actors.destroy_actor(actor)
    originals=list(actors.get_all_level_actors())
    def transform_values(actor):
        t=actor.get_actor_transform()
        return tuple(round(v,6) for v in (t.translation.x,t.translation.y,t.translation.z,
            t.rotation.x,t.rotation.y,t.rotation.z,t.rotation.w,t.scale3d.x,t.scale3d.y,t.scale3d.z))
    before={a.get_path_name():transform_values(a) for a in originals}
    collision_before={a.get_path_name():str(a.static_mesh_component.get_collision_profile_name()) for a in originals if isinstance(a,unreal.StaticMeshActor)}
    count=0
    def shell(kind,center,size,yaw=0):
        nonlocal count
        mesh=load(DEST+'/Environment/SM_IND_'+kind)
        box=mesh.get_bounding_box();dim=box.max-box.min;mid=(box.min+box.max)*.5
        swapped=abs(yaw)==90
        scale=unreal.Vector(size.y/dim.x if swapped else size.x/dim.x,size.x/dim.y if swapped else size.y/dim.y,size.z/dim.z)
        offset=unreal.Vector(mid.x*scale.x,mid.y*scale.y,mid.z*scale.z)
        if swapped:offset=unreal.Vector(-offset.y,offset.x,offset.z)
        actor=actors.spawn_actor_from_class(unreal.StaticMeshActor,center-offset,unreal.Rotator(pitch=0,yaw=yaw,roll=0))
        actor.set_actor_label('IND_ART_'+kind+'_'+str(count));count+=1
        comp=actor.static_mesh_component;comp.set_static_mesh(mesh);comp.set_collision_profile_name('NoCollision')
        # Gameplay keeps its original collision and navigation representation.
        comp.set_editor_property('can_ever_affect_navigation',False)
        actor.set_actor_scale3d(scale)
    for actor in originals:
        label=actor.get_actor_label()
        if isinstance(actor,unreal.StaticMeshActor):
            comp=actor.static_mesh_component
            if label.startswith(('Floor','Wall','SideWall','EndWall','SafetyDivider','TargetBackstop','CenterCover')):
                comp.set_material(0,load(DEST+'/Materials/MI_IND_Concrete'))
            elif label.startswith(('Cargo','WeaponPlinth')):comp.set_material(0,load(DEST+'/Materials/MI_IND_Graphite'))
            elif label.startswith(('LaneGuide','FlankRoute','CenterMark','WallStripe','WeaponStripe','LowCover')):comp.set_material(0,load(DEST+'/Materials/MI_IND_Yellow'))
            kind=None
            if label.startswith('Cargo') and not label.startswith('CargoRib'):kind='Cargo240'
            elif label.startswith(('LowCover','CenterCover','SafetyDivider')):kind='Barrier200'
            elif label.startswith('WallPillar'):kind='Pillar300'
            if kind:
                center,extent=actor.get_actor_bounds(False)
                shell(kind,center,extent*2)
                comp.set_visibility(False);comp.set_cast_shadow(False)
            if label.startswith(('SideWall','EndWall')):
                center,extent=actor.get_actor_bounds(False);size=extent*2
                along_x=size.x>size.y;length=size.x if along_x else size.y
                pieces=math.ceil(length/400);step=length/pieces
                for i in range(pieces):
                    c=unreal.Vector(center.x,center.y,center.z)
                    if along_x:c.x+=-length*.5+step*(i+.5)
                    else:c.y+=-length*.5+step*(i+.5)
                    shell('Wall400',c,unreal.Vector(step,size.y,size.z) if along_x else unreal.Vector(size.x,step,size.z),0 if along_x else 90)
                comp.set_visibility(False);comp.set_cast_shadow(False)
        elif isinstance(actor,unreal.SkyLight):
            c=actor.light_component;c.set_mobility(unreal.ComponentMobility.MOVABLE)
            c.set_editor_property('source_type',unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
            c.set_editor_property('cubemap',load('/Engine/MapTemplates/Sky/DaylightAmbientCubemap'))
            c.set_editor_property('real_time_capture',False);c.set_editor_property('intensity',1.2)
        elif isinstance(actor,unreal.DirectionalLight):
            actor.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
            actor.light_component.set_editor_property('atmosphere_sun_light',True)
        elif isinstance(actor,unreal.TextRenderActor):
            text=str(actor.text_render.get_editor_property('text'))
            if 'MCP' in text:actor.text_render.set_text(text.replace('MCP',''))
    after={a.get_path_name():transform_values(a) for a in originals}
    assert before==after,repr({k:(v,after.get(k)) for k,v in before.items() if v!=after.get(k)})
    assert collision_before=={a.get_path_name():str(a.static_mesh_component.get_collision_profile_name()) for a in originals if isinstance(a,unreal.StaticMeshActor)},'Original collision profile changed'
    assert levels.save_current_level()
    report['maps'].append({'map':name,'cosmetic_shells':count,'original_transforms_and_collision_preserved':True})


try:
    runtime_parts();weapons()
    for name in ('LV_FPSFoundry','LV_FPSTraining'):dress_map(name)
    report['success']=True
    unreal.log('INDUSTRIAL_INTEGRATION PASS')
finally:
    (SOURCE/'IntegrationReport.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
