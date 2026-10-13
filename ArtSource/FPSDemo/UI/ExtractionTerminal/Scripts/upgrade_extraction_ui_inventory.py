"""Explicit v1 -> v2 factory/grid UI migration through the editor.

Backs up this suite's terminal before replacing its v1 layout. Existing row assets,
legacy Industrial UI, fonts, themes, gameplay and map packages remain unchanged.
Version 2 reruns validate without replacing later Designer edits.
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
OUT = ROOT / 'Saved/FPSExtraction/UI/InventoryUpgrade' / datetime.now().strftime('%Y%m%d-%H%M%S-%f')
OUT.mkdir(parents=True, exist_ok=False)

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

before = digest(DISK) if DISK.exists() else None
if DISK.exists():
    shutil.copy2(DISK, OUT / DISK.name)
others = {p: digest(p) for p in (ROOT/'Content').rglob('*.uasset') if p != DISK}
maps = {p: digest(p) for p in (ROOT/'Content').rglob('*.umap')}
result = unreal.FPSExtractionUIBuilder.upgrade_inventory_assets()
assert result.startswith('PASS'), result
assert all(digest(p) == h for p, h in others.items()), 'Unrelated asset changed'
assert all(digest(p) == h for p, h in maps.items()), 'Map changed'
bp = unreal.load_asset(PATH)
assert isinstance(bp, unreal.WidgetBlueprint) and bp.generated_class(), PATH
tree = unreal.find_object(bp, 'WidgetTree')
assert tree, 'Missing widget tree'
version = unreal.find_object(tree, 'InventoryLayoutVersion')
assert isinstance(version, unreal.TextBlock) and str(version.get_text()) == '2'
controls = ['Overview','Warehouse','Production','Modification','OverviewPanel',
    'InventoryPanel','WorkbenchPanel','EquipmentPanel','BaseRows','ResultRows',
    'EnterWarehouse','EnterProduction','EnterModification','PrimarySlot','SecondarySlot',
    'SelectedInfo','SelectedRotate','SelectedTransfer','SelectedSell','InventoryFeedback',
    'RaidSelectedInfo','RaidRotate','SelectedDrop','BagPanel','CloseBag','ResultPanel','Return']
assert all(unreal.find_object(tree, name) for name in controls), 'Required v2 control missing'
grids = []
for name, container, size in [
    ('WarehouseGrid', unreal.FPSExtractionContainer.WAREHOUSE, (464,464)),
    ('BaseBackpackGrid', unreal.FPSExtractionContainer.BACKPACK, (290,232)),
    ('RaidBackpackGrid', unreal.FPSExtractionContainer.BACKPACK, (360,288)),
]:
    widget = unreal.find_object(tree, name)
    assert isinstance(widget, unreal.FPSExtractionGridUI), name
    assert widget.get_visibility() == unreal.SlateVisibility.VISIBLE, name
    assert widget.get_editor_property('container') == container, name
    slot = widget.get_editor_property('slot')
    assert slot.get_size().to_tuple() == size, name
    assert widget.get_editor_property('text_font') and widget.get_editor_property('number_font'), name
    grids.append(dict(name=name, position=slot.get_position().to_tuple(), size=size,
        cell_size=widget.get_editor_property('cell_size')))
assert unreal.find_object(tree,'WeaponSummary').get_editor_property('slot').get_size().y >= 116
report = dict(status='PASS', native_result=result, version=2, asset=PATH,
    before_sha256=before, after_sha256=digest(DISK), grids=grids, controls_checked=controls,
    unrelated_assets_unchanged=len(others), maps_unchanged=len(maps),
    generator=str(Path(__file__).resolve()), recorded=datetime.now().isoformat(),
    scope='Editor migration and saved tree validation. Native/Slate interaction and visual QA run separately.')
(OUT/'Report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
unreal.log('EXTRACTION_UI_INVENTORY_UPGRADE PASS ' + str(OUT/'Report.json'))
