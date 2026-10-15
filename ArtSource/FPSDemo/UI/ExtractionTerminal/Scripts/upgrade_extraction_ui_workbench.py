"""Add the v3 weapon workbench to a saved v2 terminal without rebuilding other pages.

Run in the coordinated editor after C++ compilation. Native/Slate interaction,
rendering and independent package reload are separate integration checks.
"""
from pathlib import Path
from datetime import datetime
import hashlib
import json
import shutil
import unreal

ROOT = Path(__file__).resolve().parents[5]
ASSET_ROOT = '/Game/FPSDemo/Art/UI/ExtractionTerminal/'
DISK_ROOT = ROOT / 'Content/FPSDemo/Art/UI/ExtractionTerminal'
TERMINAL = DISK_ROOT / 'WBP_ExtractionTerminal.uasset'
ROW = DISK_ROOT / 'WBP_ExtractionAttachmentRow.uasset'
OUT = ROOT / 'Saved/FPSExtraction/UI/WorkbenchUpgrade' / datetime.now().strftime('%Y%m%d-%H%M%S-%f')


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def tree_for(name):
    bp = unreal.load_asset(ASSET_ROOT + name)
    assert isinstance(bp, unreal.WidgetBlueprint) and bp.generated_class(), name
    tree = unreal.find_object(bp, 'WidgetTree')
    assert tree, name
    return bp, tree


def snapshot(tree):
    """Preserve existing hierarchy, Canvas layout, visibility, text and fonts."""
    records = {}

    def visit(widget, parent):
        name = widget.get_name()
        record = dict(parent=parent, type=widget.get_class().get_name(),
                      visibility=str(widget.get_visibility()))
        slot = widget.get_editor_property('slot')
        if isinstance(slot, unreal.CanvasPanelSlot):
            anchors = slot.get_anchors()
            offsets = slot.get_offsets()
            record['canvas'] = dict(position=slot.get_position().to_tuple(),
                size=slot.get_size().to_tuple(), alignment=slot.get_alignment().to_tuple(),
                anchors=(anchors.minimum.to_tuple(), anchors.maximum.to_tuple()),
                offsets=(offsets.left, offsets.top, offsets.right, offsets.bottom),
                auto_size=slot.get_auto_size(), z_order=slot.get_z_order())
        if isinstance(widget, unreal.TextBlock):
            font = widget.get_editor_property('font')
            obj = font.get_editor_property('font_object')
            record['text'] = str(widget.get_text())
            record['font'] = dict(object=obj.get_path_name() if obj else None,
                size=font.get_editor_property('size'),
                face=str(font.get_editor_property('typeface_font_name')))
        records[name] = record
        if isinstance(widget, unreal.PanelWidget):
            for i in range(widget.get_children_count()):
                visit(widget.get_child_at(i), name)

    root = unreal.find_object(tree, 'ViewportFit')
    assert root, 'Missing v2 root'
    visit(root, None)
    return records


bp, tree = tree_for('WBP_ExtractionTerminal')
version = unreal.find_object(tree, 'InventoryLayoutVersion')
assert isinstance(version, unreal.TextBlock) and str(version.get_text()) == '2', 'Run inventory v2 upgrade first'
was_v3 = unreal.find_object(tree, 'WeaponWorkbenchLayoutVersion') is not None
layout_before = snapshot(tree)
content_before = {p: digest(p) for p in (ROOT / 'Content').rglob('*')
                  if p.suffix in ('.uasset', '.umap')}
assert TERMINAL in content_before, 'Save the existing terminal before migration'
OUT.mkdir(parents=True, exist_ok=False)
for path in (TERMINAL, ROW):
    if path.exists():
        shutil.copy2(path, OUT / path.name)
        assert digest(OUT / path.name) == content_before[path], path

result = unreal.FPSExtractionUIBuilder.upgrade_weapon_workbench_assets()
assert result.startswith('PASS'), result
for path, value in content_before.items():
    if path != TERMINAL or was_v3:
        assert path.exists() and digest(path) == value, 'Unexpected modification: ' + str(path)
created = {p for p in (ROOT / 'Content').rglob('*')
           if p.suffix in ('.uasset', '.umap')} - content_before.keys()
assert created <= {ROW}, 'Unexpected new packages: ' + str(created)

bp, tree = tree_for('WBP_ExtractionTerminal')
layout_after = snapshot(tree)
assert all(layout_after.get(n) == value for n, value in layout_before.items()), 'Existing widget layout/text/font changed'
version = unreal.find_object(tree, 'WeaponWorkbenchLayoutVersion')
assert isinstance(version, unreal.TextBlock) and str(version.get_text()) in ('3', '4')
required = ['ModificationPanel', 'WeaponViewport', 'SlotOptic', 'SlotMuzzle', 'SlotMagazine',
            'AttachmentCategory', 'AttachmentRows', 'CurrentWeaponStats',
            'CandidateStatsTitle', 'CandidateWeaponStats']
assert all(unreal.find_object(tree, n) for n in required), 'Missing workbench control'
viewport = unreal.find_object(tree, 'WeaponViewport')
assert isinstance(viewport, unreal.FPSExtractionWeaponViewport)
assert viewport.get_visibility() == unreal.SlateVisibility.VISIBLE
assert isinstance(unreal.find_object(tree, 'AttachmentRows'), unreal.ScrollBox)
for name in ('SlotOptic', 'SlotMuzzle', 'SlotMagazine'):
    button = unreal.find_object(tree, name)
    assert isinstance(button, unreal.FPSExtractionButton)
    assert str(button.get_editor_property('action')) == name
for name in required:
    widget = unreal.find_object(tree, name)
    slot = widget.get_editor_property('slot')
    if isinstance(slot, unreal.CanvasPanelSlot):
        pos, size = slot.get_position(), slot.get_size()
        assert pos.x >= 0 and pos.y >= 0 and size.x > 0 and size.y > 0, name
        assert pos.x + size.x <= 1600 and pos.y + size.y <= 900, name
row_bp, row_tree = tree_for('WBP_ExtractionAttachmentRow')
for name in ('RowTitle', 'RowDetail', 'RowCount', 'RowAction', 'RowActionLabel', 'RowReason'):
    assert unreal.find_object(row_tree, name), name
assert isinstance(unreal.find_object(row_tree, 'RowAction'), unreal.FPSExtractionButton)
report = dict(status='PASS', native_result=result, version=int(str(version.get_text())), recorded=datetime.now().isoformat(),
    generator=str(Path(__file__).resolve()), generator_sha256=digest(Path(__file__)),
    already_v3=was_v3, controls_checked=required, existing_widgets_preserved=len(layout_before),
    layout_before=layout_before, layout_after=layout_after,
    terminal_before_sha256=content_before[TERMINAL], terminal_after_sha256=digest(TERMINAL),
    row_sha256=digest(ROW), unchanged_packages_checked=len(content_before) - (0 if was_v3 else 1),
    scope='Editor save and same-process validation; independent reload, interaction and visual QA remain separate.')
(OUT / 'Report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
unreal.log('EXTRACTION_UI_WORKBENCH_UPGRADE PASS ' + str(OUT / 'Report.json'))
