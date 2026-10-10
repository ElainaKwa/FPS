"""Apply confirmed stage-three values to existing FPS mode assets only.

Run with UnrealEditor-Cmd -run=pythonscript -script=<this file> after the C++
build exposing selection_request_interval. No assets or levels are created.
Preflight both assets before editing; repeat runs are safe and skip saves when
the confirmed values are already present.
"""
import math

import unreal


ASSETS = unreal.EditorAssetLibrary
TARGETS = (
    ('/Game/FPSDemo/Modes/DA_TeamMatch', unreal.FPSGameplayKind.TEAM_MATCH),
    ('/Game/FPSDemo/Modes/DA_Training', unreal.FPSGameplayKind.TRAINING),
)
VALUES = {
    'dynamic_drop_limit': 30,
    'dynamic_drop_lifetime': 180.0,
    'selection_request_interval': 0.1,
}
# These existing authoring choices are outside this migration's scope.
PRESERVED = (
    'kind', 'default_loadout', 'selectable_weapons',
    'selection', 'replacement', 'death_drop',
)


def snapshot(asset):
    return {name: asset.get_editor_property(name) for name in PRESERVED}


def confirmed(name, value):
    if name == 'dynamic_drop_limit':
        return value == VALUES[name]
    return math.isclose(float(value), VALUES[name], rel_tol=0.0, abs_tol=1e-6)


pending = []
for path, kind in TARGETS:
    if not ASSETS.does_asset_exist(path):
        raise RuntimeError('FPS_DROP_RULES missing existing asset: ' + path)
    asset = ASSETS.load_asset(path)
    if not isinstance(asset, unreal.FPSModeRules):
        raise RuntimeError('FPS_DROP_RULES unexpected asset type: ' + path)
    if asset.get_editor_property('kind') != kind:
        raise RuntimeError('FPS_DROP_RULES mode identity changed: ' + path)
    # Reading every target property also checks that the rebuilt native schema
    # is available before either package is modified.
    old = {name: asset.get_editor_property(name) for name in VALUES}
    unreal.log('FPS_DROP_RULES old ' + path + ' ' + repr(old))
    if old['dynamic_drop_limit'] not in (-1, 30):
        raise RuntimeError('FPS_DROP_RULES unexpected existing limit; '
                           'preserving collaborator value: ' + path)
    pending.append((path, asset, old, snapshot(asset)))

for path, asset, old, preserved in pending:
    changed = False
    for name, value in VALUES.items():
        if not confirmed(name, old[name]):
            asset.set_editor_property(name, value)
            changed = True
    if snapshot(asset) != preserved:
        raise RuntimeError('FPS_DROP_RULES unrelated properties changed: ' + path)
    if changed and not ASSETS.save_loaded_asset(asset, False):
        raise RuntimeError('FPS_DROP_RULES save failed: ' + path)
    readback = ASSETS.load_asset(path)
    if not isinstance(readback, unreal.FPSModeRules):
        raise RuntimeError('FPS_DROP_RULES readback type mismatch: ' + path)
    actual = {name: readback.get_editor_property(name) for name in VALUES}
    if not all(confirmed(name, value) for name, value in actual.items()):
        raise RuntimeError('FPS_DROP_RULES readback mismatch: ' + path + ' ' + repr(actual))
    if snapshot(readback) != preserved:
        raise RuntimeError('FPS_DROP_RULES readback changed unrelated properties: ' + path)
    unreal.log('FPS_DROP_RULES PASS ' + path + ' changed=' + str(changed)
               + ' saved_readback=' + repr(actual))

unreal.log('FPS_DROP_RULES COMPLETE')
