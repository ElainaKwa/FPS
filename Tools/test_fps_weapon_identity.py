"""Run in UnrealEditor-Cmd -run=pythonscript. Only creates/deletes its own fixture directory."""
import unreal
import pathlib

ROOT = '/Game/__FPSWeaponIdentityTest'
assets = unreal.EditorAssetLibrary
levels = unreal.EditorLoadingAndSavingUtils
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert not assets.list_assets(ROOT, recursive=True, include_folder=False), 'Refusing to overwrite existing fixture assets'
original = ROOT + '/Original'
moved = ROOT + '/Moved/Renamed'
map_path = ROOT + '/IdentityMap'
created = False
try:
    definition = assets.duplicate_asset('/Game/FPSDemo/Weapons/Definitions/DA_M4', original)
    assert definition
    created = True
    definition.set_editor_property('weapon_id', 'Weapon.IdentityFixture')
    assert assets.save_loaded_asset(definition)
    stable_id = unreal.SystemLibrary.get_primary_asset_id_from_object(definition)
    assert stable_id.to_string() == 'WeaponDefinition:Weapon.IdentityFixture', str(stable_id)
    world = levels.new_blank_map(False)
    stand = actors.spawn_actor_from_class(unreal.FPSWeaponStand, unreal.Vector(10,20,30))
    stand.set_actor_label('IdentityStand')
    stand.set_editor_property('definition', definition)
    stand.set_editor_property('display_order', 4)
    assert levels.save_map(world, map_path)
    assert assets.rename_asset(original, moved)
    definition.set_editor_property('display_name', 'Renamed display')
    stand.set_editor_property('display_order', 999)
    assert assets.save_loaded_asset(definition)
    assert levels.save_map(world, map_path)
    levels.new_blank_map(False)
    world = levels.load_map(map_path)
    stands = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.FPSWeaponStand)]
    assert len(stands) == 1
    saved = stands[0].get_editor_property('definition')
    assert saved.get_path_name() == moved + '.Renamed', saved.get_path_name()
    assert unreal.SystemLibrary.get_primary_asset_id_from_object(saved) == stable_id
    assert unreal.SystemLibrary.get_object_from_primary_asset_id(stable_id) == saved
    assert str(saved.get_editor_property('weapon_id')) == 'Weapon.IdentityFixture'
    assert stands[0].get_editor_property('display_order') == 999
    unreal.log('FPS_WEAPON_IDENTITY PASS saved rename move display reorder and map reload preserve explicit reference and primary id')
finally:
    levels.new_blank_map(False)
    if created:
        # UE may return false for an empty directory even after successfully deleting all packages.
        assets.delete_directory(ROOT)
        assert not assets.list_assets(ROOT, recursive=True, include_folder=False), 'Fixture assets remain'
        # Commandlet package deletion can leave orphan files after removing registry entries.
        base = pathlib.Path(unreal.Paths.project_content_dir()).resolve() / '__FPSWeaponIdentityTest'
        for relative in ('IdentityMap.umap', 'Original.uasset', 'Moved/Renamed.uasset'):
            path = (base / relative).resolve()
            assert path.is_relative_to(base.resolve())
            if path.exists():
                path.unlink()
        unreal.log('FPS_WEAPON_IDENTITY PASS fixture cleaned through editor asset tools')
