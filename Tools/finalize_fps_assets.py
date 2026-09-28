"""One-time Unreal Editor automation: save reusable Blueprints and place the heal station.

Run in UE's Python Editor Script Plugin, not with system Python.
"""

from pathlib import Path

import unreal


CHAR_BP = "/Game/FPSDemo/Blueprints/BP_FPS_Character"
HEAL_BP = "/Game/FPSDemo/Blueprints/BP_FPS_HealStation"
MAP = "/Game/Maps/LV_NetSyncFPS"


def ensure_blueprint(path, parent_path):
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        asset = unreal.EditorAssetLibrary.load_asset(path)
    else:
        parent = unreal.load_class(None, parent_path)
        if not parent:
            raise RuntimeError("Missing C++ class: " + parent_path)
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", parent)
        folder, name = path.rsplit("/", 1)
        asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.Blueprint, factory)
    if not asset:
        raise RuntimeError("Failed to create " + path)
    unreal.EditorAssetLibrary.save_asset(path)
    unreal.log("FPS ASSET: " + path)
    return asset


ensure_blueprint(CHAR_BP, "/Script/MCP_Test.FPSDemoCharacter")
ensure_blueprint(HEAL_BP, "/Script/MCP_Test.FPSDemoHealStation")

# Keep Unreal's Reimport command pointed at the saved, editable OBJ sources.
source_dir = Path(__file__).resolve().parents[1] / "ArtSource" / "FPSDemo"
for name in ("SM_FPS_Rifle", "SM_FPS_HealStation"):
    asset_path = "/Game/FPSDemo/Meshes/" + name
    mesh = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not mesh:
        raise RuntimeError("Missing saved mesh: " + asset_path)
    mesh.get_editor_property("asset_import_data").scripted_add_filename(str(source_dir / (name + ".obj")), 0, "")
    unreal.EditorAssetLibrary.save_asset(asset_path)
    unreal.log("FPS SOURCE: " + name + " -> " + str(source_dir / (name + ".obj")))

world = unreal.EditorLoadingAndSavingUtils.load_map(MAP)
if not world:
    raise RuntimeError("Failed to load " + MAP)

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
existing = [a for a in actors.get_all_level_actors() if a.get_actor_label() == "FPS_Heal_Station"]
if not existing:
    station_class = unreal.load_class(None, HEAL_BP + ".BP_FPS_HealStation_C")
    if not station_class:
        raise RuntimeError("Heal station blueprint generated class missing")
    station = actors.spawn_actor_from_class(station_class, unreal.Vector(0, 0, 0))
    if not station:
        raise RuntimeError("Failed to place station")
    station.set_actor_label("FPS_Heal_Station")
    unreal.log("FPS STATION: placed at 0,0,0")
else:
    unreal.log("FPS STATION: already placed")

if not unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True):
    raise RuntimeError("Failed to save map or blueprints")
unreal.log("FPS MAP: saved " + MAP)
