"""Increment existing saved FPS widgets through editor APIs after the native build.

Never calls BuildAssets or imports artwork/fonts. Target packages are copied to
an independent Saved baseline before any editor save. Repeat execution validates
the saved extension without rebuilding trees or resetting Designer properties.
"""
from datetime import datetime
import hashlib
import json
from pathlib import Path
import shutil

import unreal


ROOT = Path(__file__).resolve().parents[1]
UI_ROOT = '/Game/FPSDemo/Art/Industrial/UI'
PAGES = ('Lobby', 'Waiting', 'Pause', 'Report', 'HUD')
TARGETS = [f'{UI_ROOT}/Widgets/WBP_FPS{page}' for page in PAGES]
OUTPUT = ROOT / 'Saved/FPSMovementTests/UIAssets' / datetime.now().strftime('%Y%m%d-%H%M%S-%f')
BASELINE = OUTPUT / 'Baseline'


def filename(package):
    package = str(package).split('.')[0]
    assert package.startswith('/Game/'), package
    return ROOT / 'Content' / (package[6:] + '.uasset')


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def file_record(path):
    details = path.stat()
    return {'path': str(path), 'sha256': sha256(path), 'bytes': details.st_size,
            'modified': datetime.fromtimestamp(details.st_mtime).astimezone().isoformat()}


def identity(asset):
    assert isinstance(asset, unreal.WidgetBlueprint), asset
    generated = asset.generated_class()
    assert generated, asset.get_path_name()
    defaults = unreal.get_default_object(generated)
    theme = defaults.get_editor_property('theme')
    assert theme, asset.get_path_name()
    return {'screen': str(defaults.get_editor_property('screen')), 'theme': theme.get_path_name()}


# Read all identities and files before touching any package, preserving unknown changes.
MODULE = ROOT / 'Binaries/Win64/UnrealEditor-MCP_Test.dll'
module_before = file_record(MODULE)
assets = []
for path, page in zip(TARGETS, PAGES):
    assert unreal.EditorAssetLibrary.does_asset_exist(path), path
    asset = unreal.EditorAssetLibrary.load_asset(path)
    before_identity = identity(asset)
    assert before_identity['screen'] == page, (path, before_identity)
    disk = filename(path)
    assert disk.is_file(), disk
    assets.append((path, before_identity, disk, sha256(disk)))

all_packages = unreal.EditorAssetLibrary.list_assets(UI_ROOT, recursive=True, include_folder=False)
all_files = {str(filename(path)): sha256(filename(path)) for path in all_packages if filename(path).is_file()}
BASELINE.mkdir(parents=True, exist_ok=False)
for path, before_identity, disk, digest in assets:
    backup = BASELINE / disk.name
    shutil.copy2(disk, backup)
    assert sha256(backup) == digest, backup

result = unreal.FPSUIAssetBuilder.extend_movement_assets()
assert result.startswith('PASS'), result

after = []
for path, before_identity, disk, digest in assets:
    readback = unreal.EditorAssetLibrary.load_asset(path)
    assert identity(readback) == before_identity, (path, before_identity, identity(readback))
    current = sha256(disk)
    after.append({'asset': path, 'identity': before_identity, 'before_sha256': digest,
                  'after_sha256': current, 'changed': digest != current,
                  'baseline': str(BASELINE / disk.name)})
    unreal.log('FPS_MOVEMENT_UI_SCRIPT PASS ' + path + ' changed=' + str(digest != current))

target_files = {str(disk) for path, before_identity, disk, digest in assets}
for disk, digest in all_files.items():
    if disk not in target_files:
        assert sha256(Path(disk)) == digest, 'Unrelated UI asset changed: ' + disk

report = {'status': 'PASS', 'native_result': result, 'recorded': datetime.now().astimezone().isoformat(),
          'editor_module': module_before, 'engine_version': unreal.SystemLibrary.get_engine_version(),
          'generator_sha256': sha256(Path(__file__).resolve()),
          'generator': 'Tools/extend_fps_movement_ui.py + FPSUIAssetBuilder::ExtendMovementAssets',
          'targets': after, 'unchanged_ui_packages_checked': len(all_files) - len(target_files),
          'note': 'Native editor extension validates original Widget/Parent/Slot identity; '
                  'this process checks saved identities and unrelated package bytes. '
                  'Rerun in a separate editor process for independent saved-package readback. '
                  'Rendering and gameplay validation are recorded separately.'}
assert file_record(MODULE) == module_before, 'Editor module changed during asset extension'
(OUTPUT / 'Report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
unreal.log('FPS_MOVEMENT_UI_SCRIPT PASS report=' + str(OUTPUT / 'Report.json'))
unreal.log('FPS_MOVEMENT_UI_SCRIPT COMPLETE ' + result)
