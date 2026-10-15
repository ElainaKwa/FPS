"""Separate process: refresh signatures from the final, saved asset registry graph."""
import json
from pathlib import Path
import unreal

source = Path(__file__).resolve().parents[1]
unreal.AssetRegistryHelpers.get_asset_registry().search_all_assets(True)
for row in json.loads((source / 'manifest.json').read_text(encoding='utf-8'))['weapons']:
    path = '/Game/FPSDemo/Weapons/Definitions/DA_' + row['weapon']
    definition = unreal.load_asset(path)
    assert definition and unreal.EditorAssetLibrary.save_loaded_asset(definition, False), path
unreal.log('FPS_RELOAD_REVISIONS PASS definitions=7')
