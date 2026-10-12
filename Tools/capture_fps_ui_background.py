"""Capture the actual Foundry level for the lobby asset; never saves the level."""
from pathlib import Path
import time
import unreal
ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'ArtSource/FPSDemo/Industrial/UI/Foundry.png'
OUT.parent.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level('/Game/FPSDemo/Maps/LV_FPSFoundry')
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
camera = actors.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(-1780,-1080,470), unreal.Rotator(pitch=-8,yaw=26,roll=0))
camera.camera_component.set_field_of_view(78)
camera.camera_component.set_constraint_aspect_ratio(False)
unreal.EditorLevelLibrary.set_level_viewport_camera_info(camera.get_actor_location(),camera.get_actor_rotation())
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
start = time.monotonic()
task = None
requested = False
def tick(delta):
    global task, requested
    if not requested and time.monotonic()-start>25:
        requested=True
        task=unreal.AutomationLibrary.take_high_res_screenshot(1720,540,str(OUT),camera=camera,delay=2)
    elif task and task.is_task_done() and OUT.exists():
        unreal.log('FPS_UI_BACKGROUND PASS')
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.EditorPythonScripting.set_keep_python_script_alive(False)
        unreal.SystemLibrary.quit_editor()
    elif time.monotonic()-start>150:
        unreal.log_error('FPS_UI_BACKGROUND TIMEOUT')
        unreal.SystemLibrary.quit_editor()
handle=unreal.register_slate_post_tick_callback(tick)
