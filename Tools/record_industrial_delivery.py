"""Record disk artifacts and existing validation evidence; does not rerun Unreal tests."""
import hashlib
import json
import subprocess
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'ArtSource/FPSDemo/Industrial'
CONTENT = ROOT / 'Content/FPSDemo/Art/Industrial'


def read_json(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def record(path):
    return {
        'path': path.relative_to(ROOT).as_posix(),
        'bytes': path.stat().st_size,
        'sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
        'modified_utc': datetime.fromtimestamp(path.stat().st_mtime, timezone.utc).isoformat(),
    }


source_check = read_json(SOURCE / 'SourceValidation.json')
import_check = read_json(ROOT / 'Saved/IndustrialArt/ImportReport.json')
assert source_check['success'] and import_check['success'], 'Existing validation failed'
capture_log = ROOT / 'Saved/IndustrialArt/Capture.log'
capture_lines = capture_log.read_text(encoding='utf-8', errors='replace').splitlines() if capture_log.exists() else []
assets = sorted(p for p in CONTENT.rglob('*') if p.suffix in ('.uasset', '.umap'))
runtime_file=SOURCE/'RuntimeParts.json'
runtime_count=len(read_json(runtime_file)['assets']) if runtime_file.exists() else 0
assert len(assets) == 46+runtime_count, f'Expected {46+runtime_count} art packages, found {len(assets)}'
files = sorted(p for p in SOURCE.rglob('*') if p.suffix in ('.blend', '.fbx', '.png'))
scripts = [ROOT / 'Tools' / name for name in (
    'build_industrial_art.py', 'render_industrial_icons.py', 'validate_industrial_blender.py',
    'import_industrial_art.py', 'ImportIndustrialArt.ps1', 'capture_industrial_art.py',
    'record_industrial_delivery.py', 'CaptureIndustrialArt.ps1',
    'export_industrial_runtime_parts.py', 'integrate_industrial_art.py', 'CaptureFPSArt.ps1',
    'BuildFPSArt.ps1',
)]
report = {
    'generated_by': 'Tools/record_industrial_delivery.py',
    'generated_utc': datetime.now(timezone.utc).isoformat(),
    'repository_head_at_recording': subprocess.check_output(
        ['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
    'scope': 'Industrial art library and runtime binding. Integration report records saved data; visual/gameplay acceptance is documented separately in Docs/FPS/Art/Integration.md.',
    'source_validation': source_check,
    'unreal_import_validation': import_check,
    'runtime_integration': read_json(SOURCE/'IntegrationReport.json') if (SOURCE/'IntegrationReport.json').exists() else None,
    'unreal_capture_present': (SOURCE / 'Previews/UnrealShowcase.png').exists(),
    'unreal_capture_log_evidence': [line for line in capture_lines
                                   if 'INDUSTRIAL_CAPTURE' in line or 'MapCheck:' in line],
    'files': [record(p) for p in files + assets + scripts],
}
package=ROOT/'Saved/IndustrialArt/Package/ArtIntegration-20261011-v3/Report.json'
if package.exists():
    package_result=read_json(package)
    assert package_result['result']=='PASS'
    for entry in package_result['inputs']:
        assert hashlib.sha256((ROOT/entry['path']).read_bytes()).hexdigest().upper()==entry['sha256'].upper(),entry['path']
    verification={}
    def evidence(name,paths,required):
        contents=[(ROOT/p).read_text(encoding='utf-8-sig',errors='replace') for p in paths]
        text='\n'.join(contents)
        for marker in required:assert marker in text,(name,marker)
        markers=[line for line in text.splitlines() if 'FPS_' in line and any(k in line for k in ('PASS','FAIL','COMPLETE'))]
        assert not any('FAIL' in line for line in markers),name
        verification[name]={'result':'PASS','logs':[record(ROOT/p) for p in paths],'markers':markers}
    base='Saved/IndustrialArt/'
    evidence('editor_build',[base+'IntegrationBuild.log'],['Result: Succeeded','FPS_WEAPON_DATA PASS'])
    evidence('loadout_lan',[base+'LoadoutTests/lag0-loss0-visual/'+side+'.log' for side in ('server','client')],['PASS SERVER COMPLETE','PASS CLIENT COMPLETE'])
    evidence('drop_notice_lan',[base+'DropTests/lag0-loss0/'+side+'.log' for side in ('server','client')],['PASS SERVER COMPLETE','PASS CLIENT COMPLETE','PASS SERVER capacity recycling notice','PASS CLIENT capacity recycling notice'])
    evidence('packaged_presentation',[base+'PackagedPresentation/Presentation-'+side+'.log' for side in ('server','client','observer')],['FPS_PRESENTATION_SMOKE PASS SERVER COMPLETE','FPS_PRESENTATION_SMOKE PASS CLIENT COMPLETE','FPS_PRESENTATION_OBSERVER PASS CLIENT COMPLETE'])
    evidence('packaged_maps',[base+'PackagedMaps/Map-'+mode+'.log' for mode in ('Solo','Training')],['FPS_MAP_SMOKE PASS map=LV_FPSFoundry','FPS_MAP_SMOKE PASS map=LV_FPSTraining'])
    evidence('captures',[base+'Captures/'+folder+'/Capture.log' for folder in ('Lobby-1280x800','Waiting-1280x800','Report-1280x800','Training-1280x800','Training-1920x1080','Training-2560x1440')],['FPS_ART_CAPTURE PASS sequence-complete','FPS_ART_CAPTURE PASS death-state'])
    report['integration_verification']=verification
    report['verification_scope']='Functional packaged tests and most captures use v2. Final v3 changes only HUD red-team label/marker spacing; packaged map smoke and Foundry preview are refreshed on v3. Exact launch paths remain in evidence logs.'
    report['windows_package']={'evidence':record(package),'result':'PASS','executable':package_result['executable'],'executable_sha256':package_result['executable_sha256'],'inputs_unchanged':True}
(SOURCE / 'DeliveryReport.json').write_text(
    json.dumps(report, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
print(f'Recorded {len(files)} source/media files, {len(assets)} Unreal packages, {len(scripts)} scripts')
