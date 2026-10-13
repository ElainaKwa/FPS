"""Create missing extraction UI Widget Blueprints after the coordinated C++ build.

Existing layouts are preserved. This script never edits the old terminal assets.
Run using UnrealEditor-Cmd -run=pythonscript -script=<absolute path> -unattended -nullrhi.
"""
from pathlib import Path
from datetime import datetime
import hashlib
import json
import unreal

ROOT = Path(__file__).resolve().parents[5]
TARGET = ROOT / 'Content/FPSDemo/Art/UI/ExtractionTerminal'
REPORT = ROOT / 'Saved/FPSExtraction/UI' / datetime.now().strftime('%Y%m%d-%H%M%S-%f')
REPORT.mkdir(parents=True, exist_ok=False)

def digest(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()

existing = {p: digest(p) for p in (ROOT / 'Content/FPSDemo/Art').rglob('*.uasset')}
result = unreal.FPSExtractionUIBuilder.build_assets()
assert result.startswith('PASS'), result
# Create-only: even existing files inside our own directory must remain unchanged.
assert all(digest(p) == h for p, h in existing.items()), 'An existing art package changed'
required = {
    'WBP_ExtractionTerminal': ['BasePanel', 'RaidHUD', 'BagPanel', 'ResultPanel',
        'BaseRows', 'ResultRows', 'Depart', 'Claim', 'Return',
        'Overview', 'Warehouse', 'Production', 'Modification', 'ExtractionStatus',
        'Interaction', 'AbortConfirm', 'Feedback', 'InventoryLayoutVersion',
        'WarehouseGrid', 'BaseBackpackGrid', 'RaidBackpackGrid',
        'WeaponWorkbenchLayoutVersion', 'ModificationPanel', 'WeaponViewport',
        'AttachmentRows', 'CurrentWeaponStats', 'CandidateWeaponStats'],
    'WBP_ExtractionRow': ['RowTitle', 'RowDetail', 'RowCount', 'RowAction', 'RowReason'],
    'WBP_ExtractionAttachmentRow': ['RowTitle', 'RowDetail', 'RowCount', 'RowAction', 'RowReason'],
}
assets = []
for name, controls in required.items():
    path = '/Game/FPSDemo/Art/UI/ExtractionTerminal/' + name
    bp = unreal.load_asset(path)
    assert isinstance(bp, unreal.WidgetBlueprint), path
    tree = unreal.find_object(bp, 'WidgetTree')
    assert tree, path
    for control in controls:
        assert unreal.find_object(tree, control), (path, control, 'Existing v2 terminal: run upgrade_extraction_ui_workbench.py')
    assert bp.generated_class(), path
    disk = TARGET / (name + '.uasset')
    assets.append({'asset': path, 'sha256': digest(disk), 'controls_checked': controls})
report = dict(status='PASS', native_result=result, recorded=datetime.now().isoformat(),
    generator=str(Path(__file__).resolve()), generator_sha256=digest(Path(__file__)),
    assets=assets, existing_packages_unchanged=len(existing),
    scope='Editor save and tree validation only; visual and gameplay checks are separate.')
(REPORT/'Report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
unreal.log('EXTRACTION_UI_ASSETS PASS ' + str(REPORT/'Report.json'))
