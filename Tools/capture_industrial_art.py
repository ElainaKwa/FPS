"""Capture the saved Unreal art map with a real renderer; keeps editor alive for ticks."""
import unreal
import time
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'ArtSource/FPSDemo/Industrial/Previews/UnrealShowcase.png'
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level('/Game/FPSDemo/Art/Industrial/Maps/LV_IndustrialShowcase')
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
camera=next(a for a in actors.get_all_level_actors() if a.get_actor_label()=='IND_OverviewCamera')
unreal.EditorLevelLibrary.set_level_viewport_camera_info(camera.get_actor_location(),camera.get_actor_rotation())
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.SystemLibrary.execute_console_command(world,'r.EyeAdaptationQuality 0')
unreal.SystemLibrary.execute_console_command(world,'r.ScreenPercentage 100')
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
started=time.monotonic()
started_wall=time.time()
task=None
requested=False


def tick(delta):
    global task,requested
    elapsed=time.monotonic()-started
    if not requested and elapsed>30:
        # The engine pumps Slate while finishing loads; prevent reentrant requests.
        requested=True
        task=unreal.AutomationLibrary.take_high_res_screenshot(1600,1000,str(OUT),camera=camera,delay=2.0)
        unreal.log('INDUSTRIAL_CAPTURE REQUESTED')
    elif task is not None and task.is_task_done() and OUT.exists() and OUT.stat().st_mtime>started_wall:
        unreal.log('INDUSTRIAL_CAPTURE PASS '+str(OUT))
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.EditorPythonScripting.set_keep_python_script_alive(False)
        unreal.SystemLibrary.quit_editor()
    elif elapsed>180:
        unreal.log_error('INDUSTRIAL_CAPTURE TIMEOUT')
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.SystemLibrary.quit_editor()


handle=unreal.register_slate_post_tick_callback(tick)
