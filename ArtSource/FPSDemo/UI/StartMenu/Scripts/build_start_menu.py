"""Create the saved start screen; incrementally patch existing navigation and wording."""
from pathlib import Path
from datetime import datetime
import hashlib
import json
import shutil
import unreal

ROOT = Path(__file__).resolve().parents[5]
OUT = ROOT / 'Saved/StartMenu/Authoring' / datetime.now().strftime('%Y%m%d-%H%M%S')
OUT.mkdir(parents=True)
packages = [
    '/Game/FPSDemo/Art/UI/StartMenu/WBP_FPSStart',
    '/Game/FPSDemo/Art/Industrial/UI/Widgets/WBP_FPSLobby',
    '/Game/FPSDemo/Art/UI/ExtractionTerminal/WBP_ExtractionTerminal',
    '/Game/FPSDemo/Maps/Gameplay/FactoryBase/LV_FPSFactoryBase',
]
digest = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
before = {}
for package in packages:
    disk = ROOT / ('Content/' + package.removeprefix('/Game/') + ('.umap' if package.endswith('LV_FPSFactoryBase') else '.uasset'))
    if disk.exists():
        before[package] = digest(disk)
        shutil.copy2(disk, OUT / disk.name)
result = unreal.FPSUIAssetBuilder.build_start_menu_assets()
assert result.startswith('PASS'), result
bp = unreal.load_asset(packages[0])
tree = unreal.find_object(bp, 'WidgetTree')
for name, action in [('Multiplayer', 'Multiplayer'), ('Recovery', 'Recovery'), ('InputSettings', 'InputSettings'), ('Quit', 'Quit')]:
    button = unreal.find_object(tree, name)
    assert isinstance(button, unreal.FPSActionButton) and str(button.get_editor_property('action')) == action, name
assert str(unreal.get_default_object(bp.generated_class()).get_editor_property('screen')) == 'Start'
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level(packages[3])
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
stations = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.FPSFactoryStation) and str(a.get_editor_property('page')) == 'Overview']
assert len(stations) == 1
stations[0].set_editor_property('display_name', unreal.Text('行动整备'))
assert levels.save_current_level()
after = {}
for package in packages:
    disk = ROOT / ('Content/' + package.removeprefix('/Game/') + ('.umap' if package.endswith('LV_FPSFactoryBase') else '.uasset'))
    after[package] = digest(disk)
report = {'status': 'PASS', 'result': result, 'before_sha256': before, 'after_sha256': after,
          'scope': 'Saved widget assets and one factory station label; runtime checks separate.'}
(OUT / 'Report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
unreal.log('FPS_START_MENU_ASSETS PASS ' + str(OUT / 'Report.json'))
