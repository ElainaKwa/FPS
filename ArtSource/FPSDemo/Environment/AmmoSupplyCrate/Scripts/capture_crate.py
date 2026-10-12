"""Capture the saved Unreal placement level with real rendering."""
import unreal,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'Previews/UnrealPlacement.png'
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level('/Game/FPSDemo/Art/Environment/AmmoSupplyCrate/Maps/LV_AmmoSupplyCratePreview')
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
camera=next(a for a in actors.get_all_level_actors() if a.get_actor_label()=='AMC_PreviewCamera')
unreal.EditorLevelLibrary.set_level_viewport_camera_info(camera.get_actor_location(),camera.get_actor_rotation())
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
for cmd in ['sg.ViewDistanceQuality 3','sg.AntiAliasingQuality 3','sg.ShadowQuality 3',
            'sg.GlobalIlluminationQuality 3','sg.ReflectionQuality 3','sg.PostProcessQuality 3',
            'sg.TextureQuality 3','sg.EffectsQuality 3','sg.ShadingQuality 3',
            'r.EyeAdaptationQuality 0','r.ScreenPercentage 100']:
    unreal.SystemLibrary.execute_console_command(world,cmd)
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
started=time.monotonic();wall=time.time();task=None;requested=False;finished=None
def tick(delta):
    global task,requested,finished
    if not requested and time.monotonic()-started>25:
        requested=True
        task=unreal.AutomationLibrary.take_high_res_screenshot(1600,1200,str(OUT),camera=camera,delay=2.0)
    elif finished is None and task is not None and task.is_task_done() and OUT.exists() and OUT.stat().st_mtime>wall:
        unreal.log('AMMO_CRATE_CAPTURE_PASS '+str(OUT))
        finished=time.monotonic()
    elif finished is not None and time.monotonic()-finished>4:
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.EditorPythonScripting.set_keep_python_script_alive(False);unreal.SystemLibrary.quit_editor()
    elif time.monotonic()-started>180:
        unreal.log_error('AMMO_CRATE_CAPTURE_TIMEOUT');unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor()
handle=unreal.register_slate_post_tick_callback(tick)
