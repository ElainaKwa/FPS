"""Package the verified modular M7 revision and its editable source."""
import hashlib,json,zipfile
from pathlib import Path
root=Path(__file__).resolve().parents[1]
manifest=json.loads((root/'AssetManifest.json').read_text(encoding='utf-8'))
validation=json.loads((root/'Validation.json').read_text(encoding='utf-8'))
assert validation['success'],'Only package a validated library.'
source=root/manifest['source_file']
assert hashlib.sha256(source.read_bytes()).hexdigest()==validation['source_sha256_after']
files=[source,root/'AssetManifest.json',root/'Validation.json',root/'GeometryReview.json',root/'SurfaceVolumeReview.json',root/'HighDetailReview.json']
base_file_count=len(files)
files+=sorted((root/'Exports').glob('*.fbx'))+sorted((root/'Exports').glob('*.glb'))
assert len(files)-base_file_count==2*(len(manifest['assets'])+len(manifest['presets']))
files+=sorted((root/'Textures').glob('*.png'))
files+=sorted(root.glob('Preview_*.png'))
files+=sorted((root/'Source').glob('*.py'))
files+=[root/'Reference/ReferenceManifest.json']
target=root/'M7_Modular_Assets_V2.zip'
with zipfile.ZipFile(target,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=6) as archive:
    for file in files:
        assert file.is_file() and file.resolve().is_relative_to(root)
        archive.write(file,file.relative_to(root).as_posix())
with zipfile.ZipFile(target) as archive:assert archive.testzip() is None
print(json.dumps({'archive':str(target),'files':len(files),'bytes':target.stat().st_size}))
