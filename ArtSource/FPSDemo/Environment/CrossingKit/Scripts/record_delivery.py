"""Record observed files and successful current-map runs. Never hand-edit the output."""
import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path
import shutil

ROOT=Path(__file__).resolve().parents[5]
OUT=Path(__file__).resolve().parents[1]
SAVED=ROOT/'Saved/IndustrialCrossing'
MAP=ROOT/'Content/FPSDemo/Maps/Gameplay/IndustrialCrossing/LV_FPSIndustrialCrossing.umap'


def receipt(path):
    return {'path':path.relative_to(ROOT).as_posix(),'bytes':path.stat().st_size,
            'sha256':hashlib.sha256(path.read_bytes()).hexdigest()}


runs=[(p,json.loads(p.read_text(encoding='utf-8-sig'))) for p in sorted((SAVED/'Tests').glob('*/run.json'))]
evidence=[]
for mode in ('Solo','LAN'):
    p,run=next((p,r) for p,r in reversed(runs) if r['mode']==mode)
    assert run['status']=='passed',str(p)
    assert MAP.stat().st_mtime <= datetime.fromisoformat(run['started']).timestamp(), 'Map changed after tested version'
    module=ROOT/'Binaries/Win64'/run['module']
    assert receipt(module)['sha256'].upper()==run['module_sha256'].upper(),'Module no longer matches run'
    evidence.append({'run':run,'receipt':receipt(p),'server_log':receipt(p.parent/'server.log')})
    if mode=='LAN': evidence[-1]['client_log']=receipt(p.parent/'client.log')
    else:
        assert run['visual']
        capture_start=datetime.fromisoformat(run['started']).timestamp()
images=[]
for i,name in enumerate(('Overview','LongLane','LoadingBay','ServicePassage','MidGate')):
    frame=SAVED/('CrossingCapture'+str(i)+'.png')
    assert frame.stat().st_mtime>=capture_start,'Stale frame: '+str(frame)
    dest=OUT/'Previews'/('UnrealMap'+name+'.png')
    shutil.copy2(frame,dest)
    images.append(receipt(dest))
hud=SAVED/'GameplayCapture/Match-1600x900/HUD.png'
if hud.is_file():
    dest=OUT/'Previews/UnrealHUD.png'
    shutil.copy2(hud,dest)
    images.append(receipt(dest))
files=[OUT/'Source/CrossingKit.blend',OUT/'manifest.json',MAP]
files+=sorted((OUT/'Exports').glob('*.fbx'))
files+=sorted((ROOT/'Content/FPSDemo/Art/Environment/CrossingKit/Meshes').glob('*.uasset'))
files+=sorted((OUT/'Scripts').glob('*.py'))
files+=[OUT/'README.md',ROOT/'Tools/build_industrial_art.py',ROOT/'Tools/LaunchFPSIndustrialCrossing.ps1',
        ROOT/'Tools/TestFPSIndustrialCrossing.ps1',ROOT/'Tools/CaptureFPSArt.ps1']
files+=[ROOT/'Tools/create_fps_industrial_crossing.py',ROOT/'Source/MCP_Test/FPSNavigationRegion.h',
        ROOT/'Source/MCP_Test/FPSNavigationRegion.cpp',ROOT/'Source/MCP_Test/FPSBotNavigation.cpp',
        ROOT/'Source/MCP_Test/FPSBotNavigation.h',ROOT/'Source/MCP_Test/FPSBotController.cpp',ROOT/'Source/MCP_Test/FPSFlowSmoke.cpp']
data={'generated_by':Path(__file__).relative_to(ROOT).as_posix(),'generated_at_utc':datetime.now(timezone.utc).isoformat(),
      'status':'runtime checks passed; user visual acceptance pending; no packaged-build claim',
      'authoring':json.loads((SAVED/'Authoring.json').read_text(encoding='utf-8')),
      'import':json.loads((SAVED/'KitImport.json').read_text(encoding='utf-8')),
      'files':[receipt(p) for p in files],'unreal_map_images':images,'runs':evidence}
(OUT/'DeliveryReport.json').write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('CROSSING_DELIVERY PASS files='+str(len(files))+' current_runs='+str(len(evidence)))
