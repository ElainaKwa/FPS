"""M7 renderer check, prepared for a separate UE editor process.

Do not use -nullrhi or -run=pythonscript for screenshots. Example launch:
UnrealEditor-Cmd PROJECT -ExecutePythonScript=THIS_SCRIPT -RenderOffscreen
    -unattended -nop4 -nosound -windowed -ResX=2200 -ResY=1200 -M7PreviewQuit

Writes only the M7-owned preview level if save_level=True, plus Saved evidence.
From live MCP, import this module and call build_preview(); begin_capture()
only after checking the current level has no unsaved user edits.
"""
import json
import math
import time
from pathlib import Path

import unreal


ROOT = Path(unreal.Paths.project_dir()).resolve()
OUT = ROOT / "Saved/M7UnrealImport/Previews"
CORE = "/Game/FPSDemo/Art/Weapons/Rifles/M7"
MAP = CORE + "/Maps/LV_M7Preview"
OWNER_KEY = "m7_import_owner"
OWNER = "M7UnrealImport"
PREFIX = "M7UE_"
CAMERAS = []
TICK_HANDLE = None
CAPTURE_STATE = None


def _path_owned(path):
    lib = unreal.EditorAssetLibrary
    if not lib.does_asset_exist(path):
        return False
    asset = unreal.load_asset(path)
    if asset is None or str(lib.get_metadata_tag(asset, OWNER_KEY)) != OWNER:
        raise RuntimeError("Refusing to replace an unrelated preview map: " + path)
    return True


def _camera(actors, name, location, width):
    # Side camera sees the +X muzzle direction from negative Y; frame is upright.
    camera = actors.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(*location),
                                          unreal.Rotator(pitch=0, yaw=90, roll=0))
    camera.set_actor_label(PREFIX + name)
    component = camera.camera_component
    # A long lens keeps an almost planar side view while using the perspective
    # reflection path supported by both SM5 and SM6 editor renderers.
    distance = abs(location[1])
    component.set_editor_property("projection_mode", unreal.CameraProjectionMode.PERSPECTIVE)
    component.set_editor_property("field_of_view", math.degrees(2 * math.atan(width / (2 * distance))))
    component.set_editor_property("aspect_ratio", 2200 / 1200)
    post = component.get_editor_property("post_process_settings")
    post.set_editor_property("override_auto_exposure_method", True)
    post.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_MANUAL)
    post.set_editor_property("override_auto_exposure_apply_physical_camera_exposure", True)
    post.set_editor_property("auto_exposure_apply_physical_camera_exposure", False)
    post.set_editor_property("override_auto_exposure_bias", True)
    post.set_editor_property("auto_exposure_bias", 0.0)
    component.set_editor_property("post_process_settings", post)
    component.set_editor_property("post_process_blend_weight", 1.0)
    return camera


def build_preview(save_level=True):
    """Create a dedicated level; intended for a new editor process, not user maps."""
    report_path = ROOT / "ArtSource/FPSDemo/Samples/Weapons/M7/UnrealImportReport.json"
    report = json.loads(report_path.read_text(encoding="utf-8"))
    if not report.get("success"):
        raise RuntimeError("Refusing to preview an incomplete M7 import")
    entries = {item["name"]: item for item in report["assets"]}
    specs = [
        ("Default", "SM_M7_Default_Assembled", 0.0),
        ("Tidal", "SM_M7_Equipped_Tidal_Assembled", 220.0),
        ("Standard", "SM_M7_Equipped_Standard_Assembled", 440.0),
    ]
    # Match actual manifest preset naming rather than silently omitting a rifle.
    manifest = json.loads((report_path.parent / "AssetManifest.json").read_text(encoding="utf-8"))
    preset_names = [row["name"] for row in manifest["presets"]]
    resolved = []
    for label, candidate, x in specs:
        if candidate not in entries:
            matches = [name for name in preset_names if label.lower() in name.lower()]
            if len(matches) != 1:
                raise RuntimeError("Cannot resolve M7 preview preset: " + label)
            candidate = matches[0]
        resolved.append((label, entries[candidate], x))
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if _path_owned(MAP):
        if not levels.load_level(MAP):
            raise RuntimeError("Cannot load owned M7 preview level")
        for actor in actors.get_all_level_actors():
            if str(actor.get_actor_label()).startswith(PREFIX):
                actors.destroy_actor(actor)
    else:
        if not levels.new_level(MAP):
            raise RuntimeError("Cannot create dedicated M7 preview level")
    CAMERAS.clear()
    for label, entry, offset_x in resolved:
        mesh = unreal.load_asset(entry["path"])
        if not isinstance(mesh, unreal.StaticMesh):
            raise RuntimeError("Imported M7 preset missing: " + entry["path"])
        actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(offset_x, 0, 120))
        actor.set_actor_label(PREFIX + label)
        actor.static_mesh_component.set_static_mesh(mesh)
        actor.static_mesh_component.set_collision_profile_name("NoCollision")
        actor.static_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)
        bounds = mesh.get_bounding_box()
        center_x = offset_x + .5 * (bounds.min.x + bounds.max.x)
        center_z = 120 + .5 * (bounds.min.z + bounds.max.z)
        width = max(110, (bounds.max.x - bounds.min.x) * 1.12)
        camera = _camera(actors, label + "_Side", (center_x, -900, center_z), width)
        CAMERAS.append((label + "_Side", camera, [2200, 1200]))
        if label == "Tidal":
            detail = _camera(actors, "Receiver_Detail", (offset_x - 3, -900, 112), 60)
            CAMERAS.append(("Receiver_Detail", detail, [2200, 1200]))
        panel_position = unreal.Vector(offset_x, -90, 210)
        panel_target = unreal.Vector(offset_x, 0, 120)
        panel = actors.spawn_actor_from_class(unreal.RectLight, panel_position,
                    unreal.MathLibrary.find_look_at_rotation(panel_position, panel_target))
        panel.set_actor_label(PREFIX + label + "_Softbox")
        panel.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
        panel.light_component.set_editor_property("intensity_units", unreal.LightUnits.LUMENS)
        panel.light_component.set_editor_property("intensity", 15.0)
        panel.light_component.set_editor_property("source_width", 150.0)
        panel.light_component.set_editor_property("source_height", 90.0)
        panel.light_component.set_editor_property("attenuation_radius", 500.0)
    # Two directional sources and a fixed studio cubemap give black materials
    # enough reflected light to reveal normal maps and roughness variation.
    for name, rotation, intensity in [
        ("Key", (-35, 35, 0), 3.0),
        ("Rim", (-25, -145, 0), 2.0),
    ]:
        light = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 300),
                                             unreal.Rotator(pitch=rotation[0], yaw=rotation[1], roll=rotation[2]))
        light.set_actor_label(PREFIX + name)
        light.light_component.set_editor_property("intensity", intensity)
        light.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 300))
    sky.set_actor_label(PREFIX + "Sky")
    sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky.light_component.set_editor_property("source_type", unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
    sky.light_component.set_editor_property("cubemap", unreal.load_asset("/Engine/MapTemplates/Sky/DaylightAmbientCubemap"))
    sky.light_component.set_editor_property("intensity", 0.2)
    sky.light_component.recapture_sky()
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    unreal.EditorAssetLibrary.set_metadata_tag(world, OWNER_KEY, OWNER)
    if save_level and not levels.save_current_level():
        raise RuntimeError("Failed to save dedicated M7 preview level")
    if CAMERAS:
        camera = CAMERAS[1][1]
        unreal.EditorLevelLibrary.set_level_viewport_camera_info(camera.get_actor_location(), camera.get_actor_rotation())
    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / "PreviewScene.json").write_text(json.dumps({
        "map": MAP, "map_saved": save_level,
        "kind": "Unreal renderer evidence; art preview only", "gameplay_changed": False,
        "cameras": [name for name, _, _ in CAMERAS],
        "source_import_generated_utc": report.get("generated_utc"),
    }, indent=2), encoding="utf-8")
    unreal.log("M7_UE_PREVIEW_SCENE_READY " + MAP)
    return CAMERAS


