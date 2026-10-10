"""Add stage-one mode assets/regions through Unreal Editor APIs; never recreate existing levels.

Run with UnrealEditor-Cmd -run=pythonscript -script=<this file> after the C++ build.
Existing rules/loadouts/setups/regions are preserved and validated, not reset.
"""
import unreal

assets = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
root = '/Game/FPSDemo/Modes'

def new_asset(name, cls):
    path = root + '/' + name
    if assets.does_asset_exist(path):
        return unreal.load_asset(path), False
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', cls)
    asset = tools.create_asset(name, root, cls, factory)
    assert asset, path
    return asset, True

loadout, created = new_asset('DA_DefaultSpawnLoadout', unreal.FPSSpawnLoadout)
if created:
    loadout.set_editor_property('primary', unreal.load_asset('/Game/FPSDemo/Weapons/Definitions/DA_M4'))
    loadout.set_editor_property('secondary', unreal.load_asset('/Game/FPSDemo/Weapons/Definitions/DA_M9'))
    assert assets.save_loaded_asset(loadout)

for training, map_name, asset_name in [(False, 'LV_FPSFoundry', 'DA_TeamMatch'), (True, 'LV_FPSTraining', 'DA_Training')]:
    rules, created = new_asset(asset_name, unreal.FPSModeRules)
    if created:
        rules.set_editor_property('kind', unreal.FPSGameplayKind.TRAINING if training else unreal.FPSGameplayKind.TEAM_MATCH)
        rules.set_editor_property('default_loadout', loadout)
        rules.set_editor_property('selectable_weapons', [unreal.load_asset('/Game/FPSDemo/Weapons/Definitions/DA_' + name)
            for name in ['AK', 'M4', 'MP5', 'UMP45', 'M9', 'HeavyPistol', 'BR3']])
        assert all(str(ref) for ref in rules.get_editor_property('selectable_weapons'))
        rules.set_editor_property('selection', unreal.FPSSelectionPolicy.ALWAYS if training else unreal.FPSSelectionPolicy.UNTIL_FIRST_SPAWN_REGION_EXIT)
        rules.set_editor_property('replacement', unreal.FPSReplacementPolicy.DROP if training else unreal.FPSReplacementPolicy.RECYCLE)
        rules.set_editor_property('death_drop', unreal.FPSDeathDropPolicy.NONE)
        rules.set_editor_property('dynamic_drop_limit', 30 if training else -1)
        assert assets.save_loaded_asset(rules)
    assert levels.load_level('/Game/FPSDemo/Maps/' + map_name)
    changed = False
    existing = actors.get_all_level_actors()
    setups = [a for a in existing if isinstance(a, unreal.FPSModeSetup)]
    assert len(setups) <= 1, 'Multiple mode setups require manual repair'
    if not setups:
        setup = actors.spawn_actor_from_class(unreal.FPSModeSetup, unreal.Vector(0,0,0))
        setup.set_actor_label('FPS_ModeRules')
        setup.set_editor_property('rules', rules)
        changed = True
    else:
        assert setups[0].get_editor_property('rules').get_path_name() == rules.get_path_name()
    if not training:
        starts = [a for a in existing if isinstance(a, unreal.PlayerStart)]
        for team, color, x in [(0,'Blue',-2050), (1,'Red',2050)]:
            regions = [a for a in existing if isinstance(a, unreal.FPSBaseRegion) and a.get_editor_property('team_id') == team]
            assert len(regions) <= 1
            if regions:
                continue
            owned = sorted([a for a in starts if any(str(tag).startswith('FPS'+color) for tag in a.tags)], key=lambda a: str(a.tags))
            assert len(owned) == 5, (color, len(owned))
            region = actors.spawn_actor_from_class(unreal.FPSBaseRegion, unreal.Vector(x,0,150))
            region.set_actor_label('FPS_' + color + '_BaseRegion')
            region.set_editor_property('team_id', team)
            region.set_editor_property('starts', owned)
            region.set_editor_property('fallback_start', owned[0])
            # Re-run construction after setting team-dependent signs.
            region.set_actor_location(unreal.Vector(x,0,150), False, False)
            changed = True
    if changed:
        assert levels.save_current_level()
    unreal.log('FPS_CHARACTER_SETUP PASS ' + map_name + ' changed=' + str(changed))
unreal.log('FPS_CHARACTER_SETUP COMPLETE')
