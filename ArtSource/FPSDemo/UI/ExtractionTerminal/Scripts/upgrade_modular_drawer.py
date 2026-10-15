"""Editor-only, targeted six-category drawer spacing upgrade; preserves other UI."""
from pathlib import Path
from datetime import datetime
import hashlib, json, shutil
import unreal

ROOT=Path(__file__).resolve().parents[5]
PACKAGE='/Game/FPSDemo/Art/UI/ExtractionTerminal/WBP_ExtractionTerminal'
DISK=ROOT/'Content/FPSDemo/Art/UI/ExtractionTerminal/WBP_ExtractionTerminal.uasset'
OUT=ROOT/'Saved/ModularPartsTests'/('Drawer-'+datetime.now().strftime('%Y%m%d-%H%M%S'))
bp=unreal.load_asset(PACKAGE);assert isinstance(bp,unreal.WidgetBlueprint)
tree=unreal.find_object(bp,'WidgetTree');assert tree
assert str(unreal.find_object(tree,'WeaponWorkbenchLayoutVersion').get_text())=='4'
layout={'AttachmentRows':(8,70,334,204),'DrawerStatsRule':(16,286,318,1),
        'CurrentStatsTitle':(16,298,318,27),'CurrentWeaponStats':(16,328,318,146),
        'CandidateStatsTitle':(16,482,318,27),'CandidateWeaponStats':(16,514,318,130)}
hidden={'SlotOptic','SlotMuzzle','SlotMagazine'}

def snapshot():
    result={}
    def visit(w):
        name=w.get_name()
        if name not in layout and name not in hidden:
            row={'class':w.get_class().get_name(),'visibility':str(w.get_visibility())}
            slot=w.get_editor_property('slot')
            if isinstance(slot,unreal.CanvasPanelSlot):
                row['position']=slot.get_position().to_tuple();row['size']=slot.get_size().to_tuple()
            if isinstance(w,unreal.TextBlock):
                row['text']=str(w.get_text());row['font']=str(w.get_editor_property('font'))
            if isinstance(w,unreal.Image):row['brush']=str(w.get_editor_property('brush'))
            if isinstance(w,unreal.PanelWidget):row['children']=[w.get_child_at(i).get_name() for i in range(w.get_children_count())]
            result[name]=row
        if isinstance(w,unreal.PanelWidget):
            for i in range(w.get_children_count()):visit(w.get_child_at(i))
    visit(unreal.find_object(tree,'ViewportFit'));return result

before=snapshot();OUT.mkdir(parents=True,exist_ok=False);shutil.copy2(DISK,OUT/DISK.name)
before_hash=hashlib.sha256(DISK.read_bytes()).hexdigest()
for name,rect in layout.items():
    w=unreal.find_object(tree,name);assert w,name
    slot=w.get_editor_property('slot');assert isinstance(slot,unreal.CanvasPanelSlot)
    slot.set_position(unreal.Vector2D(*rect[:2]));slot.set_size(unreal.Vector2D(*rect[2:]))
for name in hidden:unreal.find_object(tree,name).set_visibility(unreal.SlateVisibility.COLLAPSED)
unreal.find_object(tree,'CandidateStatsTitle').set_auto_wrap_text(False)
assert before==snapshot(),'Unrelated widget changed'
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
assert unreal.EditorAssetLibrary.save_loaded_asset(bp,False)
assert before==snapshot(),'Compile altered unrelated widget'
(OUT/'Report.json').write_text(json.dumps(dict(status='PASS',before_sha256=before_hash,
    after_sha256=hashlib.sha256(DISK.read_bytes()).hexdigest(),preserved_widgets=len(before),
    adjusted=layout,hidden=sorted(hidden)),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
unreal.log('FPS_MODULAR_DRAWER PASS '+str(OUT/'Report.json'))
