"""Create missing system-settings Widget Blueprints; never overwrite existing packages.

Run through Unreal Python after the coordinated C++ build. The native builder
compiles and saves both layouts. Independent reload, UI interaction, display
rollback and visual checks belong to the host's integration test run.
"""
from pathlib import Path
from datetime import datetime
import hashlib
import json
import unreal

ROOT = Path(__file__).resolve().parents[5]
PACKAGE = '/Game/FPSDemo/Art/UI/SystemSettings/'
TARGET = ROOT / 'Content/FPSDemo/Art/UI/SystemSettings'
OUT = ROOT / 'Saved/FPSSettingsTests/UIAssets' / datetime.now().strftime('%Y%m%d-%H%M%S-%f')


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def content_files():
    return {p for p in (ROOT / 'Content').rglob('*') if p.suffix in ('.uasset', '.umap')}


before = {p: digest(p) for p in content_files()}
allowed = {TARGET / 'WBP_SystemSettings.uasset', TARGET / 'WBP_SystemSettingsRow.uasset'}
OUT.mkdir(parents=True, exist_ok=False)
result = unreal.FPSSettingsUIBuilder.build_assets()
assert result.startswith('PASS'), result
assert all(p.exists() and digest(p) == h for p, h in before.items()), 'Existing asset or map changed'
assert content_files() - before.keys() <= allowed, 'Unexpected new package outside settings UI'

required = {
    'WBP_SystemSettings': {
        'SettingsContent': unreal.CanvasPanel,
        'SettingsRows': unreal.ScrollBox,
        'SettingsPageTitle': unreal.TextBlock,
        'SettingsPageNote': unreal.TextBlock,
        'SettingsFeedback': unreal.TextBlock,
        'GraphicsStatus': unreal.TextBlock,
        'HUDPreview': unreal.CanvasPanel,
        'HUDPreviewContent': unreal.CanvasPanel,
        'HUDPreviewCaption': unreal.TextBlock,
        'SettingsModal': unreal.CanvasPanel,
        'ModalShield': unreal.Border,
        'ModalTitle': unreal.TextBlock,
        'ModalBody': unreal.TextBlock,
        'ModalFeedback': unreal.TextBlock,
    },
    'WBP_SystemSettingsRow': {
        'OptionLabel': unreal.TextBlock,
        'OptionDescription': unreal.TextBlock,
        'OptionValue': unreal.TextBlock,
        'OptionSlider': unreal.Slider,
        'OptionChoice': unreal.ComboBoxString,
        'OptionAction': unreal.FPSSettingsButton,
        'OptionActionLabel': unreal.TextBlock,
    },
}
buttons = ['PageGraphics', 'PageSound', 'PageControls', 'PageBindings', 'PageHUD',
    'Back', 'ResetPage', 'ApplyGraphics', 'DiscardGraphics',
    'ExitApply', 'ExitDiscard', 'ExitContinue', 'DisplayConfirm', 'DisplayRevert',
    'ResetConfirm', 'ModalCancel', 'CaptureClear', 'CaptureCancel', 'ConflictReplace', 'ConflictCancel']
required['WBP_SystemSettings'].update({name: unreal.FPSSettingsButton for name in buttons})
assets = []
for name, controls in required.items():
    bp = unreal.load_asset(PACKAGE + name)
    assert isinstance(bp, unreal.WidgetBlueprint) and bp.generated_class(), name
    tree = unreal.find_object(bp, 'WidgetTree')
    assert tree, name
    defaults = unreal.get_default_object(bp.generated_class())
    assert isinstance(defaults, unreal.FPSSettingsUI if name == 'WBP_SystemSettings' else unreal.FPSSettingsRow), name
    dependency = defaults.get_editor_property('theme' if name == 'WBP_SystemSettings' else 'text_font')
    assert dependency, 'Missing shared theme/font: ' + name
    checks = []
    for control, kind in controls.items():
        widget = unreal.find_object(tree, control)
        assert isinstance(widget, kind), (name, control)
        entry = dict(name=control, type=widget.get_class().get_name())
        slot = widget.get_editor_property('slot')
        if isinstance(slot, unreal.CanvasPanelSlot):
            pos, size = slot.get_position(), slot.get_size()
            assert pos.x >= 0 and pos.y >= 0 and size.x > 0 and size.y > 0, control
            entry.update(position=pos.to_tuple(), size=size.to_tuple())
        if isinstance(widget, unreal.FPSSettingsButton):
            assert str(widget.get_editor_property('action')) == control, control
        if isinstance(widget, unreal.TextBlock):
            assert widget.get_editor_property('font').get_editor_property('font_object'), control
        checks.append(entry)
    if name == 'WBP_SystemSettings':
        version = unreal.find_object(tree, 'SettingsLayoutVersion')
        assert isinstance(version, unreal.TextBlock) and str(version.get_text()) == '1'
        assert unreal.find_object(tree, 'SettingsModal').get_visibility() == unreal.SlateVisibility.COLLAPSED
        assert unreal.find_object(tree, 'ModalShield').get_visibility() == unreal.SlateVisibility.VISIBLE
    disk = TARGET / (name + '.uasset')
    assets.append(dict(asset=PACKAGE + name, sha256=digest(disk),
                       dependency=dependency.get_path_name(), controls=checks))
report = dict(status='PASS', native_result=result, version=1, recorded=datetime.now().isoformat(),
    generator=str(Path(__file__).resolve()), generator_sha256=digest(Path(__file__)),
    assets=assets, existing_packages_unchanged=len(before),
    scope='Create-only save and same-process tree inspection; not runtime, independent reload or visual acceptance.')
(OUT / 'Report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
unreal.log('SYSTEM_SETTINGS_UI_ASSETS PASS ' + str(OUT / 'Report.json'))
