"""Record MiSans delivery evidence and measure visible glyphs in real captures."""
import hashlib
import json
import re
from datetime import datetime, timezone
from pathlib import Path
import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
EVIDENCE = ROOT / 'Saved/MiSansAlignment'
UI = ROOT / 'ArtSource/FPSDemo/Industrial/UI'

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def log_check(path, required):
    content = path.read_text(encoding='utf-8-sig', errors='replace')
    for marker in required:
        assert marker in content, (path, marker)
    for marker in ('FPS_UI_ACTION FAIL', 'FPS_ART_CAPTURE FAIL', 'Fatal error:'):
        assert marker not in content, (path, marker)
    # Editor -game launches experimental editor toolset startup Python without
    # editor-only classes. Preserve these diagnostics; packaged runs have none.
    python_errors = [line for line in content.splitlines() if 'LogPython: Error:' in line]
    if python_errors:
        assert path.is_relative_to(EVIDENCE/'Captures'), (path, python_errors)
        blocks = '\n'.join(line.split('LogPython: Error:', 1)[1] for line in python_errors).split('Traceback (most recent call last):')[1:]
        assert blocks
        for block in blocks:
            assert 'Engine/Plugins/Experimental/Toolset' in block.replace('\\', '/'), block
            assert re.search(r"AttributeError: module 'unreal' has no attribute '(ToolsetDefinition|AgentSkill|PythonTestRunner)'\s*$", block), block
    return {'path': path.relative_to(ROOT).as_posix(), 'sha256': sha(path), 'required': required,
            'editor_toolset_startup_diagnostics': python_errors}

checks = [log_check(EVIDENCE/'Build.log', ['Result: Succeeded', 'FPS_WEAPON_DATA PASS']),
          log_check(EVIDENCE/'Import.log', ['FPS_UI_ASSETS PASS'])]
captures = []
for directory in ('Captures', 'PackageCaptures'):
    for folder in sorted((EVIDENCE/directory).iterdir()):
        checks.append(log_check(folder/'Capture.log', ['FPS_ART_CAPTURE PASS sequence-complete']))
        size = tuple(map(int, folder.name.rsplit('-', 1)[1].split('x')))
        for path in sorted(folder.glob('*.png')):
            with Image.open(path) as image:
                assert image.size == size, path
            captures.append({'path': path.relative_to(ROOT).as_posix(), 'size': size, 'sha256': sha(path)})

measurements = []
def ink(scene, name, box, target_y, color='white'):
    with Image.open(EVIDENCE/'Captures'/f'{scene}.png') as image:
        x, y, right, bottom = box
        pixels = np.asarray(image.convert('RGB'))[y:bottom, x:right].astype(int)
    if color == 'dark':
        mask = pixels.max(2) < 100
    elif color == 'yellow':
        mask = (pixels[:,:,0] > 180) & (pixels[:,:,1] > 130) & (pixels[:,:,2] < 130)
    else:
        mask = (pixels.min(2) > 170) & ((pixels.max(2)-pixels.min(2)) < 30)
    rows, columns = np.where(mask)
    assert len(rows), name
    bounds = [int(x+columns.min()), int(y+rows.min()), int(x+columns.max()), int(y+rows.max())]
    center = [(bounds[0]+bounds[2])/2, (bounds[1]+bounds[3])/2]
    error = abs(center[1]-target_y)
    assert error <= 1.5, (name, center, target_y)
    measurements.append({'name': name, 'source': scene, 'bounds': bounds, 'ink_center': center,
                         'target_center_y': target_y, 'vertical_error_px': round(error, 3)})
    return center

ink('Lobby-1920x1080/Lobby', 'navigation', (190,282,380,341), 315)
ink('Lobby-1920x1080/Lobby', 'help-icon-row', (128,896,360,937), 915)
ink('Lobby-1920x1080/Lobby', 'difficulty-button', (1595,776,1690,838), 805.8)
ink('Lobby-1920x1080/Lobby', 'start-button', (1480,904,1660,962), 936, 'dark')
ink('Training-1920x1080/Loadout', 'close-keycap', (1525,124,1610,159), 141)
ink('Training-1920x1080/Loadout', 'base-shield-row', (285,199,570,241), 217.2)
ink('Training-1920x1080/Loadout', 'equipped-badge', (530,334,592,363), 351, 'dark')
ink('Training-1920x1080/HUD', 'team-marker-row', (337,960,388,994), 976.8)
ink('Training-1920x1080/HUD', 'R-keycap', (1798,988,1815,1006), 997.2, 'dark')
ink('Training-1920x1080/HUD', 'reload-action', (1820,980,1865,1013), 997.2)
ink('Training-1920x1080/HUD', 'training-heading', (829,34,1090,80), 57.6)
ink('Training-1920x1080/Inspect', 'pickup-F-keycap', (902,638,918,659), 648, 'yellow')
ink('Training-1920x1080/Inspect', 'pickup-label', (935,629,1040,669), 648, 'yellow')
ink('Match-2560x1440/HUD', 'blue-team-label', (958,40,1040,116), 76.8)
blue = ink('Match-2560x1440/HUD', 'blue-score', (1047,40,1113,116), 76.8)
timer = ink('Match-2560x1440/HUD', 'timer', (1119,40,1411,116), 76.8)
red = ink('Match-2560x1440/HUD', 'red-score', (1445,40,1506,116), 76.8)
ink('Match-2560x1440/HUD', 'red-team-label', (1520,40,1608,116), 76.8)
symmetry_error = abs((blue[0]+red[0])/2-timer[0])
assert symmetry_error <= 1, symmetry_error

fonts = []
for name in ('MiSans-Regular.ttf', 'MiSans-Bold.ttf', 'MiSans-License.pdf'):
    path = UI/'Fonts'/name
    assert sha(path) == sha(ROOT/'ArtSource/FPSDemo/Samples/UI/FontComparisons/Fonts'/name), name
    fonts.append({'path': path.relative_to(ROOT).as_posix(), 'sha256': sha(path)})
package_path = ROOT/'Saved/IndustrialArt/Package/MiSans-20261012-v1/Report.json'
package = json.loads(package_path.read_text(encoding='utf-8-sig'))
assert package['result'] == 'PASS'
for entry in package['inputs']:
    assert sha(ROOT/entry['path']).lower() == entry['sha256'].lower(), entry['path']
assert any(entry['path'].endswith('MiSans-License.pdf') for entry in package['font_licenses'])
result = {'generated_by': 'Tools/verify_fps_misans.py', 'generated_utc': datetime.now(timezone.utc).isoformat(),
          'status': 'PASS', 'checks': checks, 'captures': captures, 'font_sources_unmodified': fonts,
          'glyph_alignment': measurements, 'score_symmetry_error_px': symmetry_error,
          'package_report': package_path.relative_to(ROOT).as_posix(),
          'limits': ['Glyph bounds use fixed contrast thresholds; visual inspection remains required.',
                     'No new multiplayer, controller, ultrawide or Shipping validation.']}
(UI/'MiSansVerification.json').write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
print(f'PASS MiSans: {len(captures)} captures, {len(measurements)} glyph checks, score symmetry error {symmetry_error}px')
