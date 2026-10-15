"""Run once through Unreal's PythonScript commandlet; preserve existing explicit profiles."""
import unreal

path = '/Game/FPSDemo/Weapons/Damage/DA_HitDamage_Default'
profile = unreal.EditorAssetLibrary.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
if profile is None:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', unreal.FPSHitDamageProfile)
    profile = unreal.AssetToolsHelpers.get_asset_tools().create_asset('DA_HitDamage_Default', '/Game/FPSDemo/Weapons/Damage', unreal.FPSHitDamageProfile, factory)
    profile.set_editor_property('profile_id', 'HitDamage.Default')
    assert unreal.EditorAssetLibrary.save_loaded_asset(profile)

registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)
assets = registry.get_assets_by_class(unreal.TopLevelAssetPath('/Script/MCP_Test', 'FPSWeaponDefinition'), True)
changed = []
for asset in assets:
    weapon = asset.get_asset()
    if weapon.get_editor_property('hit_damage_profile') is None:
        weapon.set_editor_property('hit_damage_profile', profile)
        assert unreal.EditorAssetLibrary.save_loaded_asset(weapon)
        changed.append(str(weapon.get_path_name()))
unreal.log('FPS_HIT_MIGRATION PASS assigned=' + str(len(changed)) + ' definitions=' + str(len(assets)) + ' paths=' + str(changed))
