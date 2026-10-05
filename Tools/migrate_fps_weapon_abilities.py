import unreal
ids = ('AK', 'M4', 'MP5', 'UMP45', 'M9', 'HeavyPistol')
for name in ids:
    path = '/Game/FPSDemo/Weapons/Definitions/DA_' + name
    asset = unreal.load_asset(path)
    assert asset, path
    asset.set_editor_property('fire_mode', unreal.FPSFireMode.SINGLE if name in ('M9', 'HeavyPistol') else unreal.FPSFireMode.AUTOMATIC)
    asset.set_editor_property('burst_count', 3)
    asset.set_editor_property('burst_interval', .3)
    asset.set_editor_property('recoil_recovery_time', .5)
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset, False), path
unreal.log('FPS_WEAPON_STAGE2_ASSETS PASS six saved definitions')
