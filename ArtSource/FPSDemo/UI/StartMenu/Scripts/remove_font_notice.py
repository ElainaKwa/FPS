"""Remove only the start screen's visible font footer; retain fonts and license files."""
from pathlib import Path
from datetime import datetime
import hashlib
import json
import shutil
import unreal

ROOT = Path(__file__).resolve().parents[5]
OUT = ROOT / 'Saved/StartMenu/Authoring' / ('FontNotice-' + datetime.now().strftime('%Y%m%d-%H%M%S'))
OUT.mkdir(parents=True, exist_ok=False)
package = '/Game/FPSDemo/Art/UI/StartMenu/WBP_FPSStart'
file = ROOT / 'Content/FPSDemo/Art/UI/StartMenu/WBP_FPSStart.uasset'
before = hashlib.sha256(file.read_bytes()).hexdigest()
shutil.copy2(file, OUT / file.name)
bp = unreal.load_asset(package)
tree = unreal.find_object(bp, 'WidgetTree')
notice = unreal.find_object(tree, 'FontNotice')
changed = notice is not None and notice.get_parent() is not None
if changed:
    notice.remove_from_parent()
    assert notice.get_parent() is None
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    assert unreal.EditorAssetLibrary.save_loaded_asset(bp, False)
report = dict(status='PASS', asset=package, removed=changed,
              before_sha256=before, after_sha256=hashlib.sha256(file.read_bytes()).hexdigest())
(OUT / 'Report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
unreal.log('FPS_START_FONT_NOTICE PASS ' + str(OUT / 'Report.json'))
