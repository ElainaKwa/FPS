"""Adjust only the saved stamina panel/track slots, without rebuilding the HUD."""
from pathlib import Path
from datetime import datetime
import hashlib
import json
import shutil
import unreal

ROOT = Path(__file__).resolve().parents[1]
PATH = '/Game/FPSDemo/Art/Industrial/UI/Widgets/WBP_FPSHUD'
OUT = ROOT / 'Saved/FPSMovementTests/StaminaLayout' / datetime.now().strftime('%Y%m%d-%H%M%S')
OUT.mkdir(parents=True, exist_ok=False)
disk = ROOT / 'Content/FPSDemo/Art/Industrial/UI/Widgets/WBP_FPSHUD.uasset'
shutil.copy2(disk, OUT / disk.name)
def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()
others = {p: digest(p) for p in (ROOT/'Content/FPSDemo/Art/Industrial/UI').rglob('*.uasset') if p != disk}
bp = unreal.load_asset(PATH)
tree = unreal.find_object(bp, 'WidgetTree')
assert tree, 'Saved widget tree missing'
panel = unreal.find_object(tree, 'StaminaPanel')
bar = unreal.find_object(tree, 'StaminaBar')
assert isinstance(panel, unreal.CanvasPanel) and isinstance(bar, unreal.ProgressBar)
panel_slot = panel.get_editor_property('slot')
bar_slot = bar.get_editor_property('slot')
assert isinstance(panel_slot, unreal.CanvasPanelSlot) and isinstance(bar_slot, unreal.CanvasPanelSlot)
before = [panel_slot.get_position().to_tuple(), panel_slot.get_size().to_tuple(), bar_slot.get_size().to_tuple()]
panel_slot.set_position(unreal.Vector2D(672.5,740))
panel_slot.set_size(unreal.Vector2D(255,9))
bar_slot.set_position(unreal.Vector2D(0,0))
bar_slot.set_size(unreal.Vector2D(255,9))
delay = unreal.find_object(tree, 'StaminaDelayBar')
if delay:
    delay_slot = delay.get_editor_property('slot')
    delay_slot.set_position(unreal.Vector2D(0,0))
    delay_slot.set_size(unreal.Vector2D(255,9))
assert unreal.BlueprintEditorLibrary.compile_blueprint(bp)
assert unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)
assert all(digest(p) == h for p,h in others.items()), 'Unrelated UI changed'
assert panel_slot.get_position().to_tuple() == (672.5,740.0)
assert panel_slot.get_size().to_tuple() == (255.0,9.0)
assert bar_slot.get_size().to_tuple() == (255.0,9.0)
result = dict(asset=PATH, before=before, position=[672.5,740], size=[255,9],
              status='PASS', sha256=digest(disk), unchanged_packages=len(others),
              generated_by='Tools/update_fps_stamina_layout.py', recorded=datetime.now().isoformat())
(OUT/'Report.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
unreal.log('FPS_STAMINA_LAYOUT PASS '+str(OUT/'Report.json'))
