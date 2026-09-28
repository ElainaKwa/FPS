"""Repair the newly authored FPS maps using named Rotator arguments."""
import unreal
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for name in ['LV_FPSFoundry','LV_FPSTraining']:
    assert levels.load_level('/Game/FPSDemo/Maps/'+name)
    for a in actors.get_all_level_actors():
        label=a.get_actor_label()
        if isinstance(a,(unreal.PlayerStart,unreal.TargetPoint)):
            tags=[str(t) for t in a.tags]
            red=any(t.startswith('FPSRed') or t.startswith('FPSTarget') for t in tags)
            rack=any(t.startswith('FPSWeapon') for t in tags)
            yaw=180 if red or (rack and name=='LV_FPSFoundry' and a.get_actor_location().x>0) else 0
            a.set_actor_rotation(unreal.Rotator(pitch=0,yaw=yaw,roll=0),False)
        elif isinstance(a,unreal.DirectionalLight):
            a.set_actor_rotation(unreal.Rotator(pitch=-55,yaw=-35,roll=0),False)
        elif isinstance(a,unreal.TextRenderActor):
            yaw=180
            if 'BLUE BASE' in label or 'RANGE / SOLO' in label or 'F : PICK' in label: yaw=0
            if 'FOUNDRY /' in label: yaw=-90
            if '05:00 /' in label: yaw=90
            a.set_actor_rotation(unreal.Rotator(pitch=0,yaw=yaw,roll=0),False)
        elif label=='FiringLine':
            a.set_actor_location(unreal.Vector(-1900,0,1),False,False)
    assert levels.save_current_level()
    unreal.log('FPS_MAP_ROTATION_FIXED '+name)
