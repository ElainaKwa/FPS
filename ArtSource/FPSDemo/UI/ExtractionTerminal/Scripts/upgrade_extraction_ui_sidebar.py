"""Targeted sidebar QA upgrade. Run only after the coordinated native rebuild.

Preserves widget identities, positions, unrelated sizes, other widgets and packages.
Only changes explicit text/wrapping fields, EquipmentPolicy height and Credits font.
"""
from pathlib import Path
from datetime import datetime
import hashlib
import json
import shutil
import unreal

ROOT = Path(__file__).resolve().parents[5]
PATH = '/Game/FPSDemo/Art/UI/ExtractionTerminal/WBP_ExtractionTerminal'
DISK = ROOT / 'Content/FPSDemo/Art/UI/ExtractionTerminal/WBP_ExtractionTerminal.uasset'
OUT = ROOT / 'Saved/FPSExtraction/UI/SidebarUpgrade' / datetime.now().strftime('%Y%m%d-%H%M%S-%f')

def digest(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()

bp = unreal.load_asset(PATH)
assert isinstance(bp, unreal.WidgetBlueprint), PATH
tree = unreal.find_object(bp, 'WidgetTree')
assert tree, 'WidgetTree missing'
assert not unreal.find_object(tree, 'InventoryLayoutVersion'), 'Sidebar upgrade is v1-only; use upgrade_extraction_ui_inventory.py for v2'
number_font = unreal.load_asset('/Game/FPSDemo/Art/Industrial/UI/Fonts/F_UI_Number')
assert isinstance(number_font, unreal.Font), 'Existing Rajdhani font missing'
updates = {
    'WeaponSummary': {'text': 'M4\n配件  0 件', 'wrap': False},
    'AmmoSummary': {'wrap': False},
    'EquipmentPolicy': {'text': '枪械、配件保留\n无维修费\n补给需 10 资金', 'wrap': False, 'height': 104},
    'FactoryRule': {'text': '撤离：搜获入库\n失败：仅丢搜获', 'wrap': False},
    'ResupplyLabel': {'text': '补给 / 10 资金'},
    'Credits': {'font_object': number_font},
}
widgets = {name: unreal.find_object(tree, name) for name in updates}
assert all(isinstance(w, unreal.TextBlock) for w in widgets.values()), 'Expected text controls missing; nothing changed'
OUT.mkdir(parents=True, exist_ok=False)
shutil.copy2(DISK, OUT / DISK.name)
others = {p: digest(p) for p in (ROOT/'Content/FPSDemo/Art').rglob('*.uasset') if p != DISK}
before_hash = digest(DISK)
changes = []
for name, fields in updates.items():
    widget = widgets[name]
    slot = widget.get_editor_property('slot')
    before = {'name': name, 'text': str(widget.get_text()), 'auto_wrap_text': widget.get_editor_property('auto_wrap_text')}
    position = slot.get_position().to_tuple() if isinstance(slot, unreal.CanvasPanelSlot) else None
    size = slot.get_size().to_tuple() if isinstance(slot, unreal.CanvasPanelSlot) else None
    if 'text' in fields:
        widget.set_text(fields['text'])
    if 'wrap' in fields:
        widget.set_auto_wrap_text(fields['wrap'])
    if 'font_object' in fields:
        font = widget.get_editor_property('font')
        font.set_editor_property('font_object', fields['font_object'])
        widget.set_font(font)  # Preserve Designer font size, face, spacing, outline.
    if 'height' in fields:
        assert isinstance(slot, unreal.CanvasPanelSlot), name
        slot.set_size(unreal.Vector2D(size[0], fields['height']))
    if position is not None:
        expected_size = (size[0], fields.get('height', size[1]))
        assert slot.get_position().to_tuple() == position and slot.get_size().to_tuple() == expected_size
        before['position'] = position
        before['size'] = size
    changes.append(before)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
# Do not depend on the Python return convention (bool in this UE 5.8 source,
# void in some older bindings). An existing GeneratedClass alone can be stale.
compile_status = str(bp.get_editor_property('status'))
assert 'uptodate' in compile_status.replace('_', '').lower(), f'Blueprint compilation failed: {compile_status}'
assert bp.generated_class(), 'Compiled Blueprint has no GeneratedClass'
assert unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False), 'Save failed'
assert all(digest(p) == h for p,h in others.items()), 'Unrelated package changed'
for name, fields in updates.items():
    widget = widgets[name]
    if 'text' in fields:
        assert str(widget.get_text()) == fields['text'], name
    if 'wrap' in fields:
        assert widget.get_editor_property('auto_wrap_text') == fields['wrap'], name
assert widgets['Credits'].get_editor_property('font').font_object == number_font
report = dict(status='PASS', compile_status=compile_status, asset=PATH, changes=changes, before_sha256=before_hash,
    after_sha256=digest(DISK), unrelated_packages_unchanged=len(others),
    generator=str(Path(__file__).resolve()), recorded=datetime.now().isoformat(),
    scope='Targeted saved asset upgrade only; runtime text requires rebuilt FPSExtractionUI.cpp; screenshots remain separate.')
(OUT/'Report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
unreal.log('EXTRACTION_UI_SIDEBAR_UPGRADE PASS ' + str(OUT/'Report.json'))
