"""Record only checks with current on-disk evidence, including preserved model hashes."""
import hashlib
import argparse
import json
from datetime import datetime, timezone
from pathlib import Path
from PIL import Image

ROOT=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser()
parser.add_argument('--package',default='UIRefinement-20261012-v2')
parser.add_argument('--evidence-root',default='Saved/UIRefinement')
parser.add_argument('--build-log',default='Saved/IndustrialArt/UIBuild.log')
parser.add_argument('--import-log',default='Saved/IndustrialArt/UIImport.log')
parser.add_argument('--output',default='Verification.json')
parser.add_argument('--loadout-root',default=None)
parser.add_argument('--capture-folder',default='Final')
args=parser.parse_args()
UI=ROOT/'ArtSource/FPSDemo/Industrial/UI'
OUT=ROOT/args.evidence_root
def digest(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def read(path): return path.read_text(encoding='utf-8-sig', errors='replace')
def evidence(relative, required, forbidden=()):
    path=ROOT/relative
    content=read(path)
    assert all(x in content for x in required), relative
    assert not any(x in content for x in forbidden), relative
    markers=list(required)+['FPS_UI_ACTION','FPS_UI asset-ready']
    return {'path':relative,'sha256':digest(path),'required_markers':list(required),
            'observations':[line.strip() for line in content.splitlines() if any(x in line for x in markers)]}

checks=[evidence(args.build_log, ['Result: Succeeded','FPS_WEAPON_DATA PASS']),
        evidence(args.import_log,['FPS_UI_ASSETS PASS'],['Ensure condition failed','Traceback (most recent call last)'])]
captures=[]
for folder in sorted((OUT/args.capture_folder).glob('*')):
    if not folder.is_dir(): continue
    checks.append(evidence(str((folder/'Capture.log').relative_to(ROOT)).replace('\\','/'),
                           ['FPS_ART_CAPTURE PASS sequence-complete'],['FPS_UI_ACTION FAIL','FPS_ART_CAPTURE FAIL','Fatal error:']))
    width,height=map(int,folder.name.rsplit('-',1)[1].split('x'))
    for file in sorted(folder.glob('*.png')):
        with Image.open(file) as im: assert im.size==(width,height), file
        captures.append({'path':str(file.relative_to(ROOT)).replace('\\','/'),'sha256':digest(file),'size':[width,height]})
assert len(captures)>=15
for role in ('server','client'):
    checks.append(evidence(f'{args.loadout_root or args.evidence_root+"/Loadout"}/lag0-loss0-visual/{role}.log',
                           [f'FPS_LOADOUT_SMOKE PASS {role.upper()} COMPLETE'],['FPS_LOADOUT_SMOKE FAIL']))
prior=json.loads(read(ROOT/'ArtSource/FPSDemo/Industrial/DeliveryReport.json'))
preserved=[]
for row in prior['files']:
    path=ROOT/row['path']
    is_model=path.suffix in ('.blend','.fbx') or ('Content/FPSDemo/Art/Industrial/' in row['path'] and '/UI/' not in row['path'])
    if is_model:
        assert path.exists() and digest(path)==row['sha256'], 'Model changed: '+row['path']
        preserved.append(row['path'])
package_path=f'Saved/IndustrialArt/Package/{args.package}/Report.json'
package=json.loads(read(ROOT/package_path))
assert package['result']=='PASS'
for row in package['inputs']:
    assert digest(ROOT/row['path']).lower()==row['sha256'].lower(), 'Package input changed: '+row['path']
files=[]
for folder in (ROOT/'Content/FPSDemo/Art/Industrial/UI',UI):
    for p in sorted(folder.rglob('*')):
        if p.suffix.lower() in ('.uasset','.png','.otf','.ttf','.txt'):
            files.append({'path':str(p.relative_to(ROOT)).replace('\\','/'),'sha256':digest(p),'bytes':p.stat().st_size})
result={'generated_by':'Tools/record_fps_ui_verification.py','generated_utc':datetime.now(timezone.utc).isoformat(),
        'status':'PASS','checks':checks,'captures':captures,'files':files,
        'model_source_and_asset_hashes_preserved':preserved,'package_report':package_path,
        'package_report_sha256':digest(ROOT/package_path),'executable_sha256':package['executable_sha256'],
        'limits':['Base models are unchanged; reference-quality gun/human modeling remains deferred.',
                  'Pointer events are routed through the offscreen Slate hit-test path; not a physical mouse usability study.',
                  'No ultrawide, controller navigation, performance stress test or shipping build acceptance.']}
(UI/args.output).write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
print(f'FPS_UI_VERIFICATION PASS captures={len(captures)} preserved_models={len(preserved)} ui_files={len(files)}')