def _finish_capture(success, error=None):
    global TICK_HANDLE, CAPTURE_STATE
    state = CAPTURE_STATE
    if TICK_HANDLE is not None:
        unreal.unregister_slate_post_tick_callback(TICK_HANDLE)
        TICK_HANDLE = None
    result = {"success": success, "renderer": "Unreal Editor RHI",
              "screenshots": state["completed"], "error": error}
    (OUT / "CaptureReport.json").write_text(json.dumps(result, indent=2), encoding="utf-8")
    unreal.EditorPythonScripting.set_keep_python_script_alive(not state['quit_after'])
    if success:
        unreal.log("M7_UE_CAPTURE_COMPLETE " + json.dumps(result))
    else:
        unreal.log_error("M7_UE_CAPTURE_FAILED " + str(error))
    if state["quit_after"]:
        unreal.SystemLibrary.quit_editor()


def _tick(delta):
    state = CAPTURE_STATE
    try:
        now = time.monotonic()
        if now - state["start"] > 300:
            _finish_capture(False, "Renderer screenshot timeout")
            return
        if state["task"] is not None:
            path = state["path"]
            if state["task"].is_task_done() and path.is_file() and path.stat().st_mtime >= state["requested_wall"]:
                state["completed"].append({"name": state["name"], "path": str(path), "bytes": path.stat().st_size})
                state["task"] = None
                state["index"] += 1
                state["next"] = now + 3
            return
        if state["index"] == len(CAMERAS):
            _finish_capture(True)
            return
        if now < state["next"]:
            return
        name, camera, resolution = CAMERAS[state["index"]]
        path = OUT / ("UE_M7_" + name + ".png")
        state.update(name=name, path=path, requested_wall=time.time())
        # Guard reentrancy before calling a screenshot API that pumps the editor.
        state["next"] = float("inf")
        state["task"] = unreal.AutomationLibrary.take_high_res_screenshot(*resolution, str(path), camera=camera, delay=2.0)
        unreal.log("M7_UE_CAPTURE_REQUESTED " + str(path))
    except Exception as exc:
        _finish_capture(False, repr(exc))


def begin_capture(quit_after=False):
    global TICK_HANDLE, CAPTURE_STATE
    if not CAMERAS or TICK_HANDLE is not None:
        raise RuntimeError("Build the M7 preview first; capture cannot overlap")
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    for command in ("r.ScreenPercentage 100", "r.EyeAdaptationQuality 0", "r.Streaming.FullyLoadUsedTextures 1"):
        unreal.SystemLibrary.execute_console_command(world, command)
    unreal.EditorPythonScripting.set_keep_python_script_alive(True)
    started = time.monotonic()
    CAPTURE_STATE = {"start": started, "next": started + 35, "index": 0,
                     "task": None, "completed": [], "quit_after": quit_after}
    TICK_HANDLE = unreal.register_slate_post_tick_callback(_tick)


if __name__ == "__main__":
    build_preview()
    begin_capture(quit_after="-M7PreviewQuit" in unreal.SystemLibrary.get_command_line())
