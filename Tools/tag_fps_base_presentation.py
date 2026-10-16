"""Tag the two existing Crossing base signs through Editor APIs; do not rebuild the map."""
import datetime
import hashlib
import json
import shutil
from pathlib import Path
import unreal

root = Path(__file__).resolve().parents[1]
map_path = '/Game/FPSDemo/Maps/Gameplay/IndustrialCrossing/LV_FPSIndustrialCrossing'
map_file = root / 'Content/FPSDemo/Maps/Gameplay/IndustrialCrossing/LV_FPSIndustrialCrossing.umap'
out = root / 'Saved/ExtractionSafety/Authoring' / datetime.datetime.now().strftime('%Y%m%d-%H%M%S%f')
out.mkdir(parents=True)
digest = lambda path: hashlib.sha256(path.read_bytes()).hexdigest().upper()
before_hash = digest(map_file)
shutil.copy2(map_file, out / map_file.name)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level(map_path)
all_actors = actors.get_all_level_actors()
signs = [a for a in all_actors if isinstance(a, unreal.TextRenderActor)
         and str(a.text_render.get_editor_property('text')) in ('BLUE  /  SAFE BASE', 'RED  /  SAFE BASE')]
assert len(signs) == 2, 'Expected exactly the two existing team-base signs'
changed = []
for sign in signs:
    tags = list(sign.get_editor_property('tags'))
    if unreal.Name('FPSBasePresentation') not in tags:
        tags.append(unreal.Name('FPSBasePresentation'))
        sign.set_editor_property('tags', tags)
        changed.append(sign.get_actor_label())
assert len(actors.get_all_level_actors()) == len(all_actors)
if changed:
    assert levels.save_current_level()
assert levels.load_level(map_path)
tagged = [a for a in actors.get_all_level_actors() if a.actor_has_tag('FPSBasePresentation')]
assert len(tagged) == 2 and all(isinstance(a, unreal.TextRenderActor) for a in tagged)
report = dict(status='passed', map=map_path, source='Tools/tag_fps_base_presentation.py',
              actor_count=len(all_actors), changed=changed, tagged=[a.get_actor_label() for a in tagged],
              sha256_before=before_hash, sha256_after=digest(map_file))
(out / 'Report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
unreal.log('FPS_BASE_PRESENTATION PASS ' + str(out))
