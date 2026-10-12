"""Verify the first source-layout migration against its pre-move inventory.

Run from the project root after the moves. Does not regenerate or alter assets.
Saved/ArtOrganization/Before.json is the captured pre-migration input; the durable
report contains both old/new paths and hashes. Not a general asset quality test.
"""
import ast
import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EVIDENCE = ROOT / 'Saved/ArtOrganization'
M4 = 'ArtSource/FPSDemo/Samples/Weapons/M4'
FONTS = 'ArtSource/FPSDemo/Samples/UI/FontComparisons'
allowed_changes = {f'{M4}/Scripts/{name}.py' for name in
                   ('build_sample', 'export_sample', 'verify_export')}
before = json.loads((EVIDENCE/'Before.json').read_text(encoding='utf-8-sig'))
rows = []
for row in before:
    target = row['new']
    if target.startswith(M4+'/Source/'):
        target = target.replace(M4+'/Source/', M4+'/Scripts/', 1)
    elif target.startswith(M4+'/M4_Precision_Study.blend'):
        target = target.replace(M4+'/', M4+'/Source/', 1)
    source_path = (ROOT/row['old']).resolve()
    target_path = (ROOT/target).resolve()
    assert source_path.is_relative_to(ROOT) and target_path.is_relative_to(ROOT)
    assert not source_path.exists(), f'Duplicate old source: {source_path}'
    assert target_path.is_file(), target
    current_hash = hashlib.sha256(target_path.read_bytes()).hexdigest()
    unchanged = current_hash == row['sha256'].lower()
    assert unchanged or target in allowed_changes, f'Unexpected content change: {target}'
    if target_path.suffix == '.py':
        ast.parse(target_path.read_text(encoding='utf-8'), filename=target)
    rows.append(dict(old=row['old'], new=target, before_sha256=row['sha256'].lower(),
                     after_sha256=current_hash, unchanged=unchanged))

# Current preview links must resolve; historical snapshots retain their old bytes.
for relative in [M4+'/Previews/Compare.html', FONTS+'/ThreeWayComparison.html']:
    page = ROOT/relative
    for value in re.findall(r'(?:src|href)="([^"#]+)"', page.read_text(encoding='utf-8')):
        if not value.startswith(('http:', 'https:', 'data:')):
            assert (page.parent/value).is_file(), (relative, value)
for view in ['Hero', 'Profile', 'FirstPerson']:
    for prefix in ['Previews', 'Revisions/v2/Previews']:
        assert (ROOT/M4/prefix/(view+'.png')).is_file()
for family in ['MiSans', 'HarmonyOS']:
    for weight in ['Regular', 'Bold']:
        name = f'MiSans-{weight}.ttf' if family == 'MiSans' else f'HarmonyOS_Sans_SC_{weight}.ttf'
        assert (ROOT/FONTS/'Fonts'/name).is_file(), name
    assert (ROOT/FONTS/'Previews'/family/'Lobby.png').is_file()

cpp = (ROOT/'Source/MCP_Test/FPSUIAssetBuilder.cpp').read_text(encoding='utf-8')
assert FONTS+'/Fonts' in cpp
font_files = list((ROOT/FONTS/'Fonts').glob('*'))
assert sum(p.suffix in ('.ttf', '.otf') for p in font_files) >= 4
for name in ['build_fps_font_comparison.py', 'verify_fps_misans.py']:
    script = ROOT/'Tools'/name
    ast.parse(script.read_text(encoding='utf-8'), filename=str(script))
    assert FONTS in script.read_text(encoding='utf-8')
assert FONTS in (ROOT/'Tools/render_fps_font_comparison.cjs').read_text(encoding='utf-8')

markers = {'BlenderPaths.log': 'M4_BUILD_PATHS_PASS',
           'BlenderSource.log': 'M4_SAVED_SOURCE_CHECK_PASS',
           'BlenderFBX.log': 'FBX_REIMPORT_PASS',
           'EditorBuild.log': 'Result: Succeeded'}
for name, marker in markers.items():
    assert marker in (EVIDENCE/name).read_text(encoding='utf-8-sig', errors='replace'), name
report = dict(generated_by='Tools/check_art_source_migration.py',
              version='ArtSource-Organization-20261012-batch1',
              scope='M4 sample and font-comparison source files only; Content not relocated',
              input_inventory='Saved/ArtOrganization/Before.json',
              status='PASS', file_count=len(rows),
              unchanged_count=sum(r['unchanged'] for r in rows),
              changed_scripts=[r['new'] for r in rows if not r['unchanged']],
              all_source_migration_complete=False,
              log_markers=markers, files=rows)
(ROOT/'ArtSource/FPSDemo/SourceMigration.json').write_text(
    json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
print('ART_SOURCE_MIGRATION_PASS', len(rows), 'files;', report['unchanged_count'], 'unchanged')
