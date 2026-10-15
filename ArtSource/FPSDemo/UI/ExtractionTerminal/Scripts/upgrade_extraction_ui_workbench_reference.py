"""Explicit v3 -> v4 workbench-only migration; preserve other factory pages.

Run in the editor after the coordinated C++ build. Back up before replacing the
ModificationPanel; compare the remaining hierarchy, layout, text and fonts.
"""
from pathlib import Path
from datetime import datetime
import hashlib
import json
import shutil
import unreal

ROOT = Path(__file__).resolve().parents[5]
PACKAGE = '/Game/FPSDemo/Art/UI/ExtractionTerminal/WBP_ExtractionTerminal'
DISK = ROOT / 'Content/FPSDemo/Art/UI/ExtractionTerminal/WBP_ExtractionTerminal.uasset'
OUT = ROOT / 'Saved/FPSExtraction/UI/WorkbenchReferenceUpgrade' / datetime.now().strftime('%Y%m%d-%H%M%S-%f')
EXCLUDED = {'ModificationPanel', 'WeaponWorkbenchLayoutVersion'}
REBUILD = '-WorkbenchReferenceRebuild' in unreal.SystemLibrary.get_command_line()


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def content():
    return {p for p in (ROOT / 'Content').rglob('*') if p.suffix in ('.uasset', '.umap')}


def read():
    bp = unreal.load_asset(PACKAGE)
    assert isinstance(bp, unreal.WidgetBlueprint) and bp.generated_class(), PACKAGE
    tree = unreal.find_object(bp, 'WidgetTree')
    assert tree, 'No widget tree'
    version = unreal.find_object(tree, 'WeaponWorkbenchLayoutVersion')
    assert isinstance(version, unreal.TextBlock), 'Upgrade to v3 first'
    return bp, tree, int(str(version.get_text()))


def preserved_layout(tree):
    records = {}

    def visit(widget, parent):
        name = widget.get_name()
        if name in EXCLUDED:
            return
        record = dict(parent=parent, type=widget.get_class().get_name(),
                      visibility=str(widget.get_visibility()))
        slot = widget.get_editor_property('slot')
        if isinstance(slot, unreal.CanvasPanelSlot):
            a, o = slot.get_anchors(), slot.get_offsets()
            record['canvas'] = dict(position=slot.get_position().to_tuple(),
                size=slot.get_size().to_tuple(), alignment=slot.get_alignment().to_tuple(),
                anchors=(a.minimum.to_tuple(), a.maximum.to_tuple()),
                offsets=(o.left, o.top, o.right, o.bottom),
                auto_size=slot.get_auto_size(), z_order=slot.get_z_order())
        if isinstance(widget, unreal.TextBlock):
            font = widget.get_editor_property('font')
            obj = font.get_editor_property('font_object')
            record['text'] = str(widget.get_text())
            record['font'] = dict(object=obj.get_path_name() if obj else None,
                size=font.get_editor_property('size'), face=str(font.get_editor_property('typeface_font_name')))
        children = []
        if isinstance(widget, unreal.PanelWidget):
            children = [widget.get_child_at(i) for i in range(widget.get_children_count())]
            record['children'] = [c.get_name() for c in children if c.get_name() not in EXCLUDED]
        records[name] = record
        for child in children:
            visit(child, name)

    root = unreal.find_object(tree, 'ViewportFit')
    assert root, 'Missing layout root'
    visit(root, None)
    return records


bp, tree, before_version = read()
assert before_version in (3, 4), 'Reference upgrade requires v3 or v4'
before_layout = preserved_layout(tree)
before = {p: digest(p) for p in content()}
assert DISK in before, 'Save original terminal before upgrading'
OUT.mkdir(parents=True, exist_ok=False)
shutil.copy2(DISK, OUT / DISK.name)
assert digest(OUT / DISK.name) == before[DISK]
result = unreal.FPSExtractionUIBuilder.upgrade_weapon_workbench_reference_assets(REBUILD)
assert result.startswith('PASS'), result
assert content() == before.keys(), 'Migration unexpectedly added or removed a content package'
for path, value in before.items():
    if path != DISK or (before_version == 4 and not REBUILD):
        assert digest(path) == value, 'Unexpected package change: ' + str(path)
bp, tree, version = read()
assert version == 4
after_layout = preserved_layout(tree)
assert before_layout == after_layout, 'A factory page outside the workbench changed'
controls = ['ModificationPanel', 'WeaponOverviewPanel', 'WeaponEnterPanel', 'AttachmentDrawer',
    'ViewPrimaryWeapon', 'ViewSecondaryWeapon', 'EnterWeaponModification', 'WorkbenchBack',
    'WorkbenchOverview', 'CloseAttachmentDrawer', 'ResetWorkbenchView', 'WeaponViewport',
    'OverviewWeaponName', 'OverviewWeaponStats', 'WeaponInstanceNote', 'WeaponModificationNote',
    'AttachmentRows', 'CurrentWeaponStats', 'CandidateWeaponStats', 'CandidateStatsTitle',
    'SlotOptic', 'SlotMuzzle', 'SlotMagazine', 'WorkbenchFeedback']
assert all(unreal.find_object(tree, name) for name in controls), 'Missing v4 control'
panel = unreal.find_object(tree, 'ModificationPanel')
assert panel.get_parent().get_name() == 'Layout', 'Workbench must be a sibling of factory BasePanel'
assert panel.get_visibility() == unreal.SlateVisibility.COLLAPSED
assert unreal.find_object(tree, 'AttachmentDrawer').get_visibility() == unreal.SlateVisibility.COLLAPSED
viewport = unreal.find_object(tree, 'WeaponViewport')
assert isinstance(viewport, unreal.FPSExtractionWeaponViewport)
assert viewport.get_visibility() == unreal.SlateVisibility.VISIBLE
slot = viewport.get_editor_property('slot')
assert slot.get_position().to_tuple() == (40, 150)
assert slot.get_size().to_tuple() == (1520, 650)
assert isinstance(unreal.find_object(tree, 'AttachmentRows'), unreal.ScrollBox)
for name in controls:
    widget = unreal.find_object(tree, name)
    if isinstance(widget, unreal.FPSExtractionButton):
        assert str(widget.get_editor_property('action')) == name, name
    if isinstance(widget, unreal.TextBlock):
        assert widget.get_editor_property('font').get_editor_property('font_object'), name
for name in ('PrimaryWeaponImage', 'SecondaryWeaponImage'):
    icon = unreal.find_object(tree, name)
    assert isinstance(icon, unreal.Image) and icon.get_editor_property('brush').get_editor_property('resource_object'), name
report = dict(status='PASS', native_result=result, version=version,
    recorded=datetime.now().isoformat(), generator=str(Path(__file__).resolve()),
    generator_sha256=digest(Path(__file__)), before_version=before_version,
    terminal_before_sha256=before[DISK], terminal_after_sha256=digest(DISK),
    backup=str(OUT / DISK.name), existing_factory_widgets_preserved=len(before_layout),
    layout_before=before_layout, layout_after=after_layout, controls_checked=controls,
    unchanged_packages_checked=len(before) - (before_version != 4 or REBUILD),
    explicit_rebuild=REBUILD,
    scope='Saved editor migration and same-process structure checks. Independent reload, input, model changes and visual acceptance are separate.')
(OUT / 'Report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
unreal.log('EXTRACTION_UI_WORKBENCH_REFERENCE_UPGRADE PASS ' + str(OUT / 'Report.json'))
