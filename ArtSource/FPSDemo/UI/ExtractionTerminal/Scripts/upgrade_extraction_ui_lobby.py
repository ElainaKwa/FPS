"""Replace only the old overview, save scene composition, preserve all other pages."""
from pathlib import Path
from datetime import datetime
import hashlib
import json
import shutil
import unreal

ROOT = Path(__file__).resolve().parents[5]
SUITE = ROOT / 'ArtSource/FPSDemo/UI/ExtractionTerminal'
DEST = '/Game/FPSDemo/Art/UI/ExtractionTerminal'
DISK = ROOT / 'Content/FPSDemo/Art/UI/ExtractionTerminal/WBP_ExtractionTerminal.uasset'
OUT = ROOT / 'Saved/FPSExtraction/UI/LobbyUpgrade' / datetime.now().strftime('%Y%m%d-%H%M%S-%f')
REBUILD = '-LobbyRebuild' in unreal.SystemLibrary.get_command_line()
digest = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()

def layout(tree):
    records = {}
    def visit(w, parent):
        name = w.get_name()
        if name in ('OverviewPanel', 'LobbyLayoutVersion'):
            return
        record = {'parent': parent, 'class': w.get_class().get_name(), 'visibility': str(w.get_visibility())}
        slot = w.get_editor_property('slot')
        if isinstance(slot, unreal.CanvasPanelSlot):
            record['position'] = slot.get_position().to_tuple()
            record['size'] = slot.get_size().to_tuple()
        if isinstance(w, unreal.TextBlock) and name != 'OverviewLabel':
            record['text'] = str(w.get_text())
            f = w.get_editor_property('font')
            record['font'] = str(f)
        if isinstance(w, unreal.PanelWidget):
            children = [w.get_child_at(i) for i in range(w.get_children_count())]
            record['children'] = [c.get_name() for c in children if c.get_name() not in ('OverviewPanel', 'LobbyLayoutVersion')]
            for c in children:
                visit(c, name)
        records[name] = record
    visit(unreal.find_object(tree, 'ViewportFit'), None)
    return records

OUT.mkdir(parents=True)
before = {p: digest(p) for p in (ROOT / 'Content').rglob('*') if p.suffix in ('.uasset', '.umap')}
shutil.copy2(DISK, OUT / DISK.name)
bp = unreal.load_asset(DEST + '/WBP_ExtractionTerminal')
tree = unreal.find_object(bp, 'WidgetTree')
old_layout = layout(tree)
version = unreal.find_object(tree, 'LobbyLayoutVersion')
if not version or REBUILD:
    import runpy
    runpy.run_path(str(Path(__file__).with_name('lobby_assets.py')))['ensure_assets'](REBUILD)
result = unreal.FPSExtractionUIBuilder.upgrade_lobby_assets(REBUILD)
assert result.startswith('PASS'), result
tree = unreal.find_object(bp, 'WidgetTree')
assert layout(tree) == old_layout, 'A page outside the lobby changed'
assert str(unreal.find_object(tree, 'LobbyLayoutVersion').get_text()) == '1'
assert unreal.find_object(tree, 'OverviewPanel').get_parent().get_name() == 'Layout'
for name in ('LobbyOverview', 'LobbyWarehouse', 'LobbyFactory', 'LobbyModification', 'LobbySettings', 'LobbyPrepare', 'LobbyDepart', 'LobbyProduction'):
    assert isinstance(unreal.find_object(tree, name), unreal.FPSExtractionButton), name
cdo = unreal.get_default_object(bp.generated_class())
assert cdo.get_editor_property('lobby_presentation')
allowed = {DISK, ROOT/'Content/FPSDemo/Art/UI/ExtractionTerminal/DA_LobbyPresentation.uasset'}
allowed |= {ROOT / f'Content/FPSDemo/Art/UI/ExtractionTerminal/Textures/{n}.uasset' for n in ('T_LobbyVignette', 'T_LobbyIndustrialCrossing')}
changed = [str(p.relative_to(ROOT)) for p, h in before.items() if digest(p) != h and p not in allowed]
assert not changed, changed
report = dict(status='PASS', result=result, saved=str(DISK), before_sha256=before[DISK], after_sha256=digest(DISK),
              generator=str(Path(__file__).resolve()), generator_sha256=digest(Path(__file__)),
              preserved_widgets=len(old_layout), protected_packages=len(before)-len(set(before)&allowed),
              backup=str(OUT / DISK.name), scope='Editor save and structure only; runtime verification is separate.')
(OUT/'Report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
unreal.log('EXTRACTION_LOBBY_UPGRADE PASS ' + str(OUT/'Report.json'))
